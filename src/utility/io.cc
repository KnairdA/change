#include "io.h"

#include <unistd.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/stat.h>

namespace utility {

FileDescriptorGuard::FileDescriptorGuard(const std::string& path) {
	this->fd_ = open(path.c_str(), O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);

	if ( !this->fd_ ) {
		this->fd_ = STDERR_FILENO;
	}
}

FileDescriptorGuard::~FileDescriptorGuard() {
	close(this->fd_);
}

FileDescriptorGuard::operator int() const {
	return this->fd_;
}

std::string get_file_path(int fd) {
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

bool is_regular_file(const char* path) {
	struct stat fd_stat;
	lstat(path, &fd_stat);

	return S_ISREG(fd_stat.st_mode);
}

}
