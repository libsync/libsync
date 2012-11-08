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
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "net.hxx"
#include "log.hxx"

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
  if (closed)
    return;

  local_close(sock);
  closed = true;
}

void Net::write(const uint8_t * data, size_t size)
{
  ssize_t wrote;
  while (size > 0)
    if ((wrote = send(sock, data, size, 0)) >= 0)
      {
        data += wrote;
        size -= wrote;
      }
    else
      throw std::string("Write error during transmission: ") + strerror(errno);
}

void Net::write(const std::string & data)
{
  write((uint8_t*)data.c_str(), data.length());
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
      throw std::string("Read error during transmission: ") + strerror(errno);
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
  struct addrinfo hints, *servinfo;
  int yes = 1, ret;

  // Allow our socket to bind to both IPv4/v6 on TCP
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  // Get all of the potential bind addresses
  if ((ret = getaddrinfo(host.c_str(), std::to_string(port).c_str(),
                         &hints, &servinfo)) != 0)
    throw std::string("getaddrinfo: ") + gai_strerror(ret);

  // Bind to the first possible address in the list
  lsock = -1;
  for(struct addrinfo *it = servinfo; it != NULL; it = it->ai_next)
    {
      // Create the socket for the address
      if ((lsock = socket(it->ai_family, it->ai_socktype,
                         it->ai_protocol)) == -1)
        continue;

      // Set the socket to be reusable after unclean shutdown
      if (setsockopt(lsock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
          local_close(lsock);
          freeaddrinfo(servinfo);
          throw "Failed to set reusable socket options";
        }

      // Attempt to bind to the socket
      if (bind(lsock, it->ai_addr, it->ai_addrlen) == 0)
        break;

      // The socket didn't work so close it
      local_close(lsock);
      lsock = -1;
    }

  freeaddrinfo(servinfo);

  // No socket was bound
  if (lsock == -1)
    throw std::string("Failed to bind the socket - ") +
      host + ":" + std::to_string(port);

  // Setup the socket for listening
  if (listen(lsock, 10) == -1)
    throw std::string("Failed to listen on socket - ")
      + host + ":" + std::to_string(port);

  global_log.message(std::string("Bound socket on ") +
                     host + ":" + std::to_string(port), Log::NOTICE);
}

NetServer::~NetServer()
{
  close();
}

static int local_accept(int sockfd, struct sockaddr * addr, socklen_t *addrlen)
{
  accept(sockfd, addr, addrlen);
}

Net * NetServer::accept()
{
  int sock;
  struct sockaddr_storage addr;
  socklen_t addr_size = sizeof(addr);

  // Accept the connection
  if ((sock = local_accept(lsock, (struct sockaddr *)&addr, &addr_size)) == -1)
    throw "Failed to accept connection";

  // Retrieve the remote info
  char addr_str[INET6_ADDRSTRLEN];
  uint16_t rport;

  // IPv4 Info
  if (((struct sockaddr *)&addr)->sa_family == AF_INET)
    {
      rport = be16toh(((struct sockaddr_in *)&addr)->sin_port);
      inet_ntop(addr.ss_family, &(((struct sockaddr_in *)&addr)->sin_addr),
                addr_str, INET6_ADDRSTRLEN);
    }

  // IPv6 Info
  else
    {
      rport = be16toh(((struct sockaddr_in6 *)&addr)->sin6_port);
      inet_ntop(addr.ss_family, &(((struct sockaddr_in6 *)&addr)->sin6_addr),
                addr_str, INET6_ADDRSTRLEN);
    }

  global_log.message(std::string("Accepted connection from ") + addr_str +
                                 ":" + std::to_string(rport), Log::NOTICE);

  return new Net(sock, std::string(addr_str), rport);
}

void NetServer::close()
{
  if (closed)
    return;

  local_close(lsock);
  closed = true;
}

NetClient::NetClient(const std::string & host, uint16_t port) :
  host(host), port(port)
{}

static int local_connect(int socket, const struct sockaddr * address,
                         socklen_t address_len)
{
  connect(socket, address, address_len);
}

Net * NetClient::connect()
{
  int sock = -1;
  struct addrinfo hints, *servinfo;
  int ret;

  // Allow our socket to bind to both IPv4/v6 on TCP
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  // Get all of the potential connection addresses
  if ((ret = getaddrinfo(host.c_str(), std::to_string(port).c_str(),
                         &hints, &servinfo)) != 0)
    throw std::string("getaddrinfo: ") + gai_strerror(ret);

  // Connect to the first possible address in the list
  struct addrinfo *it;
  for(it = servinfo; it != NULL; it = it->ai_next)
    {
      // Create the socket
      if ((sock = socket(it->ai_family, it->ai_socktype,
                           it->ai_protocol)) == -1)
        continue;

      if (local_connect(sock, it->ai_addr, it->ai_addrlen) == 0)
        break;

      local_close(sock);
      sock = -1;
    }

  // No socket was connected
  if (sock == -1)
    {
      freeaddrinfo(servinfo);
      throw std::string("Failed to connect to ") +
        host + ":" + std::to_string(port);
    }

  // Retrieve the remote info
  char addr_str[INET6_ADDRSTRLEN];

  // IPv4 Info
  if (((struct sockaddr *)it->ai_addr)->sa_family == AF_INET)
    inet_ntop(it->ai_family,
              &(((struct sockaddr_in *)it->ai_addr)->sin_addr),
              addr_str, INET6_ADDRSTRLEN);

  // IPv6 Info
  else
    inet_ntop(it->ai_family,
              &(((struct sockaddr_in6 *)it->ai_addr)->sin6_addr),
              addr_str, INET6_ADDRSTRLEN);

  freeaddrinfo(servinfo);

  global_log.message(std::string("Connected to ") + addr_str +
                     ":" + std::to_string(port), Log::NOTICE);

  return new Net(sock, std::string(addr_str), port);
}
