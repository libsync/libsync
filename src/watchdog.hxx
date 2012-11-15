/*
  Watchdog file change notifier

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

#ifndef __WATCHDOG_HXX__
#define __WATCHDOG_HXX__

#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

class Watchdog
{
public:
  enum FileStatus
    {
      modified = 0,
      deleted
    };
  struct Data
  {
    std::string filename;
    FileStatus status;
    uint64_t modified;
    uint64_t size;
    bool directory;
  };

  Watchdog();
  ~Watchdog();
  void close();

  void add_watch(const std::string & path, bool recursive = true);
  void del_watch(const std::string & path);
  void disregard(const std::string & path);
  void regard(const std::string & path);

  Data wait();
private:
  int inotify;
  bool closed;
  std::unordered_map<std::string, int> paths;
  std::unordered_map<int, std::string> wds;
  std::string inotify_bytes;
  std::unordered_set<std::string> no_notify;
  std::unordered_map<std::string, uint64_t> no_notify_old;
  std::mutex no_notify_lock;

  std::string gather_event();
};

#endif
