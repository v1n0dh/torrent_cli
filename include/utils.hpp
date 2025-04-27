#ifndef UTILS_HPP
#define UTILS_HPP

#include <cstdint>
#include <iostream>
#include <type_traits>
#include <vector>

std::vector<uint8_t> hex_string_to_bytes(const std::string& hex_str);

std::string hex_bytes_to_string(std::vector<uint8_t> bytes);

std::string url_encode_hash(const std::string& info_hash);

int uint8_to_uint32(const std::vector<uint8_t>& bytes);

std::vector<uint8_t> uint32_to_uint8(int data);

template <typename... Types>
std::vector<uint8_t> uint32_to_uint8(Types... Args) {
	static_assert((std::is_same_v<Types, int> && ...),
				  "Only int types are allowed to be converted to uint8_t!");
	std::vector<uint8_t> b, tmp;

	for (int i : {Args...}) {
		tmp = uint32_to_uint8(i);
		b.insert(b.end(), tmp.begin(), tmp.end());
		tmp.clear();
	}

	return b;
}

#endif
