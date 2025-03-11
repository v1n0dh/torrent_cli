#include <cstdint>
#include <cstdio>
#include <iomanip>
#include <vector>
#include <string>

#include "../include/utils.hpp"

std::vector<uint8_t> hex_string_to_bytes(const std::string& hex_str) {
	std::vector<uint8_t> bytes(hex_str.length());
	for (int i = 0; i < hex_str.length(); i++) {
		uint8_t byte = (int)(unsigned char) hex_str[i];
		bytes[i] = byte;
	}
	return bytes;
}

std::string hex_bytes_to_string(std::vector<uint8_t> bytes) {
	std::ostringstream ss;
	for (uint8_t b : bytes)
		ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);

	return ss.str();
}

std::string url_encode_hash(const std::string& info_hash) {
	std::ostringstream url_encode_info_hash;
	for (int i = 0; i < info_hash.length(); i+=2) {
		url_encode_info_hash << std::setw(1) << '%';
		url_encode_info_hash << info_hash[i] << info_hash[i+1];
	}

	return url_encode_info_hash.str();
}
