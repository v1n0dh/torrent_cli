#include "../include/torrent_client.hpp"

int main() {
	Torrent_File file("debian.torrent");

	Torrent_Client client(std::move(file));
	client.start_io_ctx();
	client.calculate_pieces();
	client.pre_allocate_file();
	client.download_file();

	return 0;
}
