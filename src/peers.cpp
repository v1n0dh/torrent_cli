#include <asio/completion_condition.hpp>
#include <asio/connect.hpp>
#include <asio/detail/socket_option.hpp>
#include <asio/error.hpp>
#include <asio/error_code.hpp>
#include <asio/ip/address.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/socket_base.hpp>
#include <asio/steady_timer.hpp>
#include <asm-generic/socket.h>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <future>
#include <iterator>
#include <memory>
#include <sys/socket.h>
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
	asio::steady_timer timer(*this->_io_context);

	auto conn_successful = std::make_shared<std::promise<bool>>();
	std::future<bool> conn_result_fut = conn_successful->get_future();
	auto promise_set = std::make_shared<std::atomic<bool>>(false);

	asio::ip::tcp::endpoint endpoint(asio::ip::make_address(this->ip_address, ec), this->port);

	timer.expires_after(std::chrono::seconds(PEER_TIMEOUT));
	timer.async_wait([this, conn_successful, promise_set](const asio::error_code& ec) {
		if (!ec && !promise_set->exchange(true)) { this->close(); conn_successful->set_value(false); }
	});

	_socket.async_connect(endpoint,
						  [this, conn_successful, promise_set, &timer] (const asio::error_code& ec) {
		if (!promise_set->exchange(true)) {
			timer.cancel();
			if (!ec) {
				conn_successful->set_value(true);
				this->status = PEER_CONNECTED;
			} else {
				conn_successful->set_value(false);
			}
		}
	});

	return conn_result_fut.get();
}

bool Peer::do_handshake(const std::vector<uint8_t>& info_hash) {
	asio::error_code ec;

	if (!this->_socket.is_open()) { this->close(); return {}; }

	std::vector<uint8_t> handshake_in_bytes;
	Handshake handshake(info_hash);
	handshake >> handshake_in_bytes;

	_socket.write_some(asio::buffer(handshake_in_bytes), ec);

	std::vector<uint8_t> v_buff(handshake_in_bytes.size());
	_socket.read_some(asio::buffer(v_buff), ec);
	if (ec) {
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

	if (!this->_socket.is_open()) { this->close(); return false; }

	std::vector<uint8_t> v_buff;
	msg >> v_buff;

	asio::write(_socket, asio::buffer(v_buff), ec);
	if (ec) {
		this->close();
		return false;
	}

	return true;
}

Message Peer::recv_message() {
	if (!this->_socket.is_open()) { this->close(); return {}; }

	asio::error_code ec;

	auto timer_1 = std::make_shared<asio::steady_timer>(*this->_io_context);
	auto operation_successful_1 = std::make_shared<std::promise<bool>>();
	auto operation_result_fut_1 = operation_successful_1->get_future();
	auto promise_set_1 = std::make_shared<std::atomic<bool>>(false);

	timer_1->expires_after(std::chrono::seconds(5));
	timer_1->async_wait([this, operation_successful_1, promise_set_1](const asio::error_code& ec) {
		if (!ec && !promise_set_1->exchange(true)) { this->close(); operation_successful_1->set_value(false); }
	});

	std::vector<uint8_t> v_buff(4);

	asio::async_read(_socket, asio::buffer(v_buff), [this, operation_successful_1, promise_set_1, timer_1]
					 (const asio::error_code& ec, size_t bytes_transferred) {
		if (!promise_set_1->exchange(true)) {
			if (!ec) {
				operation_successful_1->set_value(true);
				timer_1->cancel();
			} else {
				operation_successful_1->set_value(false);
				this->close();
			}
		}
	});
	if (!operation_result_fut_1.get()) return {};

	int msg_len = uint8_to_uint32(v_buff);
	v_buff.resize(4 + msg_len);

	auto timer_2 = std::make_shared<asio::steady_timer>(*this->_io_context);
	auto operation_successful_2 = std::make_shared<std::promise<bool>>();
	auto operation_result_fut_2 = operation_successful_2->get_future();
	auto promise_set_2 = std::make_shared<std::atomic<bool>>(false);

	timer_2->expires_after(std::chrono::seconds(5));
	timer_2->async_wait([this, operation_successful_2, promise_set_2](const asio::error_code& ec) {
		if (!ec && !promise_set_2->exchange(true)) { this->close(); operation_successful_2->set_value(false); }
	});

	asio::async_read(_socket, asio::buffer(v_buff.data() + 4, msg_len),
					 [this, operation_successful_2, promise_set_2, timer_2]
					 (const asio::error_code& ec, size_t bytes_transferred) {
		if (!promise_set_2->exchange(true)) {
			if (!ec) {
				operation_successful_2->set_value(true);
				timer_2->cancel();
			} else {
				operation_successful_2->set_value(false);
				this->close();
			}
		}
	});
	if (!operation_result_fut_2.get()) return {};

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
