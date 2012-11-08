/*
  Socket Connector Module

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

#ifndef __CONNECTOR_SOCKET_HXX__
#define __CONNECTOR_SOCKET_HXX__

#include <cstdint>
#include <istream>
#include <ostream>
#include <string>

#include "net.hxx"
#include "connector.hxx"
#include "metadata.hxx"

class SockConnector : public Connector
{
public:
  SockConnector(const std::string & host, uint16_t port,
                const std::string & user, const std::string & pass,
                bool reg = false);
  ~SockConnector();

  Metadata * get_metadata();
  void push_file(const std::string & filename, uint64_t modified,
                 std::istream & data, size_t data_size);
  void get_file(const std::string & filename, uint64_t & modified,
                std::ostream & data);
  void delete_file(const std::string & filename, uint64_t & modified);
private:
  NetClient client;
  std::string user, pass;
  Net * net;

  void connect(bool reg = false);
};

#endif
