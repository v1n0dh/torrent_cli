# BitTorrent Client in Modern C++23

A high-performance, command-line BitTorrent client written in modern C++23, designed to efficiently download files from the BitTorrent network using asynchronous IO and multithreading. This client fully implements the BitTorrent protocol’s core features, including torrent metadata parsing, tracker communication, peer management, and piece-wise file downloading.

## Features

- Full Bencode encoder and decoder for torrent metadata serialization and parsing.
- Asynchronous networking modules for tracker communication and peer-to-peer data exchange using standalone ASIO (non-Boost) library.
- Multi-threaded piece downloader with thread-safe work queues and synchronization mechanisms.
- Prioritized piece selection strategy combined with SHA-1 hash verification for robust data integrity.
- Efficient file mapping and reconstruction of multi-file torrents into their original folder and file structures.
- Timeout handling, retry mechanisms, and error recovery for stable peer connection management.
- Custom thread pool to optimize CPU and IO resource utilization and improve download throughput.
- Designed with modern C++23 features such as concepts, smart pointers, and atomic operations to ensure safe and clean code.

## Requirements

- C++23 compatible compiler (e.g., GCC 12+, Clang 15+, MSVC latest)
- Standalone ASIO library (https://think-async.com)
- OpenSSL library (for SHA-1 hashing)
- JsonCPP, CPR libraries

## Building

```bash
git clone https://github.com/v1n0dh/torrent_cli.git
cd torrent_cli
make
```

## Usage
```bash
Usage:
  torrent_cli [OPTION...]

  -h, --help              Print Usage
  -t, --torrent-file arg  .torrent file input location
  -f, --out-file arg      destination file location
```

## Limitations

- Magnet link support is not implemented at this time; only .torrent files are supported.
- Trackerless DHT peer discovery is currently not supported.
- No upload/seeding functionality yet; the client is focused on downloading only.
- Performance optimizations such as bandwidth throttling or piece rarity algorithms are minimal.

## torrent_cli Demo

[![asciicast](https://asciinema.org/a/pKabIilykwSbznGTo4kdp13rB.svg)](https://asciinema.org/a/pKabIilykwSbznGTo4kdp13rB)
