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

#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "net.hxx"

Net::Net(int sock, const std::string & host, uint16_t port) :
  sock(sock), closed(false), host(host), port(port)
{}

Net::~Net()
{
  close();
}

static void local_close(int fd)
{
  close(fd);
}

void Net::close()
{
  if (!closed)
    local_close(sock);
}

void Net::write(uint8_t * data, size_t size)
{
  ssize_t wrote;
  while (size > 0)
    if ((wrote = send(sock, data, size, 0)) >= 0)
      {
        data += wrote;
        size -= wrote;
      }
    else
      throw "Write error during transmission";
}

void Net::write(uint8_t b)
{
  write((uint8_t*)&b, sizeof(b));
}

void Net::write(uint16_t i)
{
  i = htobe16(i);
  write((uint8_t*)&i, sizeof(i));
}

void Net::write(uint32_t i)
{
  i = htobe32(i);
  write((uint8_t*)&i, sizeof(i));
}

void Net::write(uint64_t i)
{
  i = htobe64(i);
  write((uint8_t*)&i, sizeof(i));
}

ssize_t Net::read(uint8_t * data, size_t size)
{
  return recv(sock, data, size, 0);
}

void Net::read_all(uint8_t * data, size_t size)
{
  ssize_t len = 0;
  while (size > 0)
    if ((len = read(data, size)) >= 0)
      {
        data += len;
        size -= len;
      }
    else
      throw "Read error during transmission";
}

uint8_t Net::read8()
{
  uint8_t i;
  read_all((uint8_t*)&i, sizeof(i));
  return i;
}

uint16_t Net::read16()
{
  uint16_t i;
  read_all((uint8_t*)&i, sizeof(i));
  i = be16toh(i);
  return i;
}

uint32_t Net::read32()
{
  uint32_t i;
  read_all((uint8_t*)&i, sizeof(i));
  i = be32toh(i);
  return i;
}

uint64_t Net::read64()
{
  uint64_t i;
  read_all((uint8_t*)&i, sizeof(i));
  i = be64toh(i);
  return i;
}

int Net::get_fd() const
{
  if (closed)
    return -1;
  return sock;
}

NetServer::NetServer(const std::string & host, uint16_t port) :
  host(host), port(port)
{
}

NetServer::~NetServer()
{
}

Net * NetServer::accept()
{
  return NULL;
}

void NetServer::close()
{
}

NetClient::NetClient(const std::string & host, uint16_t port) :
  host(host), port(port)
{}

Net * NetClient::connect()
{
  return NULL;
}
