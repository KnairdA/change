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

		template <typename Head>
		inline void append(Head&& head) {
			this->stream_ << head << std::endl;
		}

		template <typename Head, typename... Tail>
		inline void append(Head&& head, Tail&&... tail) {
			this->stream_ << head;

			this->append(std::forward<Tail>(tail)...);
		}

		void forward(boost::process::pistream&);

	private:
		__gnu_cxx::stdio_filebuf<char> buffer_;
		std::ostream                   stream_;
		std::mutex                     write_mutex_;

};

}

#endif  // CHANGE_SRC_LOGGER_H_
