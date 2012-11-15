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
#include "log.hxx"

#define IN_ATTR IN_ATTRIB | IN_CLOSE_WRITE | IN_CREATE | \
  IN_DELETE | IN_MODIFY | IN_MOVE

#define BUFF 4096

Watchdog::Watchdog()
  : closed(false)
{
  inotify = inotify_init();
  if (inotify < 0)
    throw std::string("inotify_init: ") + strerror(errno);
}

Watchdog::~Watchdog()
{
  close();
}

static void local_close(int fd)
{
  close(fd);
}

void Watchdog::close()
{
  if (!closed)
    {
      for (auto it = paths.begin(); it != paths.end(); it++)
        inotify_rm_watch(inotify, it->second);
      local_close(inotify);
      closed = true;
    }
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

void Watchdog::disregard(const std::string & path)
{
  no_notify_lock.lock();
  no_notify.insert(path);
  no_notify_lock.unlock();
}

void Watchdog::regard(const std::string & path)
{
  no_notify_lock.lock();
  no_notify.erase(path);
  no_notify_old[path] = time(NULL);
  no_notify_lock.unlock();
}

Watchdog::Data Watchdog::wait()
{
  struct inotify_event * event = NULL;
  std::string event_str;
  Data data;
  struct stat stats;

  // Get events until we have a valid one
  no_notify_lock.lock();
  while (event == NULL || event->mask & IN_IGNORED ||
         no_notify.count(data.filename) > 0 ||
         no_notify_old[data.filename] >= data.modified)
    {
      no_notify_lock.unlock();
      event_str = gather_event();
      event = (struct inotify_event *)event_str.c_str();

      data.filename = wds.at(event->wd) + "/" + event->name;

      // Parse the event into data struct
      if (event->mask & (IN_DELETE | IN_DELETE_SELF | IN_MOVED_FROM))
        {
          data.status = FileStatus::deleted;
          data.modified = time(NULL);
          data.directory = false;
          data.size = 0;
        }
      else
        {
          data.status = FileStatus::modified;
          if (stat(data.filename.c_str(), &stats) < 0)
            throw "Failed to get file stats";

          data.modified = stats.st_mtime;
          data.directory = S_ISDIR(stats.st_mode);
          data.size = stats.st_size;
        }

      no_notify_lock.lock();
    }
  no_notify_old.erase(data.filename);
  no_notify_lock.unlock();

  global_log.message("Change event" + event->wd, 3);

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
