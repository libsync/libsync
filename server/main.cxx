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
#include <unordered_set>
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
  std::unordered_set<NetMsg *> handles;
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
  delete[] uusername;

  size_t pass_len = (size_t)net->read16();
  uint8_t * upass = new uint8_t[pass_len];
  net->read_all(upass, pass_len);
  std::string pass((char*)upass, pass_len);
  delete[] upass;

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

void exec_command(const std::string & user_dir, Message * msg,
                  NetMsg * netmsg, UserData * data)
{
  uint8_t *ret = (uint8_t*)msg->get().data(), cmd;
  size_t ret_len = msg->get().length();
  cmd = Read::i8(ret, ret_len);

  if (cmd == CMD_META)
    {
      size_t size;
      uint8_t * dat = data->mtd->serialize(size);
      std::string sdat((char*)dat, size);
      global_log.message(std::to_string(size), Log::NOTICE);
      msg->set(sdat);
      netmsg->reply_only(msg);
    }
  else if (cmd == CMD_PUSH)
    {
      // Read in the metadata
      uint64_t modified = Read::i64(ret, ret_len);
      uint32_t filename_len = Read::i32(ret, ret_len);
      std::string filename((char*)ret, filename_len);
      ret += filename_len;
      ret_len -= filename_len;

      // If the metadata has changed kill it
      std::string cmd;
      if (data->mtd->get_file(filename).modified > modified)
        {
          Write::i8(1, cmd);
          msg->set(cmd);
          netmsg->reply_only(msg);
          global_log.message(std::string("Skipped Push: ") + filename,
                             Log::NOTICE);
          return;
        }

      // Read all of the file contents and push to file
      std::ofstream out(user_dir + filename,
                        std::ios::out | std::ios::binary);
      Write::i8(0, cmd);
      msg->set(cmd);
      global_log.message("Writing to data file", Log::DEBUG);
      netmsg->reply_and_wait(msg, &out);
      out.close();

      // Acknowledge successful transfer
      global_log.message("Writing Succeeded", Log::DEBUG);
      msg->set(cmd);
      netmsg->reply_only(msg);

      // Update Metadata
      struct stat stats;
      stat((user_dir + filename).c_str(), &stats);
      data->mtd->modify_file(filename, stats.st_size, modified);

      // Send the update message to all clients
      cmd.clear();
      Write::i32(filename.length(), cmd);
      cmd.append(filename);
      Write::i64(modified, cmd);
      Write::i8(0, cmd);
      for (auto it = data->handles.begin(), end = data->handles.end();
           it != end; it++)
        {
          if (*it == netmsg)
            continue;

         try
            {
              Message *msg2 = (*it)->send_and_wait(cmd);
              netmsg->destroy(msg2);
            }
          catch(const char * e)
            {
              global_log.message("Failed to push update to client",
                                 Log::WARNING);
            }
          catch(const std::string & e)
            {
              global_log.message("Failed to push update to client",
                                 Log::WARNING);
            }
        }

      global_log.message(std::string("Pushed file ") + filename, Log::NOTICE);
    }
  else if (cmd == CMD_PULL)
    {
      // Get the client filename
      uint32_t filename_len = Read::i32(ret, ret_len);
      std::string filename((char*)ret, filename_len);
      ret += filename_len;
      ret_len -= filename_len;

      // Get metadata or write 1 on failure
      Metadata::Data fd = data->mtd->get_file(filename);

      // Write the metadata
      std::string cmd;
      Write::i8(0, cmd);
      Write::i64(fd.modified, cmd);
      msg->set(cmd);
      msg = netmsg->reply_and_wait(msg);

      // Write the file
      std::ifstream fin(user_dir + filename, std::ios::in | std::ios::binary);
      msg = netmsg->reply_and_wait(msg, &fin, fd.size);
      netmsg->destroy(msg);

      global_log.message(std::string("Pulled file ") + filename, Log::NOTICE);
    }
  else if (cmd == CMD_DEL)
    {
      uint64_t modified = Read::i64(ret, ret_len);
      uint32_t filename_len = Read::i32(ret, ret_len);
      std::string filename((char*)ret, filename_len);
      ret += filename_len;
      ret_len -= filename_len;

      // Update the metadata
      data->mtd->delete_file(filename, modified);

      // Reply Success
      std::string cmd;
      Write::i8(0, cmd);
      msg->set(cmd);
      netmsg->reply_only(msg);

      // Propagate the deletion to all clients
      cmd.clear();
      Write::i32(filename.length(), cmd);
      cmd.append(filename);
      Write::i64(modified, cmd);
      Write::i8(0, cmd);
      for (auto it = data->handles.begin(), end = data->handles.end();
           it != end; it++)
        {
          if (*it == netmsg)
            continue;

          try
            {
              Message *msg2 = (*it)->send_and_wait(cmd);
              netmsg->destroy(msg2);
            }
          catch(const char * e)
            {
              global_log.message("Failed to push update to client",
                                 Log::WARNING);
            }
          catch(const std::string & e)
            {
              global_log.message("Failed to push update to client",
                                 Log::WARNING);
            }
        }
      global_log.message(std::string("Deleted file ") + filename, Log::NOTICE);
    }
  else
    throw "Invalid command from client";
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
      user_dir = handshake(net, user);
      std::string mtd_name = user_dir + ".mtd";
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
              global_log.message(std::string("Deserializing: ") + mtd_name,
                                 Log::NOTICE);
              mtd_buff = new uint8_t[mtd_size];
              std::ifstream fin(mtd_name, std::ios::in | std::ios::binary);
              fin.read((char*)mtd_buff, mtd_size);
              fin.close();
              data->mtd = new Metadata(mtd_buff, mtd_size);
              delete[] mtd_buff;
            }
          else
            data->mtd = new Metadata();
        }
      else
        data = udata.at(user_dir);
      data->lock.lock();
      data->handles.insert(netmsg);
      data->lock.unlock();
      udata_lock.unlock();

      // Make sure the data directory exists
      mkdir(user_dir.c_str(), 0755);

      while (true)
        {
          // Wait to process the message
          Message *msg = netmsg->wait_new();
          uint8_t *ret = (uint8_t*)msg->get().data(), cmd;
          size_t ret_len = msg->get().length();
          cmd = Read::i8(ret, ret_len);

          // Exit the loop if the message is quit
          if (cmd == CMD_QUIT)
            {
              netmsg->destroy(msg);
              break;
            }

          // Lock the struct to prevent changes
          data->lock.lock();
          exec_command(user_dir + "/", msg, netmsg, data);

          // Save the metadata after each call
          mtd_buff = data->mtd->serialize(mtd_size);
          std::ofstream fout(mtd_name, std::ios::out | std::ios::binary);
          fout.write((char*)mtd_buff, mtd_size);
          fout.close();
          delete[] mtd_buff;

          data->lock.unlock();
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
      data->lock.lock();

      // Erase the handle to our current netmsg
      data->handles.erase(netmsg);
      if (data->handles.size() == 0)
        {
          data->lock.unlock();

          // Erase the data struct if all clients disconnect
          delete data->mtd;
          delete data;
          udata.erase(user_dir);
        }
      else
        data->lock.unlock();
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
