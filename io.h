#ifndef CHANGE_IO_H_
#define CHANGE_IO_H_

#include <unistd.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <cstring>

namespace io {

class FileDescriptorGuard {
	public:
		FileDescriptorGuard(const std::string& path) {
			this->fd = open(path.c_str(), O_WRONLY | O_APPEND);

			if ( !this->fd ) {
				this->fd = STDERR_FILENO;
			}
		}

		~FileDescriptorGuard() {
			close(this->fd);
		}

		operator int() {
			return this->fd;
		}

	private:
		int fd;

};

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

}

#endif  // CHANGE_IO_H_
