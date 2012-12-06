find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
set(LIBS ${OPENSSL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

add_library(sync client.cxx config.cxx connector_sock.cxx crypt.cxx log.cxx messages.cxx metadata.cxx net.cxx netmsg.cxx util.cxx watchdog.cxx)
target_link_libraries(sync ${LIBS})

include_directories(${LIBSYNC_SOURCE_DIR}/src)
link_directories(${LIBSYNC_BINARY_DIR}/src)

file(GLOB files "test/*.cxx")
foreach(file ${files})
	get_filename_component(fname ${file} NAME_WE)
	add_executable(test_${fname} ${file} ../util/gtest_main.cc)
	add_dependencies(test_${fname} googletest)
	target_link_libraries(test_${fname} sync ${binary_dir}/libgtest.a)
	add_test(${fname} test_${fname})
endforeach()
