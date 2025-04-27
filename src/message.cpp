#include <algorithm>
#include <cstdint>
#include <iterator>
#include <vector>

#include "../include/message.hpp"
#include "../include/utils.hpp"

Message::Message() { this->len = 0; this->id = 0; }

Message::Message(uint8_t id) : id(id) { this->len = 1; }

Message::Message(uint8_t id, const std::vector<uint8_t>& data) : id(id), payload(data ){
	this->len = this->payload.size() + 1;
}

std::vector<uint8_t>& Message::operator>>(std::vector<uint8_t>& raw_bytes) {
	// split uint32_t to uint8_t in big_endian format (len of message payload)
	std::vector<uint8_t> b = uint32_to_uint8((int) this->len);
	raw_bytes.insert(raw_bytes.end(), b.begin(), b.end());

	if (this->id != 0)
		raw_bytes.push_back(this->id);

	if (!this->payload.empty())
		raw_bytes.insert(raw_bytes.end(), this->payload.begin(), this->payload.end());

	return raw_bytes;
}

void Message::operator<<(std::vector<uint8_t>& raw_bytes) {
	size_t raw_bytes_len = raw_bytes.size();

	// Capture 1st 4 bytes into uint32_t in big endian format
	this->len = uint8_to_uint32(std::vector<uint8_t>(raw_bytes.begin(), raw_bytes.begin() + 4));
	// message id = raw_bytes[4]
	if (raw_bytes_len > 3)
		this->id = raw_bytes[4];
	// message payload = raw_bytes[5, end]
	if (raw_bytes_len > 4)
		std::copy(raw_bytes.begin() + 5, raw_bytes.end(), std::back_inserter(this->payload));
}
