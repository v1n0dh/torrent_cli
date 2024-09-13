#include "../include/bencode_parser.hpp"

int bencode_decode_int(const std::string &torr_str, int &cnt);
std::string bencode_decode_str(const std::string &torr_str, int &cnt);
Json::Value bencode_decode(const std::string &torr_str, int &cnt);
Json::Value bencode_decode_list(const std::string &torr_str, int &cnt);
Json::Value bencode_decode_dict(const std::string &torr_str, int &cnt);

int bencode_decode_int(const std::string &torr_str, int &cnt)
{
	auto p = torr_str.find(BENCODE_END_DATA);
	std::string c_num = torr_str.substr(1, p-1);
	int value = std::stoi(c_num);
	cnt += c_num.length() + 2;
	return value;
}

std::string bencode_decode_str(const std::string &torr_str, int &cnt)
{
	auto p = torr_str.find(BENCODE_BEGIN_STRING);
	std::string c_len = torr_str.substr(0, p);
	int len = std::stoi(c_len);
	std::string value = torr_str.substr(c_len.length() + 1, len);
	cnt += c_len.length() + 1 + len;
	return value;
}

Json::Value bencode_decode_list(const std::string &torr_str, int &cnt)
{
	int i = 0;
	Json::Value list(Json::arrayValue);
	while (i < torr_str.length()) {
		if (torr_str[i] == BENCODE_END_DATA) {
			cnt = i+2;
			return list;
		}
		int k = 0;
		Json::Value elem = bencode_decode(torr_str.substr(i), k);
		list.append(elem);
		i += k;
	}
	return list;
}

Json::Value bencode_decode_dict(const std::string &torr_str, int &cnt)
{
	int i = 0;
	Json::Value dict;
	while (i < torr_str.length()) {
		if (torr_str[i] == BENCODE_END_DATA) {
			cnt = i+2;
			return dict;
		}
		int k = 0;
		std::string key = bencode_decode_str(torr_str.substr(i), k);
		int j = i+k;
		k = 0;
		dict[key] = bencode_decode(torr_str.substr(j), k);
		i = j+k;
	}
	return dict;
}

Json::Value bencode_decode(const std::string &torr_str, int &cnt)
{
	if (torr_str[0] == BENCODE_BEGIN_INT) return Json::Value(bencode_decode_int(torr_str, cnt));
	if (torr_str[0] == BENCODE_BEGIN_LIST) return bencode_decode_list(torr_str.substr(1), cnt);
	if (torr_str[0] == BENCODE_BEGIN_DICT) return bencode_decode_dict(torr_str.substr(1), cnt);
	return Json::Value(bencode_decode_str(torr_str, cnt));
}

int bencode_decode_int(const std::string &torr_str)
{
	int k = 0;
	return bencode_decode_int(torr_str, k);
}

std::string bencode_decode_str(const std::string &torr_str)
{
	int k = 0;
	return bencode_decode_str(torr_str, k);
}

Json::Value bencode_decode_list(const std::string &torr_str)
{
	int k = 0;
	return bencode_decode_list(torr_str, k);
}

Json::Value bencode_decode_dict(const std::string &torr_str)
{
	int k = 0;
	return bencode_decode_dict(torr_str, k);
}

Json::Value bencode_decode(const std::string &torr_str)
{
	int k = 0;
	return bencode_decode(torr_str, k);
}
