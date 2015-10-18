#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <unistd.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <ext/stdio_filebuf.h>

#include <cstring>
#include <iostream>
#include <utility>
#include <functional>
#include <unordered_set>
#include <memory>

struct FileDescriptorGuard {
	FileDescriptorGuard(const std::string& path) {
		this->fd = open(path.c_str(), O_WRONLY | O_APPEND);

		if ( !this->fd ) {
			this->fd = STDERR_FILENO;
		}
	}

	~FileDescriptorGuard() {
		close(this->fd);
	}

	int fd;
};

class LogGuard {
	public:
		LogGuard(const int target_fd):
			buffer(target_fd, std::ios::out),
			stream(&this->buffer) { }

		void append(const std::string& msg) {
			this->stream << msg << std::endl;
		}

	private:
		__gnu_cxx::stdio_filebuf<char> buffer;
		std::ostream                   stream;
};

static std::unordered_set<std::string> tracked_files;

static std::unique_ptr<FileDescriptorGuard> fd_guard;
static std::unique_ptr<LogGuard>            log;

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
	if ( getenv("CHANGE_LOG_TARGET") != NULL ) {
		fd_guard = std::make_unique<FileDescriptorGuard>(getenv("CHANGE_LOG_TARGET"));
		log      = std::make_unique<LogGuard>(fd_guard->fd);
	} else {
		log      = std::make_unique<LogGuard>(STDERR_FILENO);
	}
}

void exit(int status) {
	get_real_function<void, int>("exit")(status);
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

bool is_tracked_file(const std::string& file_name) {
	return tracked_files.find(file_name) != tracked_files.end();
}

bool track_file(const std::string& file_name) {
	return tracked_files.emplace(file_name).second;
}

ssize_t write(int fd, const void* buffer, size_t count) {
	static auto real_write = get_real_function<ssize_t, int, const void*, size_t>("write");

	if ( is_regular_file(fd) ) {
		const std::string file_name{ get_file_name(fd) };

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
		log->append("removed '" + get_file_name(dirfd) + path + "'");
	}

	return real_unlinkat(dirfd, path, flags);
}
