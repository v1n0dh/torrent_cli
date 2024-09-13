#ifndef _BENCODE_PARSER_
#define _BENCODE_PARSER_

#include <iostream>
#include <json/json.h>

static const char BENCODE_BEGIN_DICT = 'd';
static const char BENCODE_BEGIN_LIST = 'l';
static const char BENCODE_BEGIN_INT = 'i';
static const char BENCODE_BEGIN_STRING = ':';
static const char BENCODE_END_DATA = 'e';

int bencode_decode_int(const std::string &torr_str);
std::string bencode_decode_str(const std::string &torr_str);
Json::Value bencode_decode(const std::string &torr_str);
Json::Value bencode_decode_list(const std::string &torr_str);
Json::Value bencode_decode_dict(const std::string &torr_str);
Json::Value bencode_decode(const std::string &torr_str);


std::string bencode_encode_int(Json::Value bencode);
std::string bencode_encode_str(Json::Value bencode);
std::string bencode_encode_list(Json::Value bencode);
std::string bencode_encode_dict(Json::Value bencode);
std::string bencode_encode(Json::Value bencode);

#endif
