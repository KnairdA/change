#include "logger.h"

namespace utility {

// Forward the contents of a given standard output stream to the log target
//
// While `this->stream_ << stream.rdbuf()` would be more effective it sadly
// does not work with `boost::process::pistream` due to a broken pipe error
// in conjunction with the required `boost::process::capture_stream` context
// flag.
//
void Logger::forward(boost::process::pistream& stream) {
	std::lock_guard<std::mutex> guard(this->write_mutex_);

	this->stream_ << std::string(
		(std::istreambuf_iterator<char>(stream)),
		(std::istreambuf_iterator<char>())
	);
}

}
