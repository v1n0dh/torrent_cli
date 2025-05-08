#ifndef PEERS_HPP
#define PEERS_HPP

#include <asio/io_context.hpp>
#include <cstdint>

#include <asio.hpp>
#include <asio/ip/address.hpp>
#include <asio/error_code.hpp>
#include <asio/ip/address.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <vector>

#define PEER_ID "-VB0001-lDaFog8xBTH1"

#define PEER_TIMEOUT 3

#define HANDSHAKE_PSTR "BitTorrent protocol"
#define HANDSHAKE_PSTR_SIZE 19

#define HANDSHAKE_RESRVE_BYTE_SIZE 8

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

	Peer(const std::string& ip_address, uint16_t port) :
		ip_address(ip_address), port(port), _io_context(), _socket(_io_context) {}

	bool connect();
	bool do_handshake(const std::vector<uint8_t>& info_hash);

	bool send_message(const std::vector<uint8_t>& msg);
	std::vector<uint8_t> recv_message();

	void set_bitfield(const std::vector<uint8_t>& bitfield);
	bool piece_available(int index);
	std::vector<uint8_t> recv_bitfield();

private:
	asio::io_context _io_context;
	asio::ip::tcp::socket _socket;
	std::vector<uint8_t> _bitfield;
};

#endif
