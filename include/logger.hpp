#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <ctime>
#include <iostream>

enum Log_Level {
	LOG_INFO,
	LOG_WARN,
	LOG_ERROR
};

#define LOG_LEVEL_TO_STR(level) \
	((level) == LOG_INFO ? "INFO" :    \
		(level) == LOG_WARN ? "WARN" : \
		(level) == LOG_ERROR ? "ERROR" : "UNKNOWN")

class Logger {
public:
	Logger(std::ostream& out_stream = std::cout) : _out_stream(out_stream) {}

	Logger& operator<<(Log_Level level) {
		this->_level = level;
		return *this;
	}

	template <typename T>
	Logger& operator<<(const T& value) {
		if (_prefix_pos) {
			time_t curr_time = std::time(nullptr);
			char buff[64];
			std::strftime(buff, sizeof(buff), "%Y-%m-%d %H:%M:%S", std::localtime(&curr_time));
			_out_stream << "[" << buff << "] [" << LOG_LEVEL_TO_STR(_level) << "] ";
			_prefix_pos = false;
		}

		_out_stream << value;

		if constexpr(std::is_same_v<T, const char*>) {
			if (std::string(value).back() == '\n')
				_prefix_pos = true;
		}

		return *this;
	}

	Logger& operator<<(std::ostream& (*manip)(std::ostream&)) {
		_out_stream << manip;

		if (manip == static_cast<std::ostream& (*)(std::ostream&)>(std::endl) ||
			manip == static_cast<std::ostream& (*)(std::ostream&)>(std::flush))
			_prefix_pos = true;

		return *this;
	}

private:
	std::ostream& _out_stream;
	Log_Level _level = LOG_INFO;
	static thread_local bool _prefix_pos;
};

inline thread_local bool Logger::_prefix_pos = true;

#endif
