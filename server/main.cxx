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
#include <mutex>
#include <unordered_map>
#include <sys/stat.h>

#include "../src/net.hxx"
#include "../src/netmsg.hxx"
#include "../src/log.hxx"
#include "../src/config.hxx"
#include "../src/metadata.hxx"
#include "../src/util.hxx"
#include "user.hxx"

#define LOGIN_INV 1

#define HAND_LOGIN 0
#define HAND_REG   1

#define REG_INV 1
#define REG_CLOSED 2

struct UserData
{
  Metadata *mtd;
  std::mutex lock;
  uint64_t handles;
};

std::unordered_map<std::string, UserData*> udata;
std::mutex udata_lock;

uint64_t filesize(const std::string & path)
{
  struct stat stats;
  if (stat(path.c_str(), &stats) < 0)
    throw std::string("File does not exist: ") + path;
  return (uint64_t)stats.st_size;
}

std::string handshake(Net * net, User * user)
{
  // Send the version
  net->write8(0);

  // Check the command
  uint8_t cmd = net->read8();

  // Grab the login data
  size_t user_len = (size_t)net->read16();
  uint8_t * uusername = new uint8_t[user_len+1];
  net->read_all(uusername, user_len);
  std::string username((char*)uusername, user_len);
  delete uusername;

  size_t pass_len = (size_t)net->read16();
  uint8_t * upass = new uint8_t[pass_len];
  net->read_all(upass, pass_len);
  std::string pass((char*)upass, pass_len);
  delete upass;

  // Check the login
  std::string dir;
  if (cmd == HAND_LOGIN)
    try
      {
        dir = user->login(username, pass);
        net->write8(0);
      }
    catch(const char * e)
      {
        try
          {
            dir = user->reg(username, pass);
            net->write8(0);
          }
        catch(const char * e)
          {
            net->write8(LOGIN_INV);
            throw std::string("Failed to authenticate ") + username;
          }
      }
  else
    try
      {
        dir = user->reg(username, pass);
      }
    catch(const char * e)
      {
        net->write8(REG_INV);
        throw std::string("Failed to register ") + username;
      }

  global_log.message(username + " authenticated successfully", Log::NOTICE);
  return dir;
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
      global_log.message(std::string("Pushed file ") + (char *)filename, 3);
    }
  else if (cmd == CMD_PULL)
    {
      // Get the client filename
      uint32_t filename_len = net->read32();
      uint8_t filename[filename_len+1];
      net->read_all(filename, filename_len);
      filename[filename_len] = 0;

      // Get metadata or write 0 on failure
      struct stat stats;
      if (stat((user_dir + (char*)filename).c_str(), &stats) < 0)
        {
          global_log.message(std::string("Invalid Pull ") +
                             (char *)filename, 3);
          net->write64(0);
          net->write64(0);
        }

      // Write the metadata
      net->write64(stats.st_mtime);
      net->write64(stats.st_size);

      // Write the file
      std::ifstream fin((user_dir + (char *)filename).c_str(),
                       std::ios::in | std::ios::binary);
      uint8_t * bin = new uint8_t[stats.st_size];
      fin.read((char*)bin, stats.st_size);
      fin.close();
      net->write(bin, stats.st_size);
      delete bin;

      global_log.message(std::string("Pulled file ") + (char*)filename, 3);
    }
  else if (cmd == CMD_DEL)
    {
      uint64_t modified = net->read64();
      uint32_t filename_len = net->read32();
      uint8_t filename[filename_len];
      net->read_all(filename, filename_len);

      mtd->delete_file(std::string((char*)filename), modified);
      net->write8(0);
      global_log.message(std::string("Deleted file ") + (char*)filename, 3);
    }
  else
    throw "Invalid command from client";

  return false;
}

void client(std::string store_dir, Net * net, User * user)
{
  UserData * data = NULL;
  NetMsg * netmsg = NULL;
  std::string user_dir;
  uint64_t mtd_size;
  uint8_t * mtd_buff;
  bool mtd_exists = false;

  try
    {
      std::string mtd_name = user_dir + ".mtd";
      user_dir = handshake(net, user);
      netmsg = new NetMsg(net);

      // Get the user data structure
      udata_lock.lock();
      if (udata.count(user_dir) == 0)
        {
          data = new UserData;
          udata[user_dir] = data;

          // Extract the metadata contents
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
              data->mtd = new Metadata(mtd_buff, mtd_size);
              delete mtd_buff;
            }
          else
            data->mtd = new Metadata();
        }
      else
        data = udata.at(user_dir);
      udata_lock.unlock();

      // Make sure the data directory exists
      mkdir(user_dir.c_str(), 0755);

      while (true)
        if(exec_command(user_dir + "/", net, data->mtd))
          break;
        else
          {
            // Save the metadata after each call
            mtd_buff = data->mtd->serialize(mtd_size);
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

  if(data != NULL)
    {
      udata_lock.lock();
      data->handles--;
      if (data->handles == 0)
        {
          delete data->mtd;
          delete data;
          udata.erase(user_dir);
        }
      udata_lock.unlock();
    }

  delete netmsg;
}

int main(int argc, char * argv[])
{
  Config conf;
  std::string conf_file("server.conf");
  std::string store_dir;
  bool daemonize = false;
  NetServer * server = NULL;
  User * user = NULL;

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

      // Setup the user login credentials
      user = new User(store_dir);

      // Accept all client connections and spawn a thread for each
      while (true)
        {
          Net * net = server->accept();
          std::thread c_thread(client, store_dir, net, user);
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
