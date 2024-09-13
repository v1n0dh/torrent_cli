#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <json/value.h>
#include <sstream>
#include <openssl/sha.h>
#include <string>

#include "../include/bencode_parser.hpp"
#include "../include/torrent_file.hpp"

// Converts bencoded info to info_hash using SHA1
std::string Torrent_Info::hash_info_to_sha1(const std::string info_str) {
	unsigned char raw_hash[SHA_DIGEST_LENGTH];
	SHA1(reinterpret_cast<const unsigned char*>(info_str.c_str()), info_str.size(), raw_hash);

	std::ostringstream info_hash;
	for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
		info_hash << std::setw(1) << '%';
		info_hash << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(raw_hash[i]);
	}

	return info_hash.str();
}

Torrent_File::Torrent_File(const std::string& torrent_file_path) {
	std::ifstream torrent_file(torrent_file_path);
	if (!torrent_file.is_open()) {
		std::cerr << strerror(errno) << "Unable to open the torrent file " << torrent_file_path << std::endl;
		exit(EXIT_FAILURE);
	}

	std::stringstream torrent_str_buff;
	torrent_str_buff << torrent_file.rdbuf();
	torrent_file.close();

	std::string torrent_str = torrent_str_buff.str();
	if (torrent_str[torrent_str.length() - 1] == '\n') torrent_str.pop_back();

	Json::Value bencode_to_json = bencode_decode(torrent_str);

	this->anounce = bencode_to_json["announce"].asString();
	this->info = new Torrent_Info(bencode_to_json["info"]["length"].asUInt64(),
								  bencode_to_json["info"]["name"].asString(),
								  bencode_to_json["info"]["piece length"].asUInt(),
								  bencode_to_json["info"]["pieces"].asString());
	this->info_hash = Torrent_Info::hash_info_to_sha1(bencode_encode(bencode_to_json["info"]));
}
