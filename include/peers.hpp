#ifndef PEERS_HPP
#define PEERS_HPP

#include <asio/io_context.hpp>
#include <asio.hpp>
#include <asio/ip/address.hpp>
#include <asio/error_code.hpp>
#include <asio/ip/address.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <cstdint>
#include <memory>
#include <vector>

#include "../include/message.hpp"

#define PEER_ID "-VB0001-lDaFog8xBTH1"

#define PEER_TIMEOUT 3

#define HANDSHAKE_PSTR "BitTorrent protocol"
#define HANDSHAKE_PSTR_SIZE 19

#define HANDSHAKE_RESRVE_BYTE_SIZE 8

#define PEER_DISCONNECTED  0
#define PEER_CONNECTED     1
#define PEER_UNCHOKED      2
#define PEER_CHOKED        3
#define PEER_KEEP_ALIVE    4

struct Handshake {
	std::string pstr;
	std::vector<uint8_t> info_hash;
	std::string peer_id;

	Handshake() {}
	Handshake(const std::vector<uint8_t>& info_hash) : pstr(HANDSHAKE_PSTR), info_hash(info_hash) {}

	// Seralize & Deseralize Handshake Object to & from raw_bytes
	// raw_bytes buffer from is
	// <pstr_len><pstr><reserved_bytes><info_hash><peer_id>
	// pstr_len = 1 byte, pstr = 19 bytes, reserved_bytes = 8 bytes, info_hash = 20 bytes, peer_id = 20 bytes
	std::vector<uint8_t>& operator>>(std::vector<uint8_t>& raw_bytes);
	void operator<<(std::vector<uint8_t>& raw_bytes);
};


struct Peer {
	std::string ip_address;
	uint16_t port;
	int status;

	Bitfield _bitfield;

	Peer(asio::io_context* _io_ctx) : _io_context(_io_ctx), _socket(*_io_ctx) {
		this->status = PEER_DISCONNECTED;
	}

	Peer(const std::string& ip_address, uint16_t port, asio::io_context* _io_ctx) :
		ip_address(ip_address), port(port), _io_context(_io_ctx), _socket(*_io_ctx) {
		this->status = PEER_DISCONNECTED;
	}

	~Peer() { this->close(); }

	bool connect();
	void close();
	bool do_handshake(const std::vector<uint8_t>& info_hash);

	bool send_message(Message& msg);
	Message recv_message();

	void set_bitfield(const std::vector<uint8_t>& bitfield);
	bool piece_available(int index);

private:
	asio::io_context* _io_context;
	asio::ip::tcp::socket _socket;
};

#endif
