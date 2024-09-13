#include <vector>

#include "../include/tracker.hpp"

int main() {
	Torrent_File file("debian.torrent");
	Tracker tracker(&file);
	std::vector<Peer*> peers_lst = tracker.send_tracker_req(PEER_ID, tracker.port);

	return 0;
}
