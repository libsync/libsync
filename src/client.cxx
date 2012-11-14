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

#include <iostream>
#include <fstream>

#include "client.hxx"
#include "log.hxx"

Client::Client(const Config & conf)
  : conf(conf), conn(NULL), meta(NULL), file_thread(NULL),
    pull_thread(NULL), watch_thread(NULL)
{
  Metadata *remote = NULL;

  try
    {
      // Load the local metadata from the sync directory
      if (!conf.exists("sync_dir"))
        throw "Client must specify synchronization directory";
      sync_dir = conf.get_str("sync_dir");
      meta = new Metadata(sync_dir);

      // Attempt to create the connection type specified in the config
      if (!conf.exists("conn") || conf.get_str("conn") == "sock")
        if (!conf.exists("conn_host") || !conf.exists("conn_port") ||
            !conf.exists("conn_user") || !conf.exists("conn_pass"))
          throw "Socket Connector Missing Parameters";
        else
          conn = new SockConnector(conf.get_str("conn_host"),
                                   conf.get_int("conn_port"),
                                   conf.get_str("conn_user"),
                                   conf.get_str("conn_pass"));
      else
        throw "Unrecognized connector type - " + conf.get_str("conn");

      // Spawn the threads for handling server transactions

      // Get the remote metadata and perform a merge with local metadata
      remote = conn->get_metadata();
      merge_metadata(*remote);

      // Spawn the watchdog
      // Spawn the file and metadata manager
    }
  catch(const char * e)
    {
      global_log.message(e, 1);
      delete meta;
      delete conn;
      delete remote;
      throw e;
    }
  catch(const std::string & e)
    {
      global_log.message(e, 1);
      delete meta;
      delete conn;
      delete remote;
      throw e;
    }
  delete remote;

  global_log.message("Client successfully started!", Log::NOTICE);
}

Client::~Client()
{
  delete file_thread;
  delete pull_thread;
  delete watch_thread;
  delete meta;
  delete conn;
}

void Client::recursive_remove(const std::string & filename) const
{
}

void Client::recursive_create(const std::string & filename) const
{
}

void Client::merge_metadata(const Metadata & remote)
{
  // Merge all of the local data into the remote data and push messages
  for (auto it = meta->begin(); it != meta->end(); it++)
    {
      Metadata::Data local = remote.get_file(it->first);
      std::string filename = sync_dir + it->first;
      if (it->second.modified > local.modified)
        if(it->second.deleted)
          recursive_remove(it->first);
        else
          {
            std::ofstream file(filename.c_str(),
                               std::ios::out | std::ios::binary);
            conn->get_file(it->first, local.modified, file);
            file.close();
          }
      else if (it->second.modified < local.modified)
        {
          std::ifstream file(filename.c_str(),
                             std::ios::in | std::ios::binary);
          conn->push_file(it->first, local.modified, file, local.size);
          file.close();
        }
    }

  // Merge the remote data into the local data and pull messages
  for (auto it = remote.begin(), end = remote.end(); it != end; it++)
    {
      Metadata::Data local =  meta->get_file(it->first);
    }
}

void Client::file_master()
{
}

void Client::pull_master()
{
}

void Client::watch_master()
{
  while(true)
    {
      try
        {
          Watchdog::Data data = wd.wait();
          data.filename = data.filename.substr(sync_dir.length());
          if (data.status == Watchdog::FileStatus::modified)
            {
              meta->modify_file(data.filename, data.modified);
              std::ifstream file((sync_dir + data.filename).c_str(),
                                 std::ios::in | std::ios::binary);
              conn->push_file(data.filename, data.modified, file, data.size);
              file.close();
            }
          else
            {
              meta->delete_file(data.filename, data.modified);
              conn->delete_file(data.filename, data.modified);
            }
        }
      catch(const char * e)
        {
          global_log.message(e, 1);
        }
      catch(const std::string & e)
        {
          global_log.message(e, 1);
        }
    }
  // Retrieve the first event

  // Wait to ensure we have no duplicates

  // Remove the duplicate events

  // Send the events to the file thread
}
