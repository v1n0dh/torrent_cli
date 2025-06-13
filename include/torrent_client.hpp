#ifndef TORRENT_CLIENT_HPP
#define TORRENT_CLIENT_HPP

#include <asio/executor_work_guard.hpp>
#include <asio/io_context.hpp>
#include <asio/thread_pool.hpp>
#include <future>
#include <optional>

#include "../include/piece_manager.hpp"
#include "../include/shared_container.hpp"
#include "../include/torrent_file.hpp"
#include "../include/tracker.hpp"
#include "../include/message.hpp"

#define PIECE_HASH_SIZE 20
#define MAX_THREADS 16

class Torrent_Client {
public:
	Torrent_Client(const Torrent_File&& file);
	~Torrent_Client();

	void calculate_pieces();
	void start_io_ctx();
	void get_peers(Tracker&& tracker, std::future<void> _exit_signal_future);
	bool pre_allocate_file();
	void download_file();
	void wait_for_download();

private:
	Torrent_File _torr_file;

	Shared_Queue<Piece_Work> _pw_queue;
	Shared_Queue<Peer*> _peers_queue;

	Bitfield* _bitfield;

	asio::io_context _global_io_ctx;
	std::thread _io_ctx_thread;
	std::optional<asio::executor_work_guard<asio::io_context::executor_type>> _work_gaurd;

	std::thread _peers_retrival_thread;
	std::promise<void> _peers_thread_exit_signal;

	asio::thread_pool pool{MAX_THREADS};
	std::mutex _mtx;

	std::atomic<int> completed_pieces = 0;
};

#endif
