/*
  The entry point into the file storage server

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
#include <string>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <fstream>
#include <thread>

#include "../src/net.hxx"
#include "../src/log.hxx"
#include "../src/config.hxx"
#include "../src/metadata.hxx"

#define LOGIN_INV 1

bool handshake(Net * net)
{
  // Send the version
  net->write8(0);

  // Wait for the login
  size_t user_len = (size_t)net->read16();
  std::cout << "Read User Len" << std::endl;
  uint8_t * user = new uint8_t[user_len+1];
  net->read_all(user, user_len);
  std::cout << "Read Username" << std::endl;
  user[user_len] = 0;

  size_t pass_len = (size_t)net->read16();
  uint8_t * pass = new uint8_t[pass_len+1];
  net->read_all(pass, pass_len);
  pass[pass_len] = 0;

  // Check the login
  if (strcmp("william", (char*)user) != 0 ||
      strcmp("williamisawesome", (char*)pass) != 0)
    {
      global_log.message("Failed to authenticate client", Log::NOTICE);
      net->write8(LOGIN_INV);
      return false;
    }
  net->write8(0);

  global_log.message("william authenticated successfully", Log::NOTICE);
  return true;
}

void client(Net * net)
{
  try
    {
      if (!handshake(net))
        {
          delete net;
          return;
        }
      while (true)
        {
        }
    }
  catch(const std::string & e)
    {
      std::cout << "Client Failed:" << e << std::endl;
    }
  catch(const char * e)
    {
      std::cout << "Client Failed: " << e << std::endl;
    }
}

int main(int argc, char * argv[])
{
  Config conf;
  std::string conf_file("server.conf");
  std::string store_dir;
  bool daemonize = false;
  NetServer * server = NULL;

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
      // Attempt to create the bind server specified in the config
      if (!conf.exists("bind_host") || !conf.exists("bind_port"))
        throw "Requires a port and host to bind on";
      server = new NetServer(conf.get_str("bind_host"),
                           conf.get_int("bind_port"));

      global_log.message("Successfully started!", Log::NOTICE);

      // Accept all client connections and spawn a thread for each
      while (true)
        {
          Net * net = server->accept();
          std::thread c_thread(client, net);
          c_thread.detach();
        }
    }
  catch(const char * e)
    {
      global_log.message(e, 1);
      delete server;
      return EXIT_FAILURE;
    }
  catch(const std::string & e)
    {
      global_log.message(e, 1);
      delete server;
      return EXIT_FAILURE;
    }

  delete server;

  return EXIT_SUCCESS;
}
