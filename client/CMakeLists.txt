include_directories(${LIBSYNC_SOURCE_DIR}/src)
link_directories(${CMAKEDEMO_BINARY_DIR}/src)

add_executable(sync-client main.cxx)
target_link_libraries(sync-client sync)
