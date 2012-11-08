/*
  A wrapper network library for portability between OS's

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

#ifndef __NET_HXX__
#define __NET_HXX__

#if defined(__linux__)
#  include <endian.h>
#elif defined(__FreeBSD__) || defined(__NetBSD__)
#  include <sys/endian.h>
#elif defined(__OpenBSD__)
#  include <sys/types.h>
#  define be16toh(x) betoh16(x)
#  define be32toh(x) betoh32(x)
#  define be64toh(x) betoh64(x)
#endif

#include <cstdint>

class Net
{
public:
  Net(int sock, const std::string & host, uint16_t port);
  ~Net();

  void close();

  void write(uint8_t * data, size_t size);
  void write(uint8_t b);
  void write(uint16_t i);
  void write(uint32_t i);
  void write(uint64_t i);

  ssize_t read(uint8_t * data, size_t size);
  void read_all(uint8_t * data, size_t size);
  uint8_t read8();
  uint16_t read16();
  uint32_t read32();
  uint64_t read64();

  int get_fd() const;

private:
  int sock;
  bool closed;

  std::string host;
  uint16_t port;
};

class NetServer
{
public:
  NetServer(const std::string & host, uint16_t port);
  ~NetServer();

  Net * accept();
  void close();
private:
  int listen;
  std::string host;
  uint16_t port;
};

class NetClient
{
public:
  NetClient(const std::string & host, uint16_t port);

  Net * connect();
private:
  std::string host;
  uint16_t port;
};

#endif
