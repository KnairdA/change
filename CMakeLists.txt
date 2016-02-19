cmake_minimum_required(VERSION 2.8)
project(ChangeLog)

set(
	CMAKE_CXX_FLAGS
	"${CMAKE_CXX_FLAGS} -std=c++14 -W -Wall -Wextra -Winline -ldl -fPIC"
)

include_directories(
	src/
)

add_library(
	ChangeLog
	SHARED
		src/main.cc
		src/bootstrap.cc
		src/init/alloc.cc
		src/utility/io.cc
		src/utility/logger.cc
		src/tracking/path_matcher.cc
		src/tracking/change_tracker.cc
)

target_link_libraries(
	ChangeLog
		boost_system
		boost_filesystem
)

install(
	PROGRAMS
		change
	DESTINATION
		bin
)

install(
	TARGETS
		ChangeLog
	LIBRARY DESTINATION
		lib
)

install(
	DIRECTORY DESTINATION
		share/libChangeLog
	DIRECTORY_PERMISSIONS
		OWNER_READ OWNER_WRITE OWNER_EXECUTE
		GROUP_READ GROUP_EXECUTE
		WORLD_READ WORLD_EXECUTE
)

install(
	DIRECTORY
		src/filter
	DESTINATION
		share/libChangeLog
	FILE_PERMISSIONS
		OWNER_READ OWNER_WRITE
		GROUP_READ WORLD_READ
	PATTERN
		"filter/*.filter"
)
