#include <vector>

#include "../include/tracker.hpp"

int main() {
	Torrent_File file("debian.torrent");
	Tracker tracker(&file);
	std::vector<Peer*> peers_lst = tracker.send_tracker_req(PEER_ID, tracker.port);
	for (Peer* peer : peers_lst) {
		if (peer->connect()) {
			bool success_handshake = peer->do_handshake(file.info_hash);
			if (success_handshake)
				std::cout << peer->ip_address << " Successful Handshake\n";
		}
	}

	return 0;
}
