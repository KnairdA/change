#include "change_tracker.h"

#include <boost/optional.hpp>
#include <boost/filesystem/fstream.hpp>

namespace {

// constants for increasing pair access readability
constexpr unsigned int EMPLACE_SUCCESS = 1;
constexpr unsigned int FILE_NAME       = 0;
constexpr unsigned int DIFF_PROCESS    = 1;

boost::process::context getDefaultContext() {
	boost::process::context context;

	context.environment     = boost::process::self::get_environment();
	context.stdout_behavior = boost::process::capture_stream();
	context.stdin_behavior  = boost::process::capture_stream();

	return context;
}

std::string getDiffCommand(
	const std::string& diff_cmd, const std::string& full_path) {
	return diff_cmd +  " --label " + full_path + " - " + full_path;
}

}

namespace tracking {

ChangeTracker::ChangeTracker(utility::Logger* logger, const std::string& diff_cmd):
	diff_cmd_(diff_cmd),
	logger_(logger),
	children_() { }

ChangeTracker::ChangeTracker(utility::Logger* logger):
	ChangeTracker(logger, "diff -u") { }

ChangeTracker::~ChangeTracker() {
	for ( auto&& tracked : this->children_ ) {
		std::get<DIFF_PROCESS>(tracked)->get_stdin().close();

		this->logger_->forward(std::get<DIFF_PROCESS>(tracked)->get_stdout());

		std::get<DIFF_PROCESS>(tracked)->wait();
	}
}

bool ChangeTracker::is_tracked(const std::string& file_path) const {
	return this->children_.find(
		boost::filesystem::canonical(file_path).string()
	) != this->children_.end();
}

// Begins tracking changes to a file reachable by a given path
//
// The actual tracking is performed by a `diff` instance that is
// spawned by this method and managed by this class.
// As `diff` is called as a new subprocess and because there is no
// straight forward way for checking / enforcing if it has already
// completed reading the initial contents of the file the command
// is structured in the following sequence:
//
// diff -p - $file_path
//
// This means that reading the final file contents is delegated to
// `diff` while the initial file contents are read by this method
// and written to `diff`'s standard input.
//
// If the `-` and `$file_path` arguments were exchanged there would
// be no way to:
//  - be sure the initial contents are read before the changing
//    syscall that triggered this tracking in the first place is
//    performed
//  - be sure that the initial file contents are read completly
//    as `diff` seemingly only reads the first block of the first
//    file provided if the second file argument is standard input
//
bool ChangeTracker::track(const std::string& file_path) {
	const std::string full_file_path{
		boost::filesystem::canonical(file_path).string()
	};

	std::unique_lock<std::mutex> guard(this->write_mutex_);

	auto result = this->children_.emplace(
		full_file_path,
		std::make_unique<boost::process::child>(
			boost::process::launch_shell(
				getDiffCommand(this->diff_cmd_, full_file_path),
				getDefaultContext()
			)
		)
	);

	guard.unlock();

	if ( std::get<EMPLACE_SUCCESS>(result) ) {
		boost::filesystem::ifstream file(
			std::get<FILE_NAME>(*result.first)
		);

		if ( file.is_open() ) {
			std::get<DIFF_PROCESS>(*result.first)->get_stdin() << file.rdbuf();
		}

		return true;
	} else {
		return false;
	}
}

}
