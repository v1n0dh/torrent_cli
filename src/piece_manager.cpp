#include <cmath>
#include <cstdint>
#include <openssl/sha.h>
#include <sys/types.h>
#include <thread>

#include "../include/message.hpp"
#include "../include/piece_manager.hpp"
#include "../include/utils.hpp"

bool Piece_Manager::download_piece(Piece_Work& piece_work, Peer& peer, File_Mapper& f_mapper, size_t piece_size) {
	if (peer._bitfield.is_bitfield_set() && !peer.piece_available(piece_work.index)) {
		*pw_queue << piece_work;
		return false;
	}

	Piece p(piece_work.index, piece_work.piece_len);
	int curr_offset = piece_work.offset;
	int backlog_requests = 0;

	while (!p.is_completed()) {
		if (peer.status == PEER_DISCONNECTED)
			break;

		if (peer.status == PEER_UNCHOKED) {
			while (backlog_requests <= MAX_REQUESTS && !p.is_completed() && curr_offset != -1) {
				// If Block is already recieved, continue for the next block
				if (p.blocks[curr_offset]->status == BLOCK_RECEIVED) {
					curr_offset = p.next_block();
					continue;
				}
				std::vector<uint8_t> rqst_payload = Message::create_payload<MessageType::REQUEST>(
						piece_work.index,
						(curr_offset * BLOCK_SIZE),
						BLOCK_SIZE);
				Message rqst_msg(MessageType::REQUEST, rqst_payload);

				if (!peer.send_message(rqst_msg)) {
					piece_work.offset = curr_offset;
					*pw_queue << piece_work;
					return false;
				}

				p.blocks[curr_offset]->status = BLOCK_REQUESTED;
				curr_offset = p.next_block();
				backlog_requests++;
			}
		}

		Message msg = peer.recv_message();
		if (peer.status == PEER_KEEP_ALIVE) continue;
		if (peer.status == PEER_DISCONNECTED) break;

		switch (msg.id) {
			case MessageType::CHOKE:
				peer.status = PEER_CHOKED;
				break;
			case MessageType::UNCHOKE:
				peer.status = PEER_UNCHOKED;
				break;
			case MessageType::BITFIELD:
				peer._bitfield.set(msg.payload);
				break;
			case MessageType::HAVE: {
				Have_Payload p = std::get<Have_Payload>(Message::parse_payload<MessageType::HAVE>(msg.payload));
				if (!peer._bitfield.is_bitfield_set()) peer._bitfield = Bitfield(this->total_piece_count);
				peer._bitfield.set_piece(p.piece_index);
				break;
			}
			case MessageType::PIECE: {
				Piece_Payload payload =
					std::get<Piece_Payload>(Message::parse_payload<MessageType::PIECE>(msg.payload));

				int block_idx = payload.begin / BLOCK_SIZE;
				if (block_idx < 0 || block_idx > p.num_blocks)
					break;

				if (p.blocks[block_idx]->status == BLOCK_RECEIVED)
					break;

				p.blocks[block_idx]->data = payload.data;
				p.blocks[block_idx]->offset = (payload.begin / BLOCK_SIZE);
				p.blocks[block_idx]->status = BLOCK_RECEIVED;

				backlog_requests--;
			}
		}
	}

	if (!p.is_completed()) {
		*pw_queue << piece_work;
		return false;
	}

	if (!check_piece_hash(p, piece_work.piece_hash)) {
		std::cerr << "PIECE Hash doesn't match Piece: " << p.index << "\n";
		*pw_queue << piece_work;
		return false;
	}

	f_mapper.wite_piece(p, piece_size);

	std::cout << "Piece: " << p.index << " downloaded from " << peer.ip_address
			  << " by thread " << std::this_thread::get_id() << " \n";

	return true;
}

bool Piece_Manager::check_piece_hash(const Piece& p, const std::vector<uint8_t>& hash) {
	std::vector<uint8_t> p_data;

	unsigned char raw_hash[SHA_DIGEST_LENGTH];
	std::vector<uint8_t> final_hash;

	for (Block* block : p.blocks)
		p_data.insert(p_data.end(), block->data.begin(), block->data.end());

	SHA1(p_data.data(), p_data.size(), raw_hash);

	for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
		final_hash.push_back(static_cast<uint8_t>(raw_hash[i]));

	return hex_bytes_to_string(final_hash) == hex_bytes_to_string(hash) ? true : false;
}


Piece::Piece(int index, int piece_len) : index(index), piece_len(piece_len) {
	num_blocks = std::ceil(piece_len / BLOCK_SIZE);

	blocks.resize(num_blocks);
	int block_len = BLOCK_SIZE;

	for (int i = 0; i < num_blocks; i++) {
		if (i == num_blocks - 1 && BLOCK_SIZE % piece_len != 0) block_len = BLOCK_SIZE % piece_len;
		blocks[i] = new Block(index, i, block_len);
	}
}

Piece::~Piece() {
	for (int i = 0; i < num_blocks; i++)
		delete blocks[i];
}

int Piece::next_block() {
	for (int i = 0; i < this->num_blocks; i++) {
		if (this->blocks[i]->status == BLOCK_PENDING)
			return i;
	}
	return -1;
}

bool Piece::is_completed() {
	for (Block* block : this->blocks)
		if (block->status != BLOCK_RECEIVED) return false;

	return true;
}
