#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "actual_function.h"

#include "utility/io.h"
#include "utility/logger.h"
#include "tracking/change_tracker.h"

// `true`  signals the interposed functions to execute tracking logic
// `false` signals the interposed functions to do plain forwarding
static bool enabled = false;

static std::unique_ptr<utility::FileDescriptorGuard> fd_guard;
static std::unique_ptr<utility::Logger>              logger;
static std::unique_ptr<tracking::ChangeTracker>      tracker;

void init() __attribute__ ((constructor));
void init() {
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
		const std::string file_name{ utility::get_file_name(fd) };

		if ( !tracker->is_tracked(file_name) ) {
			tracker->track(file_name);
		}
	}
}

inline void track_rename(
	const std::string& old_path, const std::string& new_path) {
	if ( enabled ) {
		if ( !tracker->is_tracked(old_path) ) {
			tracker->track(old_path);
		}

		logger->append("renamed '", old_path, "' to '", new_path, "'");
	}
}

inline void track_remove(const std::string& path) {
	if ( enabled && utility::is_regular_file(path.c_str()) ) {
		logger->append("removed '", path, "'");
	}
}

ssize_t write(int fd, const void* buffer, size_t count) {
	track_write(fd);

	return actual::write(fd, buffer, count);
}

ssize_t writev(int fd, const iovec* iov, int iovcnt) {
	track_write(fd);

	return actual::writev(fd, iov, iovcnt);
}

int rename(const char* old_path, const char* new_path) {
	track_rename(old_path, new_path);

	return actual::rename(old_path, new_path);
}

int rmdir(const char* path) {
	track_remove(path);

	return actual::rmdir(path);
}

int unlink(const char* path) {
	track_remove(path);

	return actual::unlink(path);
}

int unlinkat(int dirfd, const char* path, int flags) {
	if ( dirfd == AT_FDCWD ) {
		track_remove(path);
	} else {
		track_remove(utility::get_file_name(dirfd) + path);
	}

	return actual::unlinkat(dirfd, path, flags);
}

void* mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset) {
	if ( prot & PROT_WRITE ) {
		track_write(fd);
	}

	return actual::mmap(addr, length, prot, flags, fd, offset);
}
