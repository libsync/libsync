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

#include <sstream>
#include "connector_sock.hxx"
#include "util.hxx"
#include "log.hxx"

#define HAND_LOGIN 0
#define HAND_REG 1

#define REG_EXISTS 1
#define REG_CLOSED 2

#define LOGIN_INV 1

#define CMD_QUIT 0
#define CMD_META 1
#define CMD_PUSH 2
#define CMD_PULL 3
#define CMD_DEL 4

#define BUFF 2048

SockConnector::SockConnector(const std::string & host, uint16_t port,
                             const std::string & user, const std::string & pass,
                             bool reg)
  : closed(false), client(host, port), user(user), pass(pass),
  net(NULL), netmsg(NULL), crypt(NULL)
{
  connect(reg);
}

SockConnector::SockConnector(const std::string & host, uint16_t port,
                             const std::string & user, const std::string & pass,
                             const std::string & key,
                             bool reg)
  : closed(false), client(host, port), user(user), pass(pass),
    net(NULL), netmsg(NULL), crypt(new Crypt(key))
{
  connect(reg);
}

SockConnector::~SockConnector()
{
  close();
  delete netmsg;
  delete net;
}

void SockConnector::close()
{
  if (!closed)
    {
      std::string cmd;
      Write::i8(CMD_QUIT, cmd);
      netmsg->send_only(cmd);
      netmsg->close();
      closed = true;
    }
}

Metadata * SockConnector::get_metadata()
{
  // Ask the server for the metadata
  std::string cmd;
  Write::i8(CMD_META, cmd);
  Message *msg = netmsg->send_and_wait(cmd);

  global_log.message(std::to_string(msg->get().length()), Log::NOTICE);
  Metadata * ret = new Metadata((uint8_t*)msg->get().data(),
                                msg->get().length());

  netmsg->destroy(msg);

  return ret;
}

void SockConnector::push_file(const std::string & filename, uint64_t modified,
                              std::istream & data, size_t data_size)
{
  // Send the command info
  std::string cmd;

  Write::i8(CMD_PUSH, cmd);
  Write::i64(modified, cmd);
  Write::i32(filename.length(), cmd);
  cmd.append(filename);

  Message * msg = netmsg->send_and_wait(cmd);
  uint8_t *ret = (uint8_t*)msg->get().data();
  size_t ret_len = msg->get().length();
  if (Read::i8(ret, ret_len) != 0)
    {
      netmsg->destroy(msg);
      global_log.message(std::string("Server Skipped: ") + filename,
                         Log::NOTICE);
      return;
    }

  // Buffer the contents in memory
  int64_t red;
  char buff[BUFF];
  std::stringstream ss;
  std::cout << "Started Buffering" << std::endl;
  if (crypt == NULL)
    while((red = data.readsome(buff, BUFF)) > 0)
      ss.write(buff, red);
  else
    {
      // Write all of the data into the crypto stream
      data_size = crypt->enc_len(data_size) + crypt->hash_len();
      CryptStream *cs = crypt->ecstream();
      while ((red = data.readsome(buff, BUFF)) > 0)
          cs->write(buff, red);

      // Write the encrypted data to the buffer
      cs->write(NULL, 0);
      while((red = cs->read(buff, BUFF)) > 0)
        ss.write(buff, red);

      delete cs;
    }

  std::cout << "Finished Buffering: " << ss.tellp()-ss.tellg() << std::endl;

  // Send the file contents
  msg = netmsg->reply_and_wait(msg, &ss, data_size);
  ret = (uint8_t*)msg->get().data();
  ret_len = msg->get().length();
  if (Read::i8(ret, ret_len) != 0)
    {
      netmsg->destroy(msg);
      throw "Failed to push file";
    }
  netmsg->destroy(msg);
}

void SockConnector::get_file(const std::string & filename, uint64_t & modified,
                             std::ostream & data)
{
  // Send the command info
  std::string cmd;
  Write::i8(CMD_PULL, cmd);
  Write::i32(filename.length(), cmd);
  cmd.append(filename);

  Message *msg = netmsg->send_and_wait(cmd);
  uint8_t *ret = (uint8_t*)msg->get().data();
  size_t ret_len = msg->get().length();
  if (Read::i8(ret, ret_len) != 0)
    {
      netmsg->destroy(msg);
      throw "Failed to retrieve file";
    }

  // Get the modification time
  modified = Read::i64(ret, ret_len);

  // Get the file contents
  std::stringstream ss;
  cmd.clear();
  Write::i8(0, cmd);
  msg->set(cmd);
  netmsg->reply_and_wait(msg, &ss);
  msg->set(cmd);
  netmsg->reply_only(msg);

  // Buffer the contents in memory
  int64_t red;
  char buff[BUFF];
  if (crypt == NULL)
    while((red = ss.readsome(buff, BUFF)) > 0)
      data.write(buff, red);
  else
    {
      // Write the file into the crypto stream
      CryptStream *cs = crypt->dcstream();
      while ((red = ss.readsome(buff, BUFF)) > 0)
        cs->write(buff, red);
      cs->write(NULL, 0);

      // Write the decrypted file
      while((red = cs->read(buff, BUFF)) > 0)
        data.write(buff, red);
      delete cs;
    }
}

void SockConnector::delete_file(const std::string & filename,
                                uint64_t modified)
{
  // Send the command info
  std::string cmd;
  Write::i8(CMD_DEL, cmd);
  Write::i64(modified, cmd);
  Write::i32(filename.length(), cmd);
  cmd.append(filename);

  Message *msg = netmsg->send_and_wait(cmd);
  uint8_t *ret = (uint8_t*)msg->get().data();
  size_t ret_len = msg->get().length();
  if (Read::i8(ret, ret_len) != 0)
    throw "Server failed to delete file";
  netmsg->destroy(msg);
}

std::pair<std::string, Metadata::Data> SockConnector::wait()
{
  // Wait for a message from the server
  Message *msg = netmsg->wait_new();

  // Decompose the data
  uint8_t *data = (uint8_t*)msg->get().data();
  size_t data_len = msg->get().length();

  size_t name_len = Read::i32(data, data_len);
  std::string name((char*)data, name_len);
  data += name_len;
  data_len -= name_len;

  Metadata::Data mtd;
  mtd.size = 0;
  mtd.modified = Read::i64(data, data_len);
  mtd.deleted = (bool)Read::i8(data, data_len);

  // Write a response saying we received the update
  std::string cmd;
  Write::i8(0, cmd);
  msg->set(cmd);
  netmsg->reply_only(msg);

  return std::pair<std::string, Metadata::Data>(name, mtd);
}

void SockConnector::connect(bool reg)
{
  net = client.connect();

  // Check for compatible version
  int ver = net->read8();
  if (ver != 0)
    {
      delete net;
      net = NULL;
      throw "Incompatible Server Version";
    }

  // Send login / register
  if (reg)
    net->write8(HAND_REG);
  else
    net->write8(HAND_LOGIN);

  // Send credentials
  net->write16(user.length());
  net->write(user);
  net->write16(pass.length());
  net->write(pass);

  // Get Response Code
  int ret = net->read8();
  if (reg)
    {
      if (ret == REG_EXISTS)
        throw "User already exists";
      else if (ret == REG_CLOSED)
        throw "Registration is closed";
    }
  else
    {
      if (ret == LOGIN_INV)
        throw "Invalid Username or Password";
    }

  netmsg = new NetMsg(net);
  netmsg->start();
}
