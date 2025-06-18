#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <json/value.h>
#include <sstream>
#include <openssl/sha.h>
#include <string>

#include "../include/bencode_parser.hpp"
#include "../include/torrent_file.hpp"

std::vector<uint8_t> Torrent_Info::SHA1_info_hash(const std::string& info_str) {
	unsigned char raw_hash[SHA_DIGEST_LENGTH];
	std::vector<uint8_t> final_hash;

	SHA1(reinterpret_cast<const unsigned char*>(info_str.c_str()), info_str.size(), raw_hash);
	for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
		final_hash.push_back(static_cast<uint8_t>(raw_hash[i]));

	return final_hash;
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
	std::string pieces = bencode_to_json["info"]["pieces"].asString();

	// If announce-list is present, add to announce_list queue
	if (bencode_to_json.isMember("announce-list") &&
		bencode_to_json["announce-list"].isArray() && !bencode_to_json["announce-list"].empty()) {
		Json::Value announce_list_lst = bencode_to_json["announce-list"];
		for (auto announce_list : announce_list_lst) {
			if (announce_list.isArray()) {
				for (auto tracker_url : announce_list)
					this->announce_list.push_back(tracker_url.asString());
			} else if (announce_list.isString())
				this->announce_list.push_back(announce_list.asString());
		}
	}

	if (bencode_to_json.isMember("announce"))
		this->announce = bencode_to_json["announce"].asString();

	this->info = new Torrent_Info(bencode_to_json["info"]["length"].asUInt64(),
								  bencode_to_json["info"]["name"].asString(),
								  bencode_to_json["info"]["piece length"].asUInt(),
								  std::vector<uint8_t>(pieces.begin(), pieces.end()));
	this->info_hash = Torrent_Info::SHA1_info_hash(bencode_encode(bencode_to_json["info"]));
}
