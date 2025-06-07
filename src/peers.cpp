#include <asio/completion_condition.hpp>
#include <asio/connect.hpp>
#include <asio/detail/socket_option.hpp>
#include <asio/error.hpp>
#include <asio/error_code.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/socket_base.hpp>
#include <asio/steady_timer.hpp>
#include <asm-generic/socket.h>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <memory>
#include <mutex>
#include <sys/socket.h>
#include <system_error>
#include <vector>

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
	auto timer = std::make_shared<asio::steady_timer>(*this->_io_context);

	auto connection_successful = std::make_shared<bool>(false);
	auto operation_completed = std::make_shared<bool>(false);
	auto mtx = std::make_shared<std::mutex>();
	auto cv = std::make_shared<std::condition_variable>();

	asio::ip::tcp::endpoint endpoint(asio::ip::make_address(this->ip_address, ec), this->port);

	_socket.async_connect(endpoint, [this, timer, connection_successful, operation_completed, mtx, cv](asio::error_code ec) {
		std::lock_guard<std::mutex> lock(*mtx);
		if (!ec) {
			*connection_successful = true;
			this->status = PEER_CONNECTED;
		} else {
			*connection_successful = false;
		}

		*operation_completed = true;
		cv->notify_one();
		timer->cancel();
	});

	timer->expires_after(std::chrono::seconds(PEER_TIMEOUT));
	timer->async_wait([this, timer, connection_successful, operation_completed, mtx, cv](std::error_code ec) {
		if (!ec) {
			std::lock_guard<std::mutex> lock(*mtx);
			if (!operation_completed) {
				_socket.cancel();
				*connection_successful = false;
			}

			*operation_completed = true;
			cv->notify_one();
		}
	});

	// wait until async connect operation completes (or) timer expires
	std::unique_lock<std::mutex> lock(*mtx);
	cv->wait(lock, [&]() { return *operation_completed; });

	return *connection_successful;
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
		this->close();
		return false;
	}

	Handshake handshake_from_peer;
	handshake_from_peer << v_buff;

	// Info hash doesn't match
	if (hex_bytes_to_string(handshake_from_peer.info_hash) != hex_bytes_to_string(info_hash)) {
		this->close();
		return false;
	}

	return true;
}

bool Peer::send_message(Message& msg) {
	asio::error_code ec;

	if (!this->_socket.is_open()) return false;

	std::vector<uint8_t> v_buff;
	msg >> v_buff;

	asio::write(_socket, asio::buffer(v_buff), ec);
	if (ec) {
		std::cerr << ec.message();
		this->close();
		return false;
	}

	return true;
}

Message Peer::recv_message() {
	if (!this->_socket.is_open()) return {};

	asio::error_code ec;
	asio::steady_timer timer(*this->_io_context);

	timer.expires_after(std::chrono::seconds(30));
	timer.async_wait([&](const asio::error_code& ec) { if (!ec) this->close(); });

	std::vector<uint8_t> v_buff(4);

	asio::read(_socket, asio::buffer(v_buff), ec);
	timer.cancel();
	if (ec) { this->close(); return {}; }

	int msg_len = uint8_to_uint32(v_buff);
	v_buff.resize(4 + msg_len);

	timer.expires_after(std::chrono::seconds(30));
	timer.async_wait([&](const asio::error_code& ec) { if (!ec) this->close(); });

	asio::read(_socket, asio::buffer(v_buff.data() + 4, v_buff.size() - 4), ec);
	timer.cancel();
	if (ec) { this->close(); return {}; }

	Message msg;
	msg << v_buff;

	if (msg.len == 0) { this->status = PEER_KEEP_ALIVE; }

	return msg;
}

bool Peer::piece_available(int index) { return _bitfield.has_piece(index); }

void Peer::set_bitfield(const std::vector<uint8_t>& bitfield) { this->_bitfield.set(bitfield); }

void Peer::close() {
	if (this->_socket.is_open()) this->_socket.close();
	this->status = PEER_DISCONNECTED;
}
