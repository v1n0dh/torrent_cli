#include <charconv>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "../include/utils.hpp"

std::vector<uint8_t> hex_string_to_bytes(const std::string& hex_str) {
	std::vector<uint8_t> bytes(hex_str.length());
	for (int i = 0; i < hex_str.length(); i++) {
		uint8_t byte = (int)(unsigned char) hex_str[i];
		bytes[i] = byte;
	}
	return bytes;
}
