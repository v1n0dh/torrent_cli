#include <asio/executor_work_guard.hpp>
#include <asio/io_context.hpp>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <fcntl.h>
#include <future>
#include <memory>
#include <mutex>
#include <pthread.h>
#include <thread>
#include <unordered_map>
#include <vector>

#include "../include/torrent_client.hpp"
#include "../include/tracker.hpp"
#include "../include/message.hpp"

Torrent_Client::Torrent_Client(const Torrent_File&& file) : _torr_file(file) {}

Torrent_Client::~Torrent_Client() {

	// Kill peers retrival thread
	if (_peers_retrival_thread.joinable()) _peers_retrival_thread.join();

	if (_work_gaurd) { _work_gaurd.reset(); _global_io_ctx.stop(); }

	if (_io_ctx_thread.joinable()) _io_ctx_thread.join();

	this->pool.stop();

	delete this->_bitfield;

	this->_pw_queue.clean();
	this->_peers_queue.clean();
}

void Torrent_Client::start_io_ctx() {
	_work_gaurd.emplace(asio::make_work_guard(_global_io_ctx));

	_io_ctx_thread = std::thread([&] () {
		pthread_setname_np(pthread_self(), "io_ctx thread");
		_global_io_ctx.run();
	});
}

void Torrent_Client::calculate_pieces() {
	int total_piece_count = _torr_file.info->pieces.size() / PIECE_HASH_SIZE;

	#pragma omp parallel for
	for (int piece_index = 0; piece_index < total_piece_count; piece_index++) {
		std::vector<uint8_t> piece_hash;
		piece_hash.insert(piece_hash.end(),
						  _torr_file.info->pieces.begin() + (piece_index * PIECE_HASH_SIZE),
						  _torr_file.info->pieces.begin() + (piece_index * PIECE_HASH_SIZE) + PIECE_HASH_SIZE);

		 int piece_len = (piece_index == total_piece_count - 1 &&
						 _torr_file.info->length % _torr_file.info->piece_length != 0) ?
			_torr_file.info->length % _torr_file.info->piece_length : _torr_file.info->piece_length;

		 Piece_Work pw(piece_index, piece_len, piece_hash);

		_pw_queue << pw;
	}

	this->_bitfield = new Bitfield(total_piece_count);
}

void Torrent_Client::get_peers(Tracker&& tracker, std::future<void> _exit_signal_future) {
	this->_peers_retrival_thread = std::thread(
		[_exit_signal_future = std::move(_exit_signal_future), tracker = std::move(tracker), this] () mutable {
			pthread_setname_np(pthread_self(), "Tracker thread");

			while (_exit_signal_future.wait_for(std::chrono::milliseconds(5)) == std::future_status::timeout) {

				if (_peers_queue.size() > 30) continue;

				std::unordered_map<std::string, uint16_t> peers = tracker.request_peers(PEER_ID, tracker.port);

				Peer* peer;
				for (auto p : peers) {
					peer = new Peer(p.first, p.second, &_global_io_ctx);
					_peers_queue << peer;
				}
			}
		});
}

bool Torrent_Client::pre_allocate_file(std::string& file_path) {
	if (file_path.empty())
		file_path = this->_torr_file.info->name;
	size_t file_size = this->_torr_file.info->length;

	int fd = open(file_path.c_str(), O_RDWR | O_CREAT, 0666);
	if (fd < 0) {
		perror("Error in opening file\n");
		return false;
	}

	if (lseek(fd, file_size - 1, SEEK_SET) == -1) {
		perror("Error pre-allocating file\n");
		return false;
	}

	if (write(fd, "", 1) != 1) {
		perror("Error writing to file\n");
		return false;
	}

	close(fd);

	return true;
}

void Torrent_Client::download_file(std::string& file_path) {
	Tracker tracker(&_torr_file);
	int total_piece_count = _torr_file.info->pieces.size() / PIECE_HASH_SIZE;
	if (file_path.empty()) file_path = this->_torr_file.info->name;

	auto f_mapper = std::make_shared<File_Mapper>(file_path, this->_torr_file.info->length);

	this->get_peers(std::move(tracker), _peers_thread_exit_signal.get_future());

	for (int i = 0; i < MAX_THREADS; i++) {
		asio::post(this->pool, [this, f_mapper, total_piece_count]() {
			Piece_Work pw;
			Piece_Manager pm(&_pw_queue, total_piece_count);
			Peer *peer = nullptr;

			while (true) {
				bool existing_piece = false;

				_pw_queue >> pw;

				// If got poison piece, then break
				if (pw.index == -1) { delete peer; peer = nullptr; break; }

				{
					std::unique_lock<std::mutex> lock(this->_mtx);
					if (this->_bitfield->has_piece(pw.index)) { existing_piece = true; }
				}

				if (existing_piece) continue;

				if (peer == nullptr || peer->status == PEER_DISCONNECTED) {
					if (peer) { delete peer; peer = nullptr; }
					_peers_queue >> peer;

					if(!peer->connect() || !peer->do_handshake(_torr_file.info_hash)) {
						_pw_queue << pw;
						delete peer;
						peer = nullptr;
						continue;
					}
				}

				if (pm.download_piece(pw, *peer, *f_mapper, this->_torr_file.info->piece_length, this->_mtx)) {
					std::unique_lock<std::mutex> lock(this->_mtx);
					this->_bitfield->set_piece(pw.index);
					this->completed_pieces.fetch_add(1);
				} else {
					_pw_queue << pw;
				}
			}
		});
	}
}

void Torrent_Client::wait_for_download(std::string& file_path) {
	int total_piece_count = _torr_file.info->pieces.size() / PIECE_HASH_SIZE;
	if (file_path.empty()) file_path = this->_torr_file.info->name;
	auto f_mapper = std::make_shared<File_Mapper>(file_path, this->_torr_file.info->length);

	std::thread completion_watcher([this, total_piece_count]() {
		pthread_setname_np(pthread_self(), "Exit thread");
		// wait until all pieces are completed
		while (this->completed_pieces.load() < total_piece_count)
			std::this_thread::sleep_for(std::chrono::milliseconds(10));

		// Insert Poison pieces to exit worker threads
		for (int i = 0; i < MAX_THREADS; i++)
			_pw_queue << Piece_Work{-1, 0, {}};

		// Signal tracker thread to exit
		this->_peers_thread_exit_signal.set_value();
	});

	this->pool.join();
	completion_watcher.join();

	std::cout << "Download completed\n";
	f_mapper->flush();
}
