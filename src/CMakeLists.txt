find_package(Threads REQUIRED)
set(LIBS ${OPENSSL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

if(NOT CMAKE_SYSTEM_NAME MATCHES "Windows")
	find_package(Boost COMPONENTS regex filesystem system REQUIRED)
endif()

add_library(sync client.cxx config.cxx connector_sock.cxx crypt.cxx log.cxx messages.cxx metadata.cxx net.cxx netmsg.cxx util.cxx watchdog.cxx)
target_link_libraries(sync ${LIBS} ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY})

include_directories(${LIBSYNC_SOURCE_DIR}/src)
link_directories(${LIBSYNC_BINARY_DIR}/src)

if ( MSVC )
	add_definitions( /D _VARIADIC_MAX=10 )
	set(CMAKE_FILE_LIBRARY_SUFFIXES .lib)
	set(dir Debug)
else ()
	set(CMAKE_FILE_LIBRARY_SUFFIXES .a)
endif()

file(GLOB files "test/*.cxx")
foreach(file ${files})
	get_filename_component(fname ${file} NAME_WE)
	add_executable(test_${fname} ${file} ../util/gtest_main.cc)
	add_dependencies(test_${fname} googletest)
	target_link_libraries(test_${fname} sync ${binary_dir}/${dir}/${CMAKE_FIND_LIBRARY_PREFIXES}gtest${CMAKE_FILE_LIBRARY_SUFFIXES} ${Boost_REGEX_LIBRARY})
	add_test(${fname} test_${fname})
endforeach()
