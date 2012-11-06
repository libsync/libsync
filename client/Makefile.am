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

bin_PROGRAMS = sync-client
sync_client_SOURCES = main.cxx
sync_client_LDADD = ../src/libsync.la
sync_client_LDFLAGS = -pthread

#TEST SECTION
#check_PROGRAMS = conf_test
#TESTS = $(check_PROGRAMS)
#conf_test_SOURCES = test/list.c
#conf_test_LDADD = libadt.la
