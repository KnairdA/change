#include "change_tracker.h"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

namespace {

// constants for increasing pair access readability
constexpr unsigned int EMPLACE_SUCCESS = 1;
constexpr unsigned int FILE_PATH       = 0;
constexpr unsigned int FILE_CONTENT    = 1;

boost::process::context getDefaultContext() {
	boost::process::context context;

	context.environment     = boost::process::self::get_environment();
	context.stdout_behavior = boost::process::capture_stream();
	context.stdin_behavior  = boost::process::capture_stream();

	return context;
}

// Build `diff` command to be used to display the tracked changes in a
// human readable fashion.
//
// As the original file contents are read by this library and forwarded
// to `diff` via its standard input the command has to be structured as
// follows:
//
//   diff -u --label $file_path - $file_path
//
std::string getDiffCommand(
	const std::string& diff_cmd, const std::string& file_path) {
	return diff_cmd +  " --label " + file_path + " - " + file_path;
}

void read_file_to_stream(
	const boost::filesystem::path& file_path,
	std::stringstream* const       stream
) {
	try {
		boost::filesystem::ifstream file(file_path);

		if ( file.is_open() ) {
			*stream << file.rdbuf();
			stream->sync();
		}
	} catch ( boost::filesystem::filesystem_error& ) {
		// we catch this exception in case the parent process is
		// performing system calls that are allowed to fail - e.g.
		// in this instance writing to files that we are not allowed
		// to read.
	}
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
		const auto& tracked_path = std::get<FILE_PATH>(tracked);

		if ( boost::filesystem::exists(tracked_path) ) {
			boost::process::child diffProcess{
				boost::process::launch_shell(
					getDiffCommand(this->diff_cmd_, tracked_path),
					getDefaultContext()
				)
			};

			diffProcess.get_stdin() << std::get<FILE_CONTENT>(tracked)->rdbuf();
			diffProcess.get_stdin().close();

			this->logger_->forward(diffProcess.get_stdout());

			diffProcess.wait();
		}
	}
}

bool ChangeTracker::is_tracked(const boost::filesystem::path& file_path) const {
	return this->children_.find(file_path.string()) != this->children_.end();
}

auto ChangeTracker::create_child(const boost::filesystem::path& file_path) {
	std::lock_guard<std::mutex> guard(this->write_mutex_);

	return this->children_.emplace(
		file_path.string(),
		std::make_unique<std::stringstream>()
	);
}

// _Tracking_ consists of adding the full file path to the `children_` map and
// reading the full pre-change file contents into the mapped `std::stringstream`
// instance.
//
// This seems to be the most workable of various tested approaches to providing
// `diff` with proper input in all circumstances. Leaving the intermediate
// preservation of the original file content to `diff` sadly failed randomly as
// it is _too intelligent_ in actually reading the file from permanent storage.
// e.g. it opens all relevant file descriptors at launch and only reads the
// first block of the file contents until more is required. This leads to problems
// when the tracked file is modified after `diff` has been spawned.
//
void ChangeTracker::track(const std::string& file_path) {
	// May throw filesystem exceptions if the given path doesn't exist. We are
	// able to disregard this as this function should only be called when
	// existance is guaranteed. If this is not the case we want the library to
	// fail visibly.
	//
	const auto full_path = boost::filesystem::canonical(file_path);

	if ( !this->is_tracked(full_path) ) {
		auto result = this->create_child(full_path);

		if ( std::get<EMPLACE_SUCCESS>(result) ) {
			read_file_to_stream(
				full_path,
				std::get<FILE_CONTENT>(*result.first).get()
			);
		}
	}
}

}
