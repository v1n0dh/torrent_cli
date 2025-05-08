#ifndef TORRENT_FILE_HPP
#define TORRENT_FILE_HPP

#include <cstdint>
#include <iostream>
#include <vector>

class Torrent_Info {
public:
	uint64_t length;
	std::string name;
	uint32_t piece_length;
	std::vector<uint8_t> pieces;

	Torrent_Info(uint64_t length, std::string name, uint32_t piece_length, const std::vector<uint8_t>& pieces) :
		length(length), name(name), piece_length(piece_length), pieces(pieces) {}

	static std::vector<uint8_t> SHA1_info_hash(const std::string& info_str);
};

class Torrent_File {
public:
	std::string anounce;
	Torrent_Info* info;
	std::vector<uint8_t> info_hash;

	Torrent_File(const std::string& torrent_file_path);
};

#endif
