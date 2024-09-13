#ifndef TORRENT_FILE_HPP
#define TORRENT_FILE_HPP

#include <cstdint>
#include <iostream>
#include <utility>

class Torrent_Info {
public:
	uint64_t length;
	std::string name;
	uint32_t piece_length;
	std::string pieces;

	Torrent_Info(uint64_t length, std::string name, uint32_t piece_length, std::string pieces) :
		length(length), name(name), piece_length(piece_length), pieces(pieces) {}

	static std::string hash_info_to_sha1(const std::string info_str);
};

class Torrent_File {
public:
	std::string anounce;
	Torrent_Info* info;
	std::string info_hash;

	Torrent_File(const std::string& torrent_file_path);
private:
};

#endif
