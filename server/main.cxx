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
#include <sys/stat.h>

#include "../src/net.hxx"
#include "../src/log.hxx"
#include "../src/config.hxx"
#include "../src/metadata.hxx"

#define LOGIN_INV 1

#define HAND_LOGIN 0
#define HAND_REG   1

#define REG_CLOSED 2

uint64_t filesize(const std::string & path)
{
  struct stat stats;
  if (stat(path.c_str(), &stats) < 0)
    throw std::string("File does not exist: ") + path;
  return (uint64_t)stats.st_size;
}

bool handshake(Net * net)
{
  // Send the version
  net->write8(0);

  // Check the command
  uint8_t cmd = net->read8();
  if (cmd == HAND_REG)
    {
      net->write8(REG_CLOSED);
      return false;
    }

  // Wait for the login
  size_t user_len = (size_t)net->read16();
  uint8_t * user = new uint8_t[user_len+1];
  net->read_all(user, user_len);
  user[user_len] = 0;

  size_t pass_len = (size_t)net->read16();
  uint8_t * pass = new uint8_t[pass_len+1];
  net->read_all(pass, pass_len);
  pass[pass_len] = 0;

  // Check the login
  if (strcmp("william", (char*)user) != 0 ||
      strcmp("iamwilliam", (char*)pass) != 0)
    {
      global_log.message("Failed to authenticate client", Log::NOTICE);
      net->write8(LOGIN_INV);
      return false;
    }
  net->write8(0);

  global_log.message("william authenticated successfully", Log::NOTICE);
  return true;
}

#define CMD_QUIT 0
#define CMD_META 1
#define CMD_PUSH 2
#define CMD_PULL 3
#define CMD_DEL 4

#define BUFF 2048

bool exec_command(const std::string & user_dir, Net * net, Metadata * mtd)
{
  uint8_t cmd = net->read8();
  if (cmd == CMD_QUIT)
    return true;

  else if (cmd == CMD_META)
    {
      size_t size;
      uint8_t * data = mtd->serialize(size);
      net->write32(size);
      net->write(data, size);
    }
  else if (cmd == CMD_PUSH)
    {
      // Read in the metadata
      uint64_t modified = net->read64();
      uint32_t filename_len = net->read32();
      uint64_t file_len = net->read64();

      // Read the filename
      uint8_t filename[filename_len+1];
      net->read_all(filename, filename_len);
      filename[filename_len] = 0;

      // Read all of the file contents and push to file
      uint8_t * file = new uint8_t[file_len];
      net->read_all(file, file_len);
      std::ofstream out(user_dir + (char *)filename,
                        std::ios::out | std::ios::binary);
      out.write((char *)file, file_len);
      out.close();
      delete file;

      // Update Metadata
      mtd->modify_file(std::string((char *)filename), modified);

      net->write8(0);
    }
  else if (cmd == CMD_PULL)
    {
      uint32_t filename_len = net->read32();
      uint8_t filename[filename_len];
    }
  else if (cmd == CMD_DEL)
    {
      uint64_t modified = net->read64();
      uint32_t filename_len = net->read32();
      uint8_t filename[filename_len];
      net->read_all(filename, filename_len);

      mtd->delete_file(std::string((char*)filename), modified);
      net->write8(0);
    }
  else
    throw "Invalid command from client";

  return false;
}

void client(std::string store_dir, Net * net)
{
  try
    {
      if (!handshake(net))
        {
          delete net;
          return;
        }

      // Extract the metadata contents
      std::string mtd_name = store_dir + "/william.mtd";
      uint64_t mtd_size;
      uint8_t * mtd_buff;
      bool mtd_exists = false;
      Metadata mtd;
      try
        {
          mtd_size = filesize(mtd_name);
          mtd_exists = true;
        }
      catch(const std::string & e) {}
      if (mtd_exists)
        {
          mtd_buff = new uint8_t[mtd_size];
          std::ifstream fin(mtd_name, std::ios::in | std::ios::binary);
          fin.read((char*)mtd_buff, mtd_size);
          fin.close();
          mtd = Metadata(mtd_buff, mtd_size);
          delete mtd_buff;
        }

      while (true)
        if(exec_command(store_dir + "/william/", net, &mtd))
          break;
        else
          {
            // Save the metadata after each call
            mtd_buff = mtd.serialize(mtd_size);
            std::ofstream fout(mtd_name, std::ios::out | std::ios::binary);
            fout.write((char*)mtd_buff, mtd_size);
            fout.close();
            delete mtd_buff;
          }
    }
  catch(const std::string & e)
    {
      global_log.message(std::string("Client Failed: ") + e, 2);
    }
  catch(const char * e)
    {
      global_log.message(std::string("Client Failed: ") + e, 2);
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

      // Check to make sure we have a storage directory
      if (!conf.exists("store_dir"))
        throw "Requires a directory to store data in";
      store_dir = conf.get_str("store_dir");

      global_log.message("Successfully started!", Log::NOTICE);

      // Accept all client connections and spawn a thread for each
      while (true)
        {
          Net * net = server->accept();
          std::thread c_thread(client, store_dir, net);
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
