/*
  Generic Remote Connector Module

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

#ifndef __CONNECTOR_HXX__
#define __CONNECTOR_HXX__

#include <string>
#include <istream>
#include <fstream>
#include "metadata.hxx"

class Connector
{
public:
  virtual void close() = 0;
  virtual Metadata * get_metadata() = 0;
  virtual void push_file(const std::string & filename, uint64_t modified,
                 std::istream & data, size_t data_size) = 0;
  virtual void get_file(const std::string & filename, uint64_t & modified,
                std::ostream & data) = 0;
  virtual void delete_file(const std::string & filename,
                           uint64_t modified) = 0;
  virtual std::pair<std::string, Metadata::Data> wait() = 0;
};

#endif
