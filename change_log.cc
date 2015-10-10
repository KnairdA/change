#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <unistd.h>
#include <dlfcn.h>
#include <sys/stat.h>

#include <cstring>
#include <iostream>
#include <functional>

template <class Result, typename... Arguments>
std::function<Result(Arguments...)> get_real_function(
	const std::string& symbol_name) {

	Result (*real_function)(Arguments...) = nullptr;
	const void* symbol_address = dlsym(RTLD_NEXT, symbol_name.c_str());

	std::memcpy(&real_function, &symbol_address, sizeof(symbol_address));

	return std::function<Result(Arguments...)>(real_function);
}

std::string get_file_name(int fd) {
	char proc_link[20];
	char file_name[256];

	snprintf(proc_link, sizeof(proc_link), "/proc/self/fd/%d", fd);
	const ssize_t name_size = readlink(proc_link, file_name, sizeof(file_name));

	if ( name_size > 0 ) {
		file_name[name_size] = '\0';

		return std::string(file_name);
	} else {
		return std::string();
	}
}

bool is_regular_file(int fd) {
	struct stat fd_stat;
	fstat(fd, &fd_stat);

	return S_ISREG(fd_stat.st_mode);
}

ssize_t read(int fd, void *buffer, size_t count) {
	static auto real_read = get_real_function<ssize_t, int, void*, size_t>("read");

	const ssize_t result = real_read(fd, buffer, count);

	if ( is_regular_file(fd) ) {
		std::cerr << "read size "
		          << count
		          << " of "
		          << get_file_name(fd)
		          << std::endl;
	}

	return result;
}

ssize_t write(int fd, const void* buffer, size_t count) {
	static auto real_write = get_real_function<ssize_t, int, const void*, size_t>("write");

	if ( is_regular_file(fd) ) {
		std::cerr << "write size "
		          << count
		          << " to "
		          << get_file_name(fd)
		          << std::endl;
	}

	return real_write(fd, buffer, count);
}
