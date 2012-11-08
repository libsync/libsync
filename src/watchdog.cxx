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

#include <sys/inotify.h>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "watchdog.hxx"

#define IN_ATTR IN_ATTRIB | IN_CLOSE_WRITE | IN_CREATE | \
  IN_DELETE | IN_MODIFY | IN_MOVE

#define BUFF 4096

Watchdog::Watchdog()
{
  inotify = inotify_init();
  if (inotify < 0)
    throw std::string("inotify_init: ") + strerror(errno);
}

Watchdog::~Watchdog()
{
  for (auto it = paths.begin(); it != paths.end(); it++)
    inotify_rm_watch(inotify, it->second);
  close(inotify);
}

void Watchdog::add_watch(const std::string & path, bool recursive)
{
  // Only create a watch if it hasn't been before
  if (paths.count(path) == 0)
    {
      int wd;
      if ((wd = inotify_add_watch(inotify, path.c_str(), IN_ATTR)) < 0)
        throw std::string("inotify_add: ") + strerror(errno);

      paths[path] = wd;
      wds[wd] = path;
    }

  // Recurse into subdirectories
  if (recursive)
    {
      DIR * dir = opendir(path.c_str());
      if (dir == NULL)
        return;

      struct dirent * entry;
      while((entry = readdir(dir)) != NULL)
        if (entry->d_type == DT_DIR && strcmp(".", entry->d_name)
            && strcmp("..", entry->d_name))
          add_watch(path + "/" + entry->d_name, recursive);

      closedir(dir);
    }
}

void Watchdog::del_watch(const std::string & path)
{
  if (paths.count(path) == 0)
    throw "Watchdog: Invalid Watch to Remove";
  if (inotify_rm_watch(inotify, paths.at(path)) < 0)
    throw std::string("inotify del: ") + strerror(errno);
}

Watchdog::Data Watchdog::wait()
{
  struct inotify_event * event = NULL;
  std::string event_str;

  // Get events until we have a valid one
  while (event == NULL || event->mask & IN_IGNORED)
    {
      event_str = gather_event();
      event = (struct inotify_event *)event_str.c_str();
    }

  // Parse the event into data struct
  Data data;
  data.filename = wds.at(event->wd) + "/" + event->name;
  if (event->mask & (IN_DELETE | IN_DELETE_SELF | IN_MOVED_FROM))
    data.status = FileStatus::deleted;
  else
    data.status = FileStatus::modified;

  // Modified File
  if (data.status == FileStatus::modified)
    {
      struct stat stats;
      if (stat(data.filename.c_str(), &stats) < 0)
        throw "Failed to get file stats";

      data.modified = stats.st_mtime;
      data.directory = S_ISDIR(stats.st_mode);
      data.size = stats.st_size;
    }
  // Deleted File
  else
    {
      data.modified = time(NULL);
      data.directory = false;
      data.size = 0;
    }

  return data;
}

std::string Watchdog::gather_event()
{
  // Gather the variable system event data
  char buff[BUFF];
  struct inotify_event * event;
  ssize_t left = sizeof(struct inotify_event) - inotify_bytes.length(), red;

  // Read the first segment of the input stream
  while (left > 0)
    if ((red = read(inotify, buff, BUFF)) < 0)
      throw std::string("Failed to get inotify event: ") + strerror(errno);
    else
      {
        left -= red;
        inotify_bytes.append(buff, red);
      }

  event = (struct inotify_event *)inotify_bytes.c_str();
  size_t len = sizeof(struct inotify_event) + event->len;
  left += event->len;

  // Read the rest of the inotify struct
  while (left > 0)
    if ((red = read(inotify, buff, BUFF)) < 0)
      throw std::string("Failed to get inotify event: ") + strerror(errno);
    else
      {
        left -= red;
        inotify_bytes.append(buff, red);
      }

  // Truncate the new string
  std::string event_str = inotify_bytes.substr(0, len);
  inotify_bytes = inotify_bytes.substr(len);
  return event_str;
}
