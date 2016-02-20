#ifndef CHANGE_SRC_ACTUAL_FUNCTION_H_
#define CHANGE_SRC_ACTUAL_FUNCTION_H_

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <dlfcn.h>
#include <memory>
#include <cstring>

#include "init/alloc.h"

namespace actual {

template <class Result, typename... Arguments>
using ptr = Result(*)(Arguments...);

template <typename FunctionPtr>
FunctionPtr get_ptr(const std::string& symbol_name) {
	init::dlsymContext guard;

	const void* symbol_address{ dlsym(RTLD_NEXT, symbol_name.c_str()) };

	FunctionPtr actual_function{};
	std::memcpy(&actual_function, &symbol_address, sizeof(symbol_address));

	return actual_function;
}

}

#endif  // CHANGE_SRC_ACTUAL_FUNCTION_H_
