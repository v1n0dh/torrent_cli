#include <cstdint>
#include <vector>

#include "../include/tracker.hpp"
#include "../include/message.hpp"

int main() {
	Torrent_File file("debian.torrent");
	Tracker tracker(&file);
	std::vector<Peer*> peers_lst = tracker.send_tracker_req(PEER_ID, tracker.port);

	std::vector<uint8_t> payload{0, 0, 7, 109, 0, 0, 0, 5, 5, 6, 7};

	Piece_Payload p = std::get<Piece_Payload>(Message::parse_payload<MessageType::PIECE>(payload));

	std::cout << p.index;

	return 0;
}
