#ifndef CHANGE_UTILITY_H_
#define CHANGE_UTILITY_H_

#include <ext/stdio_filebuf.h>

#include <iostream>

namespace utility {

class Logger {
	public:
		Logger(const int target_fd):
			buffer(target_fd, std::ios::out),
			stream(&this->buffer) { }

		void append(const std::string& msg) {
			this->stream << msg << std::endl;
		}

	private:
		__gnu_cxx::stdio_filebuf<char> buffer;
		std::ostream                   stream;
};

}

#endif  // CHANGE_UTILITY_H_
