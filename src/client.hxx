/*
  The filesync client wrapper

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

#ifndef __CLIENT_HXX__
#define __CLIENT_HXX__

#include "connector_sock.hxx"
#include "watchdog.hxx"
#include "metadata.hxx"
#include "config.hxx"
#include "messages.hxx"
#include "connector.hxx"

class Client
{
public:
  Client(const Config & conf);
  ~Client();
private:
  enum MsgType
    {
    };
  struct Msg
  {
  };

  Config conf;
  Connector *conn;
  Messages<MsgType, Msg> messages;
  Metadata meta;

  void merge_metadata(const Metadata & remote);

  void file_master();
  void pull_master();
  void watch_master();
};

#endif
