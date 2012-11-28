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

#include <cctype>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "config.hxx"

#define BUFF_SIZE 2048

Config::Config()
{}

Config::Config(const std::string & filename)
{
  read(filename);
}

void Config::read(const std::string & filename)
{
  bool escaped = false;
  bool quote_open = false;
  bool key_segment = true;
  bool commented = false;
  bool line_started = false;
  std::string key, value;
  size_t value_size = 0, key_size = 0, value_current = 0, key_current = 0;

  char buff[BUFF_SIZE];
  ssize_t read;

  std::ifstream file(filename, std::ios::in | std::ios::binary);
  if (file.fail())
    throw "Failed to open the configuration file";

  while((read = file.readsome(buff, 2048)) > 0)
    for (size_t i = 0; i < read; i++)
      if (buff[i] == '\n' && !escaped)
        if (quote_open || (key_segment && line_started))
          throw "Invalid Configuration File";
        else
          {
            key.resize(key_current);
            value.resize(value_current);
            conf[key] = value;

            key.clear();
            key_current = 0;
            key_size = 0;

            value.clear();
            value_current = 0;
            value_size = 0;

            key_segment = true;
            line_started = false;
            commented = false;
          }
      else if (commented)
        continue;
      else if (buff[i] == '\\' && !escaped)
        escaped = true;
      else if (buff[i] == '"' && !escaped)
        quote_open = !quote_open;
      else if (buff[i] == '#' && !quote_open && !escaped)
        commented = true;
      else if (buff[i] == '=' && !quote_open && !escaped)
        {
          key_segment = false;
          line_started = false;
        }
      else if (quote_open || escaped || line_started || !isspace(buff[i]))
        {
          line_started = true;
          escaped = false;
          if (key_segment)
            {
              key += buff[i];
              key_size++;
            }
          else
            {
              value += buff[i];
              value_size++;
            }
          if (!isspace(buff[i]) || escaped || quote_open)
            {
              if (key_segment)
                key_current = key_size;
              else
                value_current = value_size;
            }
        }

  file.close();

  // Check for a potential read error
  if (read < 0)
    throw "Failed to read from file";

  // We may have a key value pair left
  if (quote_open)
    throw "Invalid Configuration File";
  if (!key_segment)
    conf[key] = value;
}

bool Config::exists(const std::string & key) const
{
  return conf.count(key);
}

std::string Config::get_str(const std::string & key) const
{
  if (conf.count(key))
    return conf.at(key);
  return std::string();
}

long Config::get_int(const std::string & key) const
{
  if (conf.count(key))
    return std::stol(conf.at(key));
  return double();
}

double Config::get_double(const std::string & key) const
{
  if (conf.count(key))
    return std::stod(conf.at(key));
  return double();
}
