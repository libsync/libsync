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

#include "connector_sock.hxx"

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
  : client(host, port), user(user), pass(pass), net(NULL)
{
  connect(reg);
}

SockConnector::~SockConnector()
{
  delete net;
}

Metadata * SockConnector::get_metadata()
{
  // Ask the server for the metadata
  net->write8(CMD_META);

  // Allocate a buffer based on the metadata length
  size_t buff_size = net->read32();
  uint8_t * buff = new uint8_t[buff_size];

  // Read the metadata
  net->read_all(buff, buff_size);

  Metadata * ret = new Metadata(buff, buff_size);
  delete[] buff;

  return ret;
}

void SockConnector::push_file(const std::string & filename, uint64_t modified,
                              std::istream & data, size_t data_size)
{
  // Send the command info
  net->write8(CMD_PUSH);
  net->write64(modified);
  net->write32(filename.length());
  net->write64(data_size);

  net->write(filename);

  // Write data block by block until it's done
  uint8_t buff[BUFF];
  while (data_size > 0)
    {
      ssize_t buff_len = data.readsome((char *)buff, BUFF);
      if (buff_len <= 0)
        throw "Failed to read data from the file";
      net->write(buff, buff_len);
      data_size -= buff_len;
    }

  // Check for success
  if (net->read8() != 0)
    throw "Failed to send file to the server";
}

void SockConnector::get_file(const std::string & filename, uint64_t & modified,
                             std::ostream & data)
{
  // Send the command info
  net->write8(CMD_PULL);
  net->write32(filename.length());
  net->write(filename);

  // Get the modification time
  modified = net->read64();

  // Get the file contents
  size_t data_len = net->read64();
  uint8_t buff[BUFF];
  while(data_len > 0)
    {
      size_t read = data_len > BUFF ? BUFF : data_len;
      net->read_all(buff, read);
      data.write((char *)buff, read);
      data_len -= read;
    }
}

void SockConnector::delete_file(const std::string & filename,
                                uint64_t & modified)
{
  // Send the command info
  net->write8(CMD_DEL);
  net->write64(modified);
  net->write32(filename.length());
  net->write(filename);

  if (net->read8() != 0)
    throw "Server failed to delete file";
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
}
