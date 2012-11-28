# Copyright (C) 2012 William A. Kennington III
#
# This file is part of Libsync.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = -I ../m4

lib_LTLIBRARIES = libsync.la
libsync_la_SOURCES = net.cxx config.cxx log.cxx metadata.cxx watchdog.cxx connector_sock.cxx messages.cxx client.cxx util.cxx netmsg.cxx crypt.cxx
libsync_la_LDFLAGS = -version-info 0:1:0
AM_CXXFLAGS = $(LIBDEPS_CXXFLAGS)
libsync_la_LIBADD = $(BOOST_REGEX_LIBS) $(LIBDEPS_LIBS)

#TEST SECTION
check_PROGRAMS = log_test config_test net_test watchdog_test connector_sock_test metadata_test
TESTS = $(check_PROGRAMS)

log_test_SOURCES = test/log.cxx ../util/gtest_main.cc
log_test_LDADD = libsync.la -lgtest $(BOOST_REGEX_LIBS)

config_test_SOURCES = test/config.cxx ../util/gtest_main.cc
config_test_LDADD = libsync.la -lgtest

net_test_SOURCES = test/net.cxx ../util/gtest_main.cc
net_test_LDADD = libsync.la -lgtest

watchdog_test_SOURCES = test/watchdog.cxx ../util/gtest_main.cc
watchdog_test_LDADD = libsync.la -lgtest

connector_sock_test_SOURCES = test/connector_sock.cxx ../util/gtest_main.cc
connector_sock_test_LDADD = libsync.la -lgtest

metadata_test_SOURCES = test/metadata.cxx ../util/gtest_main.cc
metadata_test_LDADD = libsync.la -lgtest
