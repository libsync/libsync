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

AC_INIT([libsync], [0.1], [william@wkennington.com], [], [https://github.com/libsync/libsync])

AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AM_MAINTAINER_MODE([enable])

AC_CANONICAL_HOST

AC_PROG_CPP
AC_PROG_CC
AC_PROG_CXX
AC_PROG_CC_STDC
AC_PROG_INSTALL

# WIN32 Support
AC_LIBTOOL_WIN32_DLL

AM_PROG_AR
AM_PROG_LIBTOOL
AM_PROG_CC_C_O
AM_PROG_LD

LT_INIT
PKG_CHECK_MODULES([LIBDEPS], [libcrypto])
PKG_CHECK_MODULES([CHECK], [check >= 0.9.4])

DX_HTML_FEATURE(ON)
DX_CHM_FEATURE(OFF)
DX_CHI_FEATURE(OFF)
DX_MAN_FEATURE(ON)
DX_RTF_FEATURE(OFF)
DX_XML_FEATURE(OFF)
DX_PDF_FEATURE(OFF)
DX_PS_FEATURE(OFF)
DX_INIT_DOXYGEN(libsync, doxygen.conf, doc)

AC_CONFIG_FILES([
 Makefile
 client/Makefile
 server/Makefile
 src/Makefile
])
AC_OUTPUT
