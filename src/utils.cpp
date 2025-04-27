#include <array>
#include <bit>
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

// NOTE: Convert in big-endian format
int uint8_to_uint32(const std::vector<uint8_t>& bytes) {
	return
		(static_cast<uint32_t>(bytes[0]) << 24) |
		(static_cast<uint32_t>(bytes[1]) << 16) |
		(static_cast<uint32_t>(bytes[2]) << 8)  |
		(static_cast<uint32_t>(bytes[3]));
}

// NOTE: Convert in big-endian format
std::vector<uint8_t>  uint32_to_uint8(int data) {
	std::array<uint8_t, 4> b = std::bit_cast<std::array<uint8_t, 4>>(data);
	std::swap(b[0], b[3]);
	std::swap(b[1], b[2]);
	return std::vector<uint8_t>(b.begin(), b.end());
}
