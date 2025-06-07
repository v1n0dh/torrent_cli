#include <asio/ip/address.hpp>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <json/value.h>
#include <sstream>
#include <cpr/cpr.h>
#include <string>
#include <unordered_map>
#include <vector>

#include "../include/bencode_parser.hpp"
#include "../include/tracker.hpp"
#include "../include/utils.hpp"

std::string Tracker::build_tracker_url(const std::string& peer_id, const std::string& port) {
	std::stringstream url;

	url << torrent->anounce << '?';
	url << "info_hash=" << url_encode_hash(hex_bytes_to_string(torrent->info_hash))  << '&';
	url << "peer_id=" << peer_id << '&';
	url << "port=" << port << '&';
	url << "uploaded=" << this->uploaded << '&';
	url << "downloaded=" << this->downloaded << '&';
	url << "left=" << this->left << '&';
	url << "compact=" << 1 << '&';
	url << "event=" << this->_tracker_event_str[STARTED];

	return url.str();
}

std::unordered_map<std::string, uint16_t> Tracker::extract_peers_from_tracker_resp(const std::string& peers_byts) {
	int peer_size = 6;
	int num_peers = peers_byts.length() / peer_size;
	if (peers_byts.length() % peer_size != 0) {
		std::cerr << "Received malformed peers\n";
		exit(EXIT_FAILURE);
	}

	std::unordered_map<std::string, uint16_t> peers;
	std::vector<uint8_t> peers_vtr = hex_string_to_bytes(peers_byts);

	std::stringstream peer_ss;
	for (int i = 0; i < num_peers; i++) {
		int port;

		// 1st four bytes of peer is ip address & last two bytes are port
		int offset = i * peer_size;
		// Get the IP Address from the peer list
		peer_ss << static_cast<int>(peers_vtr[offset]) << "."
				<< static_cast<int>(peers_vtr[offset+1]) << "."
				<< static_cast<int>(peers_vtr[offset+2]) << "."
				<< static_cast<int>(peers_vtr[offset+3]);

		std::string ip = peer_ss.str();
		peer_ss.str("");    // clear the stream to read port

		// Get the Port Num from the peer list
		port = peers_vtr[offset+5] | (peers_vtr[offset+4] << 8);

		peers[ip] = port;

		std::cout << "IP: " << std::setw(16) << std::left << ip << "Port: " << port << std::endl;
	}

	return peers;
}

std::unordered_map<std::string, uint16_t> Tracker::request_peers(const std::string& peer_id, const std::string& port) {
	std::string url = build_tracker_url(peer_id, port);

	cpr::Response response = cpr::Get(cpr::Url{url});

	if (response.status_code != 200) {
		std::cerr << std::strerror(errno) << "Could not send tracker request\n";
		exit(EXIT_FAILURE);
	}

	Json::Value tracker_resp = bencode_decode(response.text);
	std::string peers = tracker_resp["peers"].asString();

	std::unordered_map<std::string, uint16_t> peers_lst = extract_peers_from_tracker_resp(peers);
	return peers_lst;
}
