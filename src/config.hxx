/*
  Configuration File Parser

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

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <string>
#include <unordered_map>

class Config
{
public:
  /**
   * Creates an empty config(without keys)
   */
  Config();

  /**
   * Creates a config from a file
   * @param filename The name of the file to read
   */
  Config(const std::string & filename);

  /**
   * Reads the file and merges it with the current config
   * @param filename The name of the file to read
   */
  void read(const std::string & filename);

  /**
   * Returns whether the specified key exists in the config
   * @param key The string representation of the key value
   * @return True if the key exists
   */
  bool exists(const std::string & key) const;

  /**
   * Returns the string value of the given key
   * @param key The string representation of the key value
   * @return A copy of the string value for the parameter
   */
  std::string get_str(const std::string & key) const;

  /**
   * Returns the integer value of the given key
   * @param key The string representation of the key value
   * @return An integer representation of the value at key if possible
   * @throws An exception if an integer cannot be parsed
   */
  long get_int(const std::string & key) const;

  /**
   * Returns the decimal value of the given key
   * @param key The string representation of the key value
   * @return A double representation of the value at the key if possible
   * @throws An exception if a double cannot be parsed
   */
  double get_double(const std::string & key) const;

private:
  std::unordered_map<std::string, std::string> conf;
};

#endif
