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

#include "util.hxx"

#include <cstdint>
#include <string>

class Net
{
public:
  Net(int sock, const std::string & host, uint16_t port);
  ~Net();

  void close();

  void write(const uint8_t * data, size_t size);
  void write(const std::string & data);
  void write8(uint8_t b);
  void write16(uint16_t i);
  void write32(uint32_t i);
  void write64(uint64_t i);

  int64_t read(uint8_t * data, size_t size);
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
  int lsock;
  bool closed;

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
