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

#include <unordered_map>

#include "metadata.hxx"

Metadata::Metadata(uint8_t * data, size_t size)
{
}

Metadata::Metadata(const std::string & path)
{
}

uint8_t * Metadata::serialize(size_t & size)
{
}

std::unordered_map<std::string, Metadata::Data>::const_iterator
Metadata::begin() const
{
  return files.begin();
}

std::unordered_map<std::string, Metadata::Data>::const_iterator
Metadata::end() const
{
  return files.end();
}

Metadata::Data Metadata::get_file(const std::string & filename) const
{
  if (files.count(filename) == 0)
    return Data();
  return files.at(filename);
}

void Metadata::new_file(const std::string & filename, uint64_t modified)
{
  files[filename].modified = modified;
  files[filename].deleted = false;
}

void Metadata::modify_file(const std::string & filename, uint64_t modified)
{
  files[filename].modified = modified;
  files[filename].deleted = false;
}

void Metadata::delete_file(const std::string & filename, uint64_t modified)
{
  files[filename].modified = modified;
  files[filename].deleted = true;
}
