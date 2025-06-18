#include <cstdlib>

#include "../include/torrent_client.hpp"

int main(int argc, char** argv) {
	if (argc < 2) {
		std::cerr << "Usage: ";
		std::cerr << argv[0] << " file.torrent\n\n";
		exit(1);
	}

	std::string torrent_file(argv[1]);

	Torrent_File file(torrent_file);

	Torrent_Client client(std::move(file));
	client.start_io_ctx();
	client.calculate_pieces();
	client.pre_allocate_file();
	client.download_file();
	client.wait_for_download();

	return 0;
}
