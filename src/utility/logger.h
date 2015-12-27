#ifndef CHANGE_SRC_LOGGER_H_
#define CHANGE_SRC_LOGGER_H_

#include <mutex>

#include <ext/stdio_filebuf.h>

#include <boost/process.hpp>

namespace utility {

class Logger {
	public:
		Logger(const int target_fd):
			buffer_(target_fd, std::ios::out),
			stream_(&this->buffer_) { }

		void append(const std::string& msg) {
			std::lock_guard<std::mutex> guard(this->write_mutex_);

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
			std::lock_guard<std::mutex> guard(this->write_mutex_);

			this->stream_ << std::string(
				(std::istreambuf_iterator<char>(stream)),
				(std::istreambuf_iterator<char>())
			);
		}

	private:
		__gnu_cxx::stdio_filebuf<char> buffer_;
		std::ostream                   stream_;
		std::mutex                     write_mutex_;

};

}

#endif  // CHANGE_SRC_LOGGER_H_
