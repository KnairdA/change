#ifndef CHANGE_SRC_INIT_ALLOC_H_
#define CHANGE_SRC_INIT_ALLOC_H_

#include <cstdlib>

namespace init {

void* static_malloc(size_t size);
void* static_calloc(size_t block, size_t size);

bool from_static_buffer(void* ptr);

struct dlsymContext {
	dlsymContext();
	~dlsymContext();

	static bool is_active();
};

}

#endif  // CHANGE_SRC_INIT_ALLOC_H_
