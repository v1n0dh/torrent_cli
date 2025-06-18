#ifndef TRACKER_HPP
#define TRACKER_HPP

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "../include/torrent_file.hpp"

enum Tracker_Event {
	STARTED,
	STOPPED,
	COMPLETE
};

class Tracker {
public:
	Torrent_File *torrent;
	const std::string port = "6881";
	uint64_t uploaded = 0;
	uint64_t downloaded = 0;
	uint64_t left = 0;

	Tracker(Torrent_File* torrent) : torrent(torrent) {}
	std::unordered_map<std::string, uint16_t> request_peers(const std::string& peer_id, const std::string& port);

private:
	const std::vector<std::string> _tracker_event_str{"started", "stopped", "completed"};

	std::string build_tracker_url(const std::string& tracker_url, const std::string& peer_id, const std::string& port);
	std::unordered_map<std::string, uint16_t> extract_peers_from_tracker_resp(const std::string& peers_byts);
};

#endif
