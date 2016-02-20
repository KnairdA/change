#ifndef CHANGE_SRC_LOGGER_H_
#define CHANGE_SRC_LOGGER_H_

#include <mutex>

#include <ext/stdio_filebuf.h>

#include <boost/filesystem.hpp>
#include <boost/process/pistream.hpp>

namespace utility {

class Logger {
	public:
		Logger(const int target_fd):
			buffer_(target_fd, std::ios::out),
			stream_(&this->buffer_) { }

		void forward(boost::process::pistream&);

		template <typename... Arguments>
		void append(Arguments&&... args) {
			std::lock_guard<std::mutex> guard(this->write_mutex_);

			this->append_to_stream(std::forward<Arguments>(args)...);
		}

	private:
		__gnu_cxx::stdio_filebuf<char> buffer_;
		std::ostream                   stream_;
		std::mutex                     write_mutex_;

		template <typename Head>
		inline void append_to_stream(Head&& head) {
			this->stream_ << head << std::endl;
		}

		template <typename Head, typename... Tail>
		inline void append_to_stream(Head&& head, Tail&&... tail) {
			this->stream_ << head;

			this->append_to_stream(std::forward<Tail>(tail)...);
		}

};

}

#endif  // CHANGE_SRC_LOGGER_H_
