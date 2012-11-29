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

#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

#include "connector_sock.hxx"
#include "watchdog.hxx"
#include "metadata.hxx"
#include "config.hxx"
#include "connector.hxx"

class Client
{
public:
  /**
   * Creates a new instance of the client which sets up file synchronization
   * It spawns all of the threads necessary to perform config based sync
   * @param conf The configuration file for the Client
   */
  Client(const Config & conf);
  ~Client();

private:
  struct Msg
  {
    bool remote;
    std::string filename;

    Metadata::Data file_data;
  };

  bool done;
  std::string sync_dir;
  Config conf;
  Connector *conn;
  Crypt *crypt;
  Metadata *meta;
  Watchdog wd;
  std::queue<Msg> messages;
  std::mutex message_lock;
  std::condition_variable_any message_cond;

  std::thread *file_thread, *pull_thread, *watch_thread;

  /**
   * Merges the current metadata with another, and pushes change messages
   * @param remote The remote metadata to sync locally
   */
  void merge_metadata(const Metadata & remote);

  void file_master();
  void pull_master();
  void watch_master();
};

#endif
