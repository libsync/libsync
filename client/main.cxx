/*
  The entry point into the file synchronization client

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

#include <cstdlib>
#include <thread>
#include <iostream>
#include <fstream>

#include "../src/log.hxx"
#include "../src/config.hxx"
#include "../src/connector_sock.hxx"
#include "../src/watchdog.hxx"
#include "../src/metadata.hxx"

int main(int argc, char * argv[])
{
  Config conf;
  std::string conf_file("client.conf");
  std::string sync_dir;
  bool daemonize = false;
  Connector * conn = NULL;
  Watchdog wd;
  Metadata * meta = NULL;

  // Catch errors in stderr until we have the log output setup
  try
    {
      // Parse command line arguments
      for (int i = 1; i < argc; i++)
        {
          std::string arg(argv[i]);
          if (arg == "-d")
            daemonize = true;
          else if (arg == "-c" && i+1 < argc && argv[i+1][0] != '-')
            {
              conf_file = argv[i+1];
              i++;
            }
          else
            throw "sync-client [-d] [-c <config_file>]";
        }

      // Retrieve the Configuration File
      conf.read(conf_file);

      // Setup the Global Log
      if (!daemonize)
        global_log.add_output(&std::cout);
      if (conf.exists("log_file"))
        global_log.add_output(conf.get_str("log_file"));
      if (conf.exists("log_level"))
        global_log.set_level(conf.get_int("log_level"));
      else
        global_log.set_level(Log::NOTICE);
    }
  catch(const char * e)
    {
      std::cerr << e << std::endl;
      return EXIT_FAILURE;
    }
  catch(const std::string & e)
    {
      std::cerr << e << std::endl;
      return EXIT_FAILURE;
    }

  // Catch errors to the log output
  try
    {
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

      // Try Opening a Watchdog on the watch folder
      if (!conf.exists("sync_dir"))
        throw "Client must specify synchronization directory";
      sync_dir = conf.get_str("sync_dir");
      wd.add_watch(sync_dir);

      // Try Getting Metadata
      meta = conn->get_metadata();

      // Compare the received metadata
      Metadata local_md(sync_dir);
      for (auto it = meta->begin(); it != meta->end(); it++)
        {
          Metadata::Data local = local_md.get_file(it->first);
          std::string filename = sync_dir + it->first;
          if (it->second.modified > local.modified)
            if(it->second.deleted)
              remove(filename.c_str());
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
    }
  catch(const char * e)
    {
      global_log.message(e, 1);
      delete meta;
      delete conn;
      return EXIT_FAILURE;
    }
  catch(const std::string & e)
    {
      global_log.message(e, 1);
      delete meta;
      delete conn;
      return EXIT_FAILURE;
    }

  global_log.message("Successfully started!", Log::NOTICE);

  while(true)
    {
      Watchdog::Data data = wd.wait();
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

  delete meta;
  delete conn;

  return EXIT_SUCCESS;
}
