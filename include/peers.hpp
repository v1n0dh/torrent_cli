#ifndef PEERS_HPP
#define PEERS_HPP

#include <cstdint>
#include <iostream>

#include <asio/ip/address.hpp>

#define PEER_ID "23ca8f4f88b60f9825f6"

struct Peer {
	asio::ip::address ip_address;
	uint16_t port;

	Peer(asio::ip::address ip_address, uint16_t port) : ip_address(ip_address), port(port) {}
};

#endif
