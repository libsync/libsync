cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(LIBSYNC)

if ( MSVC )
	add_definitions( /MTd )
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")
#set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
#set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG")
#set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
#set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")

enable_testing()
include(ExternalProject)
SET_DIRECTORY_PROPERTIES(PROPERTIES EP_PREFIX ${CMAKE_BINARY_DIR}/ThirdParty)
ExternalProject_Add(
	googletest
	SVN_REPOSITORY http://googletest.googlecode.com/svn/trunk/
	TIMEOUT 10
	INSTALL_COMMAND ""
	LOG_DOWNLOAD ON
	LOG_CONFIGURE ON
	LOG_BUILD ON)
ExternalProject_Get_Property(googletest source_dir)
ExternalProject_Get_Property(googletest binary_dir)
INCLUDE_DIRECTORIES(${source_dir}/include)

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

add_subdirectory(src)
add_subdirectory(client)
add_subdirectory(server)
