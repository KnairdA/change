#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <algorithm>
#include <functional>

#include "actual_function.h"

#include "utility/io.h"
#include "utility/logger.h"
#include "tracking/change_tracker.h"

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

	tracker = std::make_unique<tracking::ChangeTracker>(logger.get());
}

void exit(int status) {
	__attribute__((__noreturn__)) void(*actual_exit)(int) = nullptr;
	const void* symbol_address = dlsym(RTLD_NEXT, "exit");

	std::memcpy(&actual_exit, &symbol_address, sizeof(symbol_address));

	logger->append("exit");

	actual_exit(status);
}

ssize_t write(int fd, const void* buffer, size_t count) {
	if ( utility::is_regular_file(fd) ) {
		const std::string file_name{ utility::get_file_name(fd) };

		if ( !tracker->is_tracked(file_name) ) {
			tracker->track(file_name);
		}
	}

	return actual::write(fd, buffer, count);
}

int rename(const char* old_path, const char* new_path) {
	if ( !tracker->is_tracked(old_path) ) {
		tracker->track(old_path);
	}

	logger->append("renamed '" + std::string(old_path) + "' to '" + std::string(new_path) + "'");

	return actual::rename(old_path, new_path);
}

int rmdir(const char* path) {
	logger->append("removed directory '" + std::string(path) + "'");

	return actual::rmdir(path);
}

int unlink(const char* path) {
	if ( utility::is_regular_file(path) ) {
		logger->append("rm '" + std::string(path) + "'");
	}

	return actual::unlink(path);
}

int unlinkat(int dirfd, const char* path, int flags) {
	if ( dirfd == AT_FDCWD ) {
		logger->append("removed '" + std::string(path) + "'");
	} else {
		logger->append("removed '" + utility::get_file_name(dirfd) + path + "'");
	}

	return actual::unlinkat(dirfd, path, flags);
}

void* mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset) {
	if ( ( prot & PROT_WRITE ) && utility::is_regular_file(fd) ) {
		const std::string file_name{ utility::get_file_name(fd) };

		if ( !tracker->is_tracked(file_name) ) {
			tracker->track(file_name);
		}
	}

	return actual::mmap(addr, length, prot, flags, fd, offset);
}
