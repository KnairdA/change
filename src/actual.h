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

namespace actual {

template <class Result, typename... Arguments>
using ptr = Result(*)(Arguments...);

template <typename FunctionPtr>
FunctionPtr get_ptr(const std::string& symbol_name) {
	const void* symbol_address{ dlsym(RTLD_NEXT, symbol_name.c_str()) };

	FunctionPtr actual_function{};
	std::memcpy(&actual_function, &symbol_address, sizeof(symbol_address));

	return actual_function;
}

}

#endif  // CHANGE_SRC_ACTUAL_FUNCTION_H_
