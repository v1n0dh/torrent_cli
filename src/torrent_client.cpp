#include <asio/executor_work_guard.hpp>
#include <asio/io_context.hpp>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <fcntl.h>
#include <future>
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
	_peers_thread_exit_signal.set_value();
	if (_peers_retrival_thread.joinable()) _peers_retrival_thread.join();

	if (_work_gaurd) { _work_gaurd.reset(); _global_io_ctx.stop(); }

	if (_io_ctx_thread.joinable()) _io_ctx_thread.join();

	this->pool.join();
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

void Torrent_Client::get_peers(Tracker& tracker, std::future<void> _exit_signal_future) {
	this->_peers_retrival_thread = std::thread(
		[_exit_signal_future = std::move(_exit_signal_future), &tracker, this] () {

			while (_exit_signal_future.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout) {
				pthread_setname_np(pthread_self(), "Tracker thread");

				if (this->_peers_queue.size() > 30) continue;

				std::unordered_map<std::string, uint16_t> peers = tracker.request_peers(PEER_ID, tracker.port);

				for (auto p : peers) {
					Peer* peer = new Peer(p.first, p.second, &_global_io_ctx);
					_peers_queue << peer;
				}

				std::this_thread::sleep_for(std::chrono::minutes(2));
			}
		});
}

bool Torrent_Client::pre_allocate_file() {
	std::string file_path = this->_torr_file.info->name;
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

void Torrent_Client::download_file() {
	Tracker tracker(&_torr_file);
	int total_piece_count = _torr_file.info->pieces.size() / PIECE_HASH_SIZE;

	this->get_peers(tracker, _peers_thread_exit_signal.get_future());

	File_Mapper mapped_file(this->_torr_file.info->name, this->_torr_file.info->length);

	while (!_pw_queue.empty()) {
		asio::post(this->pool, [&]() {
			if (_pw_queue.empty()) return;
			Peer* peer = nullptr;
			_peers_queue >> peer;

			if (peer == nullptr) { delete peer; return; }

			if(!peer->connect()) { delete peer; return; }
			if(!peer->do_handshake(_torr_file.info_hash))  { delete peer; return; }

			Piece_Manager pm(&_pw_queue, total_piece_count);
			Piece_Work pw;
			_pw_queue >> pw;

			{
				std::unique_lock<std::mutex> lock(this->_mtx);
				if (this->_bitfield->has_piece(pw.index)) { delete peer; return; }
			}

			if (pm.download_piece(pw, *peer, mapped_file, this->_torr_file.info->piece_length)) {
				std::unique_lock<std::mutex> lock(this->_mtx);
				_bitfield->set_piece(pw.index);
			}

			delete peer;
		});
	}
}
