#include <asio/completion_condition.hpp>
#include <asio/connect.hpp>
#include <asio/detail/socket_option.hpp>
#include <asio/error_code.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/socket_base.hpp>
#include <asio/steady_timer.hpp>
#include <asm-generic/socket.h>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <system_error>
#include <vector>

#include "../include/message.hpp"
#include "../include/peers.hpp"
#include "../include/utils.hpp"

std::vector<uint8_t>& Handshake::operator>>(std::vector<uint8_t>& raw_bytes) {
	raw_bytes.push_back(HANDSHAKE_PSTR_SIZE);
	raw_bytes.insert(raw_bytes.end(), pstr.begin(), pstr.end());

	// FYI: for reserved bytes in handshake
	for (int i = 0; i < HANDSHAKE_RESRVE_BYTE_SIZE; i++) raw_bytes.push_back(0);

	raw_bytes.insert(raw_bytes.end(), info_hash.begin(), info_hash.end());

	// convert peer-id to hex vector bytes
	std::vector<uint8_t> raw_peer_id = hex_string_to_bytes(PEER_ID);
	raw_bytes.insert(raw_bytes.end(), raw_peer_id.begin(), raw_peer_id.end());

	return raw_bytes;
}

void Handshake::operator<<(std::vector<uint8_t>& raw_bytes) {
	// pstr = raw_bytes[1, 20]
	std::copy(raw_bytes.begin() + 1, raw_bytes.begin() + 20, std::back_inserter(this->pstr));
	// info_hash = raw_bytes[28, 48]
	std::copy(raw_bytes.begin() + 28, raw_bytes.begin() + 48, std::back_inserter(this->info_hash));
	// peer_id = raw_bytes[49, end]
	std::copy(raw_bytes.begin() + 49, raw_bytes.end(), std::back_inserter(this->peer_id));
}

bool Peer::connect() {
	asio::error_code ec;
	asio::steady_timer timer(_io_context);

	bool connection_successful = false;

	asio::ip::tcp::endpoint endpoint(asio::ip::make_address(this->ip_address, ec), this->port);

	_socket.async_connect(endpoint, [&](asio::error_code ec) {
		if (!ec) {
			connection_successful = true;
		}
		timer.cancel();
	});

	timer.expires_after(std::chrono::seconds(PEER_TIMEOUT));
	timer.async_wait([&](std::error_code ec) {
		if (!ec && !connection_successful) {
			std::cout << "\nConnection Timeout to " << this->ip_address << ":" << this->port << "\n";
			_socket.cancel();
		}
	});

	_io_context.run();

	return connection_successful;
}

bool Peer::do_handshake(const std::vector<uint8_t>& info_hash) {
	asio::error_code ec;

	if (!this->_socket.is_open()) return false;

	std::vector<uint8_t> handshake_in_bytes;
	Handshake handshake(info_hash);
	handshake >> handshake_in_bytes;

	_socket.write_some(asio::buffer(handshake_in_bytes), ec);

	std::vector<uint8_t> v_buff(handshake_in_bytes.size());
	_socket.read_some(asio::buffer(v_buff), ec);
	if (ec) {
		std::cerr << ec.message();
		return false;
	}

	Handshake handshake_from_peer;
	handshake_from_peer << v_buff;

	// Info hash doesn't match
	if (hex_bytes_to_string(handshake_from_peer.info_hash) != hex_bytes_to_string(info_hash))
		return false;

	return true;
}

bool Peer::send_message(const std::vector<uint8_t>& msg) {
	asio::error_code ec;

	if (!this->_socket.is_open()) return false;

	_socket.write_some(asio::buffer(msg), ec);
	if (ec) {
		std::cerr << ec.message();
		return false;
	}

	return true;
}

std::vector<uint8_t> Peer::recv_message() {
	asio::error_code ec;
	std::vector<uint8_t> v_buff(4);

	if (!this->_socket.is_open()) return {};

	_socket.read_some(asio::buffer(v_buff), ec);
	if (ec) {
		std::cerr << ec.message();
		return {};
	}
	int msg_len = uint8_to_uint32(v_buff);

	v_buff.resize(4 + msg_len);
	_socket.read_some(asio::buffer(v_buff.data() + 4, v_buff.size() - 4));
	if (ec) {
		std::cerr << ec.message();
		return {};
	}

	return v_buff;
}

void Peer::set_bitfield(const std::vector<uint8_t>& bitfield) {
	this->_bitfield = this->recv_bitfield();
}

bool Peer::piece_available(int index) {
	int byte_idx = index / 8;
	int offset = index % 8;

	if (byte_idx < 0 || byte_idx > _bitfield.size())
		return false;

	// check if index bit is set
	return (_bitfield[byte_idx] & (1 << (7 - offset))) != 0;
}

std::vector<uint8_t> Peer::recv_bitfield() {
	Message msg;

	std::vector<uint8_t> data = recv_message();
	msg << data;

	if (msg.id != MessageType::BITFIELD) {
		std::cerr << "Error: Not Bitfield Message\n";
		return {};
	}

	return msg.payload;
}
