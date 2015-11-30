#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <algorithm>
#include <functional>
#include <unordered_set>

#include "io.h"
#include "utility.h"
#include "actual_function.h"

static std::unique_ptr<std::unordered_set<std::string>> tracked_files;
static std::unique_ptr<io::FileDescriptorGuard>         fd_guard;
static std::unique_ptr<utility::Logger>                 logger;

void init() __attribute__ ((constructor));
void init() {
	tracked_files = std::make_unique<std::unordered_set<std::string>>();

	if ( getenv("CHANGE_LOG_TARGET") != NULL ) {
		fd_guard = std::make_unique<io::FileDescriptorGuard>(
			getenv("CHANGE_LOG_TARGET")
		);
		logger   = std::make_unique<utility::Logger>(*fd_guard);
	} else {
		logger   = std::make_unique<utility::Logger>(STDERR_FILENO);
	}
}

void exit(int status) {
	__attribute__((__noreturn__)) void(*real_exit)(int) = nullptr;
	const void* symbol_address = dlsym(RTLD_NEXT, "exit");

	std::memcpy(&real_exit, &symbol_address, sizeof(symbol_address));

	logger->append("exit");

	real_exit(status);
}

bool is_tracked_file(const std::string& file_name) {
	return tracked_files->find(file_name) != tracked_files->end();
}

bool track_file(const std::string& file_name) {
	return tracked_files->emplace(file_name).second;
}

ssize_t write(int fd, const void* buffer, size_t count) {
	if ( io::is_regular_file(fd) ) {
		const std::string file_name{ io::get_file_name(fd) };

		if ( !is_tracked_file(file_name) ) {
			track_file(file_name);

			logger->append("wrote to '" + file_name + "'");
		}
	}

	return actual::write(fd, buffer, count);
}

int rename(const char* old_path, const char* new_path) {
	if ( !is_tracked_file(old_path) ) {
		track_file(old_path);
	}

	logger->append("renamed '" + std::string(old_path) + "' to '" + std::string(new_path) + "'");

	return actual::rename(old_path, new_path);
}

int rmdir(const char* path) {
	logger->append("removed directory '" + std::string(path) + "'");

	return actual::rmdir(path);
}

int unlink(const char* path) {
	logger->append("removed '" + std::string(path) + "'");

	return actual::unlink(path);
}

int unlinkat(int dirfd, const char* path, int flags) {
	if ( dirfd == AT_FDCWD ) {
		logger->append("removed '" + std::string(path) + "'");
	} else {
		logger->append("removed '" + io::get_file_name(dirfd) + path + "'");
	}

	return actual::unlinkat(dirfd, path, flags);
}

void* mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset) {
	if ( ( prot & PROT_WRITE ) && io::is_regular_file(fd) ) {
		const std::string file_name{ io::get_file_name(fd) };

		if ( !is_tracked_file(file_name) ) {
			track_file(file_name);

			logger->append("mmap '" + file_name + "'");
		}
	}

	return actual::mmap(addr, length, prot, flags, fd, offset);
}
