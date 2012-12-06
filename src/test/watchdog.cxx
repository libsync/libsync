/*
  Watchdog test suite

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

#include "gtest/gtest.h"
#include "watchdog.hxx"
#include "util.hxx"
#include <sys/stat.h>
#include <cstdio>
#include <iostream>

TEST(WatchdogTest, Basic)
{
  Watchdog wd;
  fs::create_directory(fs::path("test/watchdog"));
  wd.add_watch("test/watchdog");

  FILE * f = fopen("test/watchdog/basic", "w");
  fclose(f);

  Watchdog::Data d = wd.wait();
  EXPECT_EQ("test/watchdog/basic", d.filename);
  EXPECT_FALSE(d.directory);
  EXPECT_EQ(Watchdog::FileStatus::modified, d.status);

  remove("test/watchdog/basic");
  remove("test/watchdog");
}

TEST(WatchdogTest, RemoveWatch)
{
  Watchdog wd;
  fs::create_directory(fs::path("test/watchdog"));
  fs::create_directory(fs::path("test/watchdog2"));
  wd.add_watch("test/watchdog");
  wd.add_watch("test/watchdog2");
  wd.del_watch("test/watchdog");

  FILE * f = fopen("test/watchdog/basic", "w");
  fclose(f);
  f = fopen("test/watchdog2/basic", "w");
  fclose(f);

  Watchdog::Data d = wd.wait();
  EXPECT_EQ("test/watchdog2/basic", d.filename);
  EXPECT_FALSE(d.directory);
  EXPECT_EQ(Watchdog::FileStatus::modified, d.status);

  remove("test/watchdog/basic");
  remove("test/watchdog2/basic");
  remove("test/watchdog");
  remove("test/watchdog2");
}

TEST(WatchdogTest, RemoveFile)
{
  Watchdog wd;
  fs::create_directory(fs::path("test/watchdog"));
  FILE * f = fopen("test/watchdog/basic", "w");
  fclose(f);
  wd.add_watch("test/watchdog");

  remove("test/watchdog/basic");

  Watchdog::Data d = wd.wait();
  EXPECT_EQ("test/watchdog/basic", d.filename);
  EXPECT_FALSE(d.directory);
  EXPECT_EQ(Watchdog::FileStatus::deleted, d.status);

  remove("test/watchdog");
}

TEST(WatchdogTest, Recursive)
{
  Watchdog wd;
  fs::create_directory(fs::path("test/watchdog"));
  fs::create_directory(fs::path("test/watchdog/dir"));
  wd.add_watch("test/watchdog", true);

  FILE * f = fopen("test/watchdog/dir/basic", "w");
  fclose(f);

  Watchdog::Data d = wd.wait();
  EXPECT_EQ("test/watchdog/dir/basic", d.filename);
  EXPECT_FALSE(d.directory);
  EXPECT_EQ(Watchdog::FileStatus::modified, d.status);

  remove("test/watchdog/dir/basic");
  remove("test/watchdog/dir");
  remove("test/watchdog");
}
