#include <stdlib.h>
#include <sys/mman.h>
#include <sys/uio.h>

#include "actual.h"

#include "utility/io.h"
#include "utility/logger.h"

#include "tracking/path_matcher.h"
#include "tracking/change_tracker.h"

// `true`  signals the interposed functions to execute tracking logic
// `false` signals the interposed functions to do plain forwarding
static bool enabled = false;

static std::unique_ptr<utility::FileDescriptorGuard> fd_guard;
static std::unique_ptr<utility::Logger>              logger;
static std::unique_ptr<tracking::PathMatcher>        matcher;
static std::unique_ptr<tracking::ChangeTracker>      tracker;

void initialize() __attribute__ ((constructor));
void initialize() {
	if ( getenv("CHANGE_LOG_TARGET") != NULL ) {
		fd_guard = std::make_unique<utility::FileDescriptorGuard>(
			getenv("CHANGE_LOG_TARGET")
		);
		logger   = std::make_unique<utility::Logger>(*fd_guard);
	} else {
		logger   = std::make_unique<utility::Logger>(STDERR_FILENO);
	}

	if ( getenv("CHANGE_LOG_DIFF_CMD") != NULL ) {
		tracker = std::make_unique<tracking::ChangeTracker>(
			logger.get(), getenv("CHANGE_LOG_DIFF_CMD")
		);
	} else {
		tracker = std::make_unique<tracking::ChangeTracker>(logger.get());
	}

	if ( getenv("CHANGE_LOG_IGNORE_PATTERN_PATH") != NULL ) {
		matcher = std::make_unique<tracking::PathMatcher>(
			getenv("CHANGE_LOG_IGNORE_PATTERN_PATH")
		);
	} else {
		matcher = std::make_unique<tracking::PathMatcher>();
	}

	// tracking is only enabled when everything is initialized as both
	// the actual tracking and the decision if a event should be tracked
	// depend on `logger` and `tracker` being fully instantiated.
	// In most cases this library will work correctly without delayed
	// activation but e.g. `nvim` crashes as it performs tracked syscalls
	// before `init` has been called.
	enabled = true;
}

inline void track_write(const int fd) {
	if ( enabled && fd != *fd_guard && utility::is_regular_file(fd) ) {
		const auto path = utility::get_file_path(fd);

		if ( !matcher->isMatching(path) ) {
			tracker->track(path);
		}
	}
}

inline void track_write(const std::string& path) {
	if ( enabled && utility::is_regular_file(path.c_str()) ) {
		if ( !matcher->isMatching(path) ) {
			tracker->track(path);
		}
	}
}

inline void track_rename(
	const std::string& old_path, const std::string& new_path) {
	if ( enabled ) {
		if ( !matcher->isMatching(old_path) ) {
			tracker->track(old_path);

			if ( !matcher->isMatching(new_path) ) {
				logger->append("renamed '", old_path, "' to '", new_path, "'");
			}
		}
	}
}

inline void track_remove(const std::string& path) {
	if ( enabled && utility::is_regular_file(path.c_str()) ) {
		if ( !matcher->isMatching(path) ) {
			logger->append("removed '", path, "'");
		}
	}
}

extern "C" {

int open(const char* path, int flags, mode_t mode) {
	static actual::ptr<int, const char*, int, mode_t> actual_open{};

	if ( !actual_open ) {
		actual_open = actual::get_ptr<decltype(actual_open)>("open");
	}

	// `open` may reset the file contents when used with the `O_TRUNC` flag.
	// e.g. this is how _emacs_ clears the file content prior to writing the
	// new content.
	// Normally `O_TRUNC` is defined in `fcntl.h` which we can not include
	// as it defines the very c-function we are currently _overriding_.
	//
	if ( flags & 01000 ) {
		track_write(path);
	}

	return actual_open(path, flags, mode);
}

ssize_t write(int fd, const void* buffer, size_t count) {
	static actual::ptr<ssize_t, int, const void*, size_t> actual_write{};

	if ( !actual_write ) {
		actual_write = actual::get_ptr<decltype(actual_write)>("write");
	}

	track_write(fd);

	return actual_write(fd, buffer, count);
}

ssize_t writev(int fd, const iovec* iov, int iovcnt) {
	static actual::ptr<ssize_t, int, const iovec*, int> actual_writev{};

	if ( !actual_writev ) {
		actual_writev = actual::get_ptr<decltype(actual_writev)>("writev");
	}

	track_write(fd);

	return actual_writev(fd, iov, iovcnt);
}

void* mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset) {
	static actual::ptr<void*, void*, size_t, int, int, int, off_t> actual_mmap{};

	if ( !actual_mmap ) {
		actual_mmap = actual::get_ptr<decltype(actual_mmap)>("mmap");
	}

	if ( prot & PROT_WRITE ) {
		track_write(fd);
	}

	return actual_mmap(addr, length, prot, flags, fd, offset);
}

int rename(const char* old_path, const char* new_path) {
	static actual::ptr<int, const char*, const char*> actual_rename{};

	if ( !actual_rename ) {
		actual_rename = actual::get_ptr<decltype(actual_rename)>("rename");
	}

	track_rename(old_path, new_path);

	return actual_rename(old_path, new_path);
}

int rmdir(const char* path) {
	static actual::ptr<int, const char*> actual_rmdir{};

	if ( !actual_rmdir ) {
		actual_rmdir = actual::get_ptr<decltype(actual_rmdir)>("rmdir");
	}

	track_remove(path);

	return actual_rmdir(path);
}

int unlink(const char* path) {
	static actual::ptr<int, const char*> actual_unlink{};

	if ( !actual_unlink ) {
		actual_unlink = actual::get_ptr<decltype(actual_unlink)>("unlink");
	}

	track_remove(path);

	return actual_unlink(path);
}

int unlinkat(int dirfd, const char* path, int flags) {
	static actual::ptr<int, int, const char*, int> actual_unlinkat{};

	if ( !actual_unlinkat ) {
		actual_unlinkat = actual::get_ptr<decltype(actual_unlinkat)>("unlinkat");
	}

	// Normally `AT_FDCWD` is defined in `fcntl.h` which we can not include
	// as it defines `open` which this library also aims to override.
	//
	if ( dirfd == -100 ) {
		track_remove(path);
	} else {
		track_remove(utility::get_file_path(dirfd) + path);
	}

	return actual_unlinkat(dirfd, path, flags);
}

}
