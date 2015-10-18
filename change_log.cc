#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <memory>
#include <functional>
#include <unordered_set>

#include "io.h"
#include "utility.h"

static std::unique_ptr<std::unordered_set<std::string>> tracked_files;
static std::unique_ptr<io::FileDescriptorGuard>         fd_guard;
static std::unique_ptr<utility::Logger>                 log;

template <class Result, typename... Arguments>
std::function<Result(Arguments...)> get_real_function(
	const std::string& symbol_name) {

	Result (*real_function)(Arguments...) = nullptr;
	const void* symbol_address = dlsym(RTLD_NEXT, symbol_name.c_str());

	std::memcpy(&real_function, &symbol_address, sizeof(symbol_address));

	return std::function<Result(Arguments...)>(real_function);
}

void init() __attribute__ ((constructor));
void init() {
	tracked_files = std::make_unique<std::unordered_set<std::string>>();

	if ( getenv("CHANGE_LOG_TARGET") != NULL ) {
		fd_guard = std::make_unique<io::FileDescriptorGuard>(
			getenv("CHANGE_LOG_TARGET")
		);
		log      = std::make_unique<utility::Logger>(*fd_guard);
	} else {
		log      = std::make_unique<utility::Logger>(STDERR_FILENO);
	}
}

void exit(int status) {
	get_real_function<void, int>("exit")(status);
}

bool is_tracked_file(const std::string& file_name) {
	return tracked_files->find(file_name) != tracked_files->end();
}

bool track_file(const std::string& file_name) {
	return tracked_files->emplace(file_name).second;
}

ssize_t write(int fd, const void* buffer, size_t count) {
	static auto real_write = get_real_function<ssize_t, int, const void*, size_t>("write");

	if ( io::is_regular_file(fd) ) {
		const std::string file_name{ io::get_file_name(fd) };

		if ( !is_tracked_file(file_name) ) {
			track_file(file_name);

			log->append("wrote to '" + file_name + "'");
		}
	}

	return real_write(fd, buffer, count);
}

int rename(const char* old_path, const char* new_path) {
	static auto real_rename = get_real_function<int, const char*, const char*>("rename");

	log->append("renamed '" + std::string(old_path) + "' to '" + std::string(new_path) + "'");

	return real_rename(old_path, new_path);
}

int rmdir(const char* path) {
	static auto real_rmdir = get_real_function<int, const char*>("rmdir");

	log->append("removed directory '" + std::string(path) + "'");

	return real_rmdir(path);
}

int unlink(const char* path) {
	static auto real_unlink = get_real_function<int, const char*>("unlink");

	log->append("removed '" + std::string(path) + "'");

	return real_unlink(path);
}

int unlinkat(int dirfd, const char* path, int flags) {
	static auto real_unlinkat = get_real_function<int, int, const char*, int>("unlinkat");

	if ( dirfd == AT_FDCWD ) {
		log->append("removed '" + std::string(path) + "'");
	} else {
		log->append("removed '" + io::get_file_name(dirfd) + path + "'");
	}

	return real_unlinkat(dirfd, path, flags);
}
