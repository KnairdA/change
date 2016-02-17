#include "alloc.h"

#include <cstdio>
#include <cstdint>

namespace init {

std::uint8_t dlsym_level   = 0;
size_t       buffer_offset = 0;
char         buffer[4096];

void* static_malloc(size_t size) {
	if ( buffer_offset + size >= sizeof(buffer) ) {
		std::fprintf(stderr, "static allocator out of memory: %lu\n", buffer_offset);
		exit(1);
	}

	buffer_offset += size;

	return buffer + buffer_offset;
}

void* static_calloc(size_t block, size_t size) {
	void* const mem = static_malloc(block * size);

	for ( size_t i = 0; i < (block * size); ++i ) {
		*(static_cast<char* const>(mem) + i) = '\0';
	}

	return mem;
}

bool from_static_buffer(void* ptr) {
	return static_cast<char*>(ptr) >= buffer
	    && static_cast<char*>(ptr) <= buffer + sizeof(buffer);
}

dlsymContext::dlsymContext() {
	++dlsym_level;
}

dlsymContext::~dlsymContext() {
	--dlsym_level;
}

bool dlsymContext::is_active() {
	return dlsym_level > 0;
}

}
