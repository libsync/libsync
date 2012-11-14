/*
  A helper for managing user accounts on the server

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

#include "user.hxx"
#include "../src/log.hxx"
#include "../src/util.hxx"

#define BUFF 2048

User::User(const std::string & save_dir)
  : save_dir(save_dir), next_id(0)
{
  try
    {
      open(save_dir + "/login.mtd");
    }
  catch (const std::string & e)
    {
      next_id = 0;
    }
  catch (const char * e)
    {
      next_id = 0;
    }
}

std::string User::login(const std::string & user, const std::string & pass)
{
  lock.lock();

  // Check if the user exists
  if (info.count(user) < 1)
    {
      lock.unlock();
      throw "The user doesn't exists";
    }

  // Check if the password matches
  Data d = info[user];
  if (d.pass != pass)
    {
      lock.unlock();
      throw "The password is not valid for that user";
    }

  lock.unlock();

  return save_dir + "/" + std::to_string(d.id);
}

std::string User::reg(const std::string & user, const std::string & pass)
{
  lock.lock();

  // Make sure the user doesn't exist
  if (info.count(user) > 0)
    {
      lock.unlock();
      throw "User already exists";
    }

  Data d;
  d.id = next_id++;
  d.pass = pass;

  info[user] = d;
  save(save_dir + "/login.mtd");

  lock.unlock();

  return save_dir + "/" +  std::to_string(d.id);
}

void User::open(const std::string & filename)
{
  char buff[BUFF];
  ssize_t red;
  std::string input;
  uint8_t * data;
  size_t size;

  // Try reading the file
  std::ifstream file(filename, std::ios::in | std::ios::binary);
  if (file.fail())
    throw std::string("Failed to read user data from: ") + filename;
  while((red = file.readsome(buff, BUFF)) > 0)
    input.append(buff, red);
  file.close();
  data = (uint8_t*)input.data();
  size = input.length();

  // Get the next id
  next_id = Read::i64(data, size);

  // Get the size of the map
  size_t count = Read::i64(data, size);

  while(count > 0)
    {
      // Get the id
      uint64_t id = Read::i64(data, size);

      // Get the username
      size_t len = Read::i64(data, size);
      if (size < len)
        throw "Not enough data to deserialize login";
      std::string user;
      user.append((char*)data, len);
      data += len;
      size -= len;

      // Get the rest of the data
      Data d;
      d.id = id;

      len = Read::i64(data, size);
      if (size < len)
        throw "Not enough data to deserialize login";
      d.pass.append((char*)data, len);
      data += len;
      size -= len;

      // Append the user data
      info[user] = d;

      count--;
    }
}

void User::save(const std::string & filename)
{
  std::string out;

  // Append the next id
  uint64_t enid = htobe64(next_id);
  out.append((char*)&enid, 8);

  // Append the size
  size_t len = htobe64(info.size());
  out.append((char*)&len, 8);

  for (auto it = info.begin(), end = info.end(); it != end; it++)
    {
      // Write out the id
      uint64_t id = htobe64(it->second.id);
      out.append((char*)&id, 8);

      // Write out the username
      len = htobe64(it->first.length());
      out.append((char*)&len, 8);
      out.append(it->first);

      // Write out the password
      len = htobe64(it->second.pass.length());
      out.append((char*)&len, 8);
      out.append(it->second.pass);
    }

  // Write the serialize bytes to the file
  std::ofstream file(filename, std::ios::out | std::ios::binary);
  if (file.fail())
    throw std::string("Failed to write user data to: ") + filename;
  file.write(out.data(), out.length());
  file.close();
}
