#ifndef CHANGE_SRC_UTILITY_H_
#define CHANGE_SRC_UTILITY_H_

#include <ext/stdio_filebuf.h>

#include <boost/process.hpp>

#include <iostream>

namespace utility {

class Logger {
	public:
		Logger(const int target_fd):
			buffer_(target_fd, std::ios::out),
			stream_(&this->buffer) { }

		void append(const std::string& msg) {
			this->stream_ << msg << std::endl;
		}

		// Forward the contents of a given standard output stream to the log target
		//
		// While `this->stream_ << stream.rdbuf()` would be more effective it sadly
		// does not work with `boost::process::pistream` due to a broken pipe error
		// in conjunction with the required `boost::process::capture_stream` context
		// flag.
		//
		void forward(boost::process::pistream& stream) {
			this->stream << std::string(
				(std::istreambuf_iterator<char>(stream)),
				(std::istreambuf_iterator<char>())
			);
		}

	private:
		__gnu_cxx::stdio_filebuf<char> buffer_;
		std::ostream                   stream_;

};

}

#endif  // CHANGE_SRC_UTILITY_H_
