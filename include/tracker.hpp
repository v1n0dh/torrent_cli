#ifndef TRACKER_HPP
#define TRACKER_HPP

#include <cstdint>
#include <vector>

#include "../include/torrent_file.hpp"
#include "peers.hpp"

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
	uint64_t left;

	Tracker(Torrent_File* torrent) : torrent(torrent) {}
	std::string build_tracker_url(const std::string& peer_id, const std::string& port);
	std::vector<Peer*> send_tracker_req(const std::string& peer_id, const std::string& port);

private:
	const std::vector<std::string> _tracker_event_str{"started", "stopped", "completed"};

	std::vector<Peer*> extract_peers_from_tracker_resp(const std::string& peers_byts);
};

#endif
