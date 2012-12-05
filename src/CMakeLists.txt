add_library(sync client.cxx config.cxx connector_sock.cxx crypt.cxx log.cxx messages.cxx metadata.cxx net.cxx netmsg.cxx util.cxx watchdog.cxx)
target_link_libraries(sync ${LIBS})
