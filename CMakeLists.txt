cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(LIBSYNC)

find_package(OpenSSL)
set(LIBS ${OPENSSL_LIBRARIES})

set(CMAKE_CXX_FLAGS "-Wall -std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")

add_subdirectory(src)
add_subdirectory(client)
add_subdirectory(server)
