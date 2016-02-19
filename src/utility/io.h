#ifndef CHANGE_SRC_UTILITY_IO_H_
#define CHANGE_SRC_UTILITY_IO_H_

#include <string>

namespace utility {

class FileDescriptorGuard {
	public:
		FileDescriptorGuard(const std::string& path);
		~FileDescriptorGuard();

		operator int() const;

	private:
		int fd_;

};

std::string get_file_path(int fd);

bool is_regular_file(int fd);
bool is_regular_file(const char* path);

}

#endif  // CHANGE_SRC_UTILITY_IO_H_
