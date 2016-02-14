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
	src/change_log.cc
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
