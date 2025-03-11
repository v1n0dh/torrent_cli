#ifndef UTILS_HPP
#define UTILS_HPP

#include <cstdint>
#include <iostream>
#include <vector>

std::vector<uint8_t> hex_string_to_bytes(const std::string& hex_str);

std::string hex_bytes_to_string(std::vector<uint8_t> bytes);

std::string url_encode_hash(const std::string& info_hash);

#endif
