include_directories(${LIBSYNC_SOURCE_DIR}/src)
link_directories(${CMAKEDEMO_BINARY_DIR}/src)

add_executable(sync-server main.cxx server.cxx user.cxx)
target_link_libraries(sync-server sync)
