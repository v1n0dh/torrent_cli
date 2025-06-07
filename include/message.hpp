#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include "utils.hpp"
#include <cstdint>
#include <variant>
#include <vector>

enum MessageType {
	CHOKE = 0,
	UNCHOKE,
	INTERESTED,
	NOT_INTERESTED,
	HAVE,
	BITFIELD,
	REQUEST,
	PIECE,
	CANCEL
};

struct Have_Payload {
	uint32_t piece_index;

	Have_Payload(uint32_t piece_index) : piece_index(piece_index) {}
};

struct Request_Payload {
	uint32_t index;
	uint32_t begin;
	uint32_t length;

	Request_Payload(uint32_t index, uint32_t begin, uint32_t length) :
		index(index), begin(begin), length(length) {}
};

// NOTE: To use same constructor as Request_Payload
struct Cancel_Payload : Request_Payload { using Request_Payload::Request_Payload; };

struct Piece_Payload {
	uint32_t index;
	uint32_t begin;
	std::vector<uint8_t> data;

	Piece_Payload(uint32_t index, uint32_t begin, const std::vector<uint8_t>& data) :
		index(index), begin(begin), data(data) {}
};

struct Bitfield {
	std::vector<uint8_t> bitfield;

	Bitfield() {}
	Bitfield(int piece_count);

	void set(const std::vector<uint8_t>& bitfield);
	bool is_bitfield_set();

	void set_piece(int index);
	bool has_piece(int index);
};

typedef std::variant<Have_Payload, Request_Payload, Cancel_Payload, Piece_Payload> MsgPayloadType;

struct Message {
	uint32_t len;
	uint8_t id;
	std::vector<uint8_t> payload;

	Message();
	Message(uint8_t id);
	Message(uint8_t id, const std::vector<uint8_t>& data);

	// Seralize & Deseralize Message object to & from raw_bytes
	std::vector<uint8_t>& operator>>(std::vector<uint8_t>& raw_bytes);
	void operator<<(std::vector<uint8_t>& raw_bytes);

	template <MessageType t, typename... Types>
	static std::vector<uint8_t> create_payload(Types&&... Args) {
		std::tuple<Types...> args{Args...};

		if constexpr (t == MessageType::CHOKE ||
					  t == MessageType::UNCHOKE ||
					  t == MessageType::INTERESTED ||
					  t == MessageType::NOT_INTERESTED) {
			return std::vector<uint8_t>();
		} else if constexpr (t == MessageType::HAVE) {
			return uint32_to_uint8(std::get<0>(args));
		} else if constexpr (t == MessageType::REQUEST || t == MessageType::CANCEL) {
			std::vector<uint8_t> _payload, b;

			b = uint32_to_uint8(std::get<0>(args), std::get<1>(args), std::get<2>(args));
			_payload.insert(_payload.end(), b.begin(), b.end());

			return _payload;
		} else if constexpr (t == MessageType::PIECE) {
			std::vector<uint8_t> _payload, b;

			b = uint32_to_uint8(std::get<0>(args), std::get<1>(args));
			_payload.insert(_payload.end(), b.begin(), b.end());
			_payload.insert(_payload.end(), std::get<2>(args).begin(), std::get<2>(args).end());

			return _payload;
		}
	}

	template <MessageType t>
	static MsgPayloadType parse_payload(std::vector<uint8_t>& _payload) {
		if constexpr (t == MessageType::CHOKE ||
					  t == MessageType::UNCHOKE ||
					  t == MessageType::INTERESTED ||
					  t == MessageType::NOT_INTERESTED)
			return NULL;
		else if constexpr (t == MessageType::HAVE) {
			return Have_Payload{(uint32_t) uint8_to_uint32(_payload)};
		} else if constexpr (t == MessageType::REQUEST || t == MessageType::CANCEL) {
			uint32_t index = uint8_to_uint32(std::vector<uint8_t>(_payload.begin(), _payload.begin() + 4));
			uint32_t begin = uint8_to_uint32(std::vector<uint8_t>(_payload.begin() + 4, _payload.begin() + 8));
			uint32_t length = uint8_to_uint32(std::vector<uint8_t>(_payload.begin() + 8, _payload.end()));

			if constexpr (t == MessageType::REQUEST)
				return Request_Payload{index, begin, length};

			return Cancel_Payload{index, begin, length};
		} else if constexpr (t == MessageType::PIECE) {
			uint32_t index = uint8_to_uint32(std::vector<uint8_t>(_payload.begin(), _payload.begin() + 4));
			uint32_t begin = uint8_to_uint32(std::vector<uint8_t>(_payload.begin() + 4, _payload.begin() + 8));

			return Piece_Payload {
				index,
				begin,
				std::vector<uint8_t>(_payload.begin() + 8, _payload.end())
			};
		}
	}
};

#endif
