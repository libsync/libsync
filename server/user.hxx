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

#ifndef __USER_HXX__
#define __USER_HXX__

#include <cstdint>
#include <string>
#include <mutex>
#include <unordered_map>

class User
{
public:
  /**
   * Creates a new user instance which loads data from the save directory
   * @param save_dir The directory where the saved user data resides
   */
  User(const std::string & save_dir);

  /**
   * Checks to see if the login credentials are valid
   * @param user The name of the user
   * @param pass The password of the user
   * @return The data directory prefix of the user
   * @throws An exception if the user credentials are invalid
   */
  std::string login(const std::string & user, const std::string & pass);

  /**
   * Registers the user with the server
   * @param user The name of the user
   * @param pass The password of the user
   * @return The data directory prefix of the user
   * @throws An exception if the user already exists
   */
  std::string reg(const std::string & user, const std::string & pass);

private:
  struct Data
  {
    uint64_t id;
    std::string pass;
  };

  std::string save_dir;
  std::mutex lock;
  std::unordered_map<std::string, Data> info;
  uint64_t next_id;

  /**
   * Reads the login metadata from the disk and populates info
   * @param filename The name of the file to serialize into
   */
  void open(const std::string & filename);

  /**
   * Serializes the info structure to the disk
   * @param filename The name of the file to serialize from
   */
  void save(const std::string & filename);
};

#endif
