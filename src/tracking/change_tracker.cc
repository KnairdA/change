#include "change_tracker.h"

#include <boost/optional.hpp>
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

bool ChangeTracker::is_tracked(const std::string& file_path) const {
	return this->children_.find(
		boost::filesystem::canonical(file_path).string()
	) != this->children_.end();
}

auto ChangeTracker::create_child(const std::string& file_path) {
	std::lock_guard<std::mutex> guard(this->write_mutex_);

	return this->children_.emplace(
		boost::filesystem::canonical(file_path).string(),
		std::make_unique<std::stringstream>()
	);
}

// Begins tracking changes to a file reachable by a given path
//
// _Tracking_ consists of adding the full file path to the `children_`
// map and reading the full pre-change file contents into the mapped
// `std::stringstream` instance.
//
// This seems to be the most workable of various tested approaches
// to providing `diff` with proper input in all circumstances. Leaving
// the intermediate preservation of the original file content to
// `diff` sadly failed randomly as it is _too intelligent_ in actually
// reading the file from permanent storage. e.g. it opens all relevant
// file descriptors at launch and only reads the first block of the
// file contents until more is required. This leads to problems when
// the tracked file is modified after `diff` has been spawned.
//
bool ChangeTracker::track(const std::string& file_path) {
	auto result = this->create_child(file_path);

	if ( std::get<EMPLACE_SUCCESS>(result) ) {
		boost::filesystem::ifstream file(
			std::get<FILE_PATH>(*result.first)
		);

		if ( file.is_open() ) {
			*std::get<FILE_CONTENT>(*result.first) << file.rdbuf();
			std::get<FILE_CONTENT>(*result.first)->sync();
		}

		return true;
	} else {
		return false;
	}
}

}
