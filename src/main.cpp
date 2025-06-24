#include <cstdlib>
#include <cxxopts.hpp>

#include "../include/torrent_client.hpp"
#include "../include/logger.hpp"

int main(int argc, char** argv) {
	Logger log(std::cout);

	cxxopts::Options options("torrent_cli", "Example Usage of torrent_cli command");
	options.add_options()
		("h,help", "Print Usage")
		("t,torrent-file", ".torrent file input location", cxxopts::value<std::string>())
		("f,out-file", "destination file location", cxxopts::value<std::string>());

	auto result = options.parse(argc, argv);

	if (result.count("help")) {
		std::cout << options.help() << "\n";
		return 0;
	}

	if (!result.count("torrent-file")) {
		log << LOG_ERROR << "torrent_cli -t <.torrent file>\n";
		std::cout << options.help() << "\n";
		exit(EXIT_FAILURE);
	}

	std::string out_file_path;
	if (result.count("out-file"))
		out_file_path = result["out-file"].as<std::string>();

	Torrent_File file(result["torrent-file"].as<std::string>());

	Torrent_Client client(std::move(file));
	client.start_io_ctx();
	client.calculate_pieces();
	client.pre_allocate_file(out_file_path);
	client.download_file(out_file_path);
	client.wait_for_download(out_file_path);

	return 0;
}
