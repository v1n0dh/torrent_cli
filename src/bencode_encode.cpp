#include <sstream>

#include "../include/bencode_parser.hpp"

std::string bencode_encode_int(Json::Value bencode)
{
	std::stringstream ss;
	ss << "i";
	ss << bencode.asUInt64();
	ss << "e";

	return ss.str();
}

std::string bencode_encode_str(Json::Value bencode)
{
	std::stringstream ss;
	ss << bencode.asString().length();
	ss << ":";
	ss << bencode.asString();

	return ss.str();
}

std::string bencode_encode_list(Json::Value bencode)
{
	std::stringstream ss;
	ss << "l";
	for (Json::Value::const_iterator it = bencode.begin(); it != bencode.end(); it++) {
		ss << bencode_encode(*it);
	}
	ss << "e";

	return ss.str();
}

std::string bencode_encode_dict(Json::Value bencode)
{
	std::stringstream ss;
	ss << "d";
	for (Json::Value::const_iterator it = bencode.begin(); it != bencode.end(); it++) {
		ss << bencode_encode_str(it.key());
		ss << bencode_encode(*it);
	}
	ss << "e";

	return ss.str();
}

std::string bencode_encode(Json::Value bencode)
{
	if (bencode.isUInt64()) return bencode_encode_int(bencode);
	if (bencode.isString()) return bencode_encode_str(bencode);
	if (bencode.isArray()) return bencode_encode_list(bencode);
	return bencode_encode_dict(bencode);
}
