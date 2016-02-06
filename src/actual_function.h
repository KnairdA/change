#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef CHANGE_SRC_ACTUAL_FUNCTION_H_
#define CHANGE_SRC_ACTUAL_FUNCTION_H_

#include <dlfcn.h>
#include <sys/mman.h>
#include <sys/uio.h>

#include <memory>
#include <cstring>

namespace {
	template <class Result, typename... Arguments>
	std::function<Result(Arguments...)> get_actual_function(
		const std::string& symbol_name) {

		Result (*real_function)(Arguments...) = nullptr;
		const void* symbol_address = dlsym(RTLD_NEXT, symbol_name.c_str());

		std::memcpy(&real_function, &symbol_address, sizeof(symbol_address));

		return std::function<Result(Arguments...)>(real_function);
	}
}

namespace actual {
	static auto write = get_actual_function<
		ssize_t,
		int, const void*, size_t
	>("write");

	static auto writev = get_actual_function<
		ssize_t,
		int, const iovec*, int
	>("writev");

	static auto rename = get_actual_function<
		int,
		const char*, const char*
	>("rename");

	static auto rmdir = get_actual_function<
		int,
		const char*
	>("rmdir");

	static auto unlink = get_actual_function<
		int,
		const char*
	>("unlink");

	static auto unlinkat = get_actual_function<
		int,
		int, const char*, int
	>("unlinkat");

	static auto mmap = get_actual_function<
		void*,
		void*, size_t, int, int, int, off_t
	>("mmap");
}

#endif  // CHANGE_SRC_ACTUAL_FUNCTION_H_
