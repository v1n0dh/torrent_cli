#ifndef PIECE_MANAGER_HPP
#define PIECE_MANAGER_HPP

#include <cstdint>
#include <mutex>
#include <vector>

#include "../include/peers.hpp"
#include "../include/shared_container.hpp"
#include "message.hpp"

#define BLOCK_SIZE (1 << 14)
#define MAX_REQUESTS 5

#define BLOCK_PENDING 0
#define BLOCK_REQUESTED 1
#define BLOCK_RECEIVED 2

struct Piece_Work {
	int index;    // Piece Index
	int offset;   // Block offset
	int piece_len;// Piece Length
	std::vector<uint8_t> piece_hash;

	Piece_Work() {}
	Piece_Work(int index, int len, const std::vector<uint8_t>& hash)
		: index(index), offset(0), piece_len(len), piece_hash(hash) {}
};

struct Block {
	int piece_idx;    // Piece Index
	int offset;		  // Block offset
	int length;       // Length of data downloaded
	int status;
	std::vector<uint8_t> data;

	Block(int index, int offset, int len) :
		piece_idx(index), offset(offset), length(len), status(BLOCK_PENDING) {}
};

struct Piece {
	int index;
	int num_blocks;
	int piece_len;
	Shared_Vector<Block*> blocks;

	Piece(int index, int piece_len);
	~Piece();

	Piece(const Piece&) = delete;
	Piece& operator=(const Piece&) = delete;

	// Returns -1 if all the blocks are recieved
	int next_block();
	bool is_completed();
};

class File_Mapper {
public:
	std::string file_path;
	uint8_t* data;
	size_t size;
	int fd;

	File_Mapper(const std::string& file, size_t file_size);
	~File_Mapper();

	void wite_piece(const Piece& piece, size_t piece_size);
	void flush();
};

class Piece_Manager {
public:
	Piece_Manager(Shared_Queue<Piece_Work>* work_queue, int piece_count)
		: pw_queue(work_queue), total_piece_count(piece_count) {}

	bool download_piece(Piece_Work& piece_work,
						Peer& peer,
						File_Mapper& f_mapper,
						size_t piece_size,
						std::mutex& mtx);

	bool check_piece_hash(const Piece& p, const std::vector<uint8_t>& hash);

private:
	Shared_Queue<Piece_Work>* pw_queue;
	int total_piece_count;
};

#endif
