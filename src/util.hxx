/*
  Useful IO and Datastructure Utilities

  Copyright (C) 2012 William A. Kennington III

  This file is part of Libsync.

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __UTIL_HXX__
#define __UTIL_HXX__

#include <cstdint>
#include <cstring>
#include <string>

#ifdef WIN32
#  include <filesystem>
#  define fs std::tr2::sys
#else
#  include <boost/filesystem.hpp>
#  define fs boost::filesystem
#endif

#if defined(__linux__)
#  include <endian.h>
#elif defined(__FreeBSD__) || defined(__NetBSD__)
#  include <sys/endian.h>
#elif defined(__OpenBSD__)
#  include <sys/types.h>
#  define be16toh(x) betoh16(x)
#  define be32toh(x) betoh32(x)
#  define be64toh(x) betoh64(x)
#elif defined(WIN32)
#  include <Windows.h>
#  define be16toh(x) ntohs(x)
#  define be32toh(x) ntohl(x)
#  define be64toh(x) htobe64(x)
#  define htobe16(x) htons(x)
#  define htobe32(x) htonl(x)
uint64_t htobe64(uint64_t x)
{
  // Populate the mapping
  union {
    uint64_t val;
    uint8_t bytes[8];
  } map, num, out;
  map.val = 0;
  for (int i = 1; i < 8; i++)
    map.val = map.val << 8 | i;

  // Map the bytes to their proper spots
  num.val = x;
  for (int i = 0; i < 8; i++)
    out.bytes[i] = num.bytes[map.bytes[i]];
  return out.val;
}
#endif

namespace Read
{
  uint8_t i8(uint8_t * & data, size_t & size);
  uint16_t i16(uint8_t * & data, size_t & size);
  uint32_t i32(uint8_t * & data, size_t & size);
  uint64_t i64(uint8_t * & data, size_t & size);
};

namespace Write
{
  void i8(uint8_t i, uint8_t * data, size_t & offset);
  void i8(uint8_t i, std::string & data);
  void i16(uint16_t i, uint8_t * data, size_t & offset);
  void i16(uint16_t i, std::string & data);
  void i32(uint32_t i, uint8_t * data, size_t & offset);
  void i32(uint32_t i, std::string & data);
  void i64(uint64_t i, uint8_t * data, size_t & offset);
  void i64(uint64_t i, std::string & data);
};

namespace File
{
  /**
   * Recursively deletes the file and all of its directories up to sync dir
   * @param filename The name of the file or directory to remove recursively
   */
  void recursive_remove(const std::string & filename);

  /**
   * Recursively creates the directories from sync dir to the file
   * @param filename The name of the file to create
   */
  void recursive_create(const std::string & filename);
};

#endif
