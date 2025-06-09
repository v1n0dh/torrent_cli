#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "../include/piece_manager.hpp"

File_Mapper::File_Mapper(const std::string& file, size_t file_size)
		: file_path(file), size(file_size), fd(-1) {
	fd = open(file_path.c_str(), O_RDWR, 0666);
	if (fd == -1) {
		perror("Error in opening file\n");
		exit(1);
	}

	data = static_cast<uint8_t*>(mmap(nullptr, this->size - 1, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
	if (data == MAP_FAILED || data == nullptr) {
		perror("Error in mapping file\n");
		close(this->fd);
		exit(-1);
	}
}

File_Mapper::~File_Mapper() {
	if (data != nullptr && data != MAP_FAILED) {
		msync(this->data, this->size, MS_SYNC);
		munmap(this->data, this->size);
	}

	if (fd != -1) close(this->fd);
}

void File_Mapper::wite_piece(const Piece& piece, size_t piece_size) {
	size_t p_offset = piece.index * piece_size;

	for (Block* block : piece.blocks) {
		size_t blk_offset = block->offset * BLOCK_SIZE;
		std::memcpy(this->data + p_offset + blk_offset, block->data.data(), block->length);
	}
}
