/*
  Metadata storage class

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

#ifndef __METADATA_HXX__
#define __METADATA_HXX__

#include <cstdint>
#include <string>
#include <unordered_map>

class Metadata
{
public:
  struct Data
  {
    uint64_t modified;
    bool deleted;
  };

  Metadata();
  Metadata(uint8_t * data, size_t size);
  Metadata(const std::string & path);
  uint8_t * serialize(size_t & size);

  std::unordered_map<std::string, Data>::const_iterator begin() const;
  std::unordered_map<std::string, Data>::const_iterator end() const;
  Data get_file(const std::string & filename) const;

  void new_file(const std::string & filename, uint64_t modified);
  void modify_file(const std::string & filename, uint64_t modified);
  void delete_file(const std::string & filename, uint64_t modified);
private:
  std::unordered_map<std::string, Data> files;
  void build(const std::string & rootpath, const std::string & path);
};

#endif
