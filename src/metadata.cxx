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
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <cstdint>
#include <string>
#include <cstring>

#include "net.hxx"
#include "metadata.hxx"
#include "log.hxx"
#include "util.hxx"

Metadata::Metadata()
{}

#include <iostream>
Metadata::Metadata(uint8_t * data, size_t size)
{
  // Get the size of the map
  size_t count = Read::i64(data, size);

  while(count > 0)
    {
      // Get the filename
      size_t len = Read::i64(data, size);
      if (size < len)
        throw "Metadata object too small to deserialize";
      std::string filename;
      filename.append((char*)data, len);
      data += len;
      size -= len;

      // Get the modified and deleted
      Data d;
      d.modified = Read::i64(data, size);
      d.deleted = Read::i8(data, size);
      d.size = Read::i64(data, size);

      // Append the file to the metadata
      files[filename] = d;

      count--;
    }
}

Metadata::Metadata(const std::string & path)
{
  build(path, "/");
}

void Metadata::build(const std::string & rootpath, const std::string & path)
{
  DIR * dir = opendir((rootpath + path).c_str());
  if (dir == NULL)
    throw "Failed to calculate metadata";

  struct dirent * entry;
  while((entry = readdir(dir)) != NULL)
    if (entry->d_type == DT_DIR && strcmp(".", entry->d_name)
        && strcmp("..", entry->d_name))
      build(rootpath, path + entry->d_name + "/");
    else if (entry->d_type == DT_REG)
      {
        struct stat stats;
        if (stat((rootpath + path + entry->d_name).c_str(), &stats) < 0)
          continue;
        Data d;
        d.modified = stats.st_mtime;
        d.deleted = false;
        d.size = stats.st_size;
        files[path + entry->d_name] = d;
      }

  closedir(dir);
}

uint8_t * Metadata::serialize(size_t & size)
{
  std::string out;

  // Append the size
  size_t len = htobe64(files.size());
  out.append((char*)&len, 8);

  for (auto it = files.begin(); it != files.end(); it++)
    {
      // Write out the filename
      len = htobe64(it->first.length());
      out.append((char*)&len, 8);
      out.append(it->first);

      // Write the attributes
      uint64_t mod = htobe64(it->second.modified);
      out.append((char*)&mod, 8);
      out.append((char*)&it->second.deleted, 1);
      Write::i64(it->second.size, out);
    }

  // Copy the serialized bytes into the output buffer
  size = out.length();
  uint8_t * final = new uint8_t[size];
  for (size_t i = 0; i < size; i++)
    final[i] = out.at(i);
  return final;
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

void Metadata::new_file(const std::string & filename, size_t size,
                        uint64_t modified)
{
  files[filename].size = size;
  files[filename].modified = modified;
  files[filename].deleted = false;

  global_log.message(std::string("New File: ") + filename, Log::NOTICE);
}

void Metadata::modify_file(const std::string & filename, size_t size,
                           uint64_t modified)
{
  files[filename].size = size;
  files[filename].modified = modified;
  files[filename].deleted = false;
  global_log.message(std::string("Modified File: ") + filename, Log::NOTICE);
}

void Metadata::delete_file(const std::string & filename, uint64_t modified)
{
  files[filename].modified = modified;
  files[filename].deleted = true;
  global_log.message(std::string("Delete File: ") + filename, Log::NOTICE);
}
