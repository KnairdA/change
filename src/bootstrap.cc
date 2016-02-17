#include "actual.h"

#include "init/alloc.h"

void free(void* ptr) {
	static actual::ptr<void, void*> actual_free{};

	if ( !actual_free ) {
		actual_free = actual::get_ptr<decltype(actual_free)>("free");
	}

	if ( !init::from_static_buffer(ptr) ) {
		actual_free(ptr);
	}
}

void* malloc(size_t size) {
	static actual::ptr<void*, size_t> actual_malloc{};

	if ( init::dlsymContext::is_active() ) {
		return init::static_malloc(size);
	} else {
		if ( !actual_malloc ) {
			actual_malloc = actual::get_ptr<decltype(actual_malloc)>("malloc");
		}

		return actual_malloc(size);
	}
}

void* calloc(size_t block, size_t size) {
	static actual::ptr<void*, size_t, size_t> actual_calloc{};

	if ( init::dlsymContext::is_active() ) {
		return init::static_calloc(block, size);
	} else {
		if ( !actual_calloc ) {
			actual_calloc = actual::get_ptr<decltype(actual_calloc)>("calloc");
		}

		return actual_calloc(block, size);
	}
}
