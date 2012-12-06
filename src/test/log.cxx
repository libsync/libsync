/*
  Log file test suite

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

#include <fstream>
#include <sstream>
#include <cstdio>
#ifdef WIN32
#  include <regex>
#  define boost std
#else
#  include <boost/regex.hpp>
#endif

#include "gtest/gtest.h"
#include "log.hxx"
#include "util.hxx"

TEST(LogTest, Trivial)
{
  std::stringstream ss;
  Log lg;
  lg.add_output(&ss);
  lg.message("Awesome", 1);

  std::string reg("^\\[Level 1\\]\\[[^]]*\\]: Awesome\n$");
  EXPECT_TRUE(boost::regex_match(ss.str(), boost::regex(reg)));
}

TEST(LogTest, LevelHandling)
{
  Log lg(3);
  std::stringstream ss;
  lg.add_output(&ss);
  lg.message("Awesome", 3);
  lg.message("Awesome2", 2);
  lg.message("Awesome2", 4);

  std::string reg("^\\[Level 3\\]\\[[^]]*\\]: Awesome\n");
  reg += "\\[Level 2\\]\\[[^]]*\\]: Awesome2\n$";
  EXPECT_TRUE(boost::regex_match(ss.str(), boost::regex(reg)));
}

TEST(LogTest, LevelChanging)
{
  Log lg(3);
  std::stringstream ss;
  lg.add_output(&ss);
  lg.message("Awesome", 3);
  lg.message("Awesome2", 2);
  lg.set_level(2);
  lg.message("Awesome2", 4);
  lg.message("Awesome", 3);

  std::string reg("^\\[Level 3\\]\\[[^]]*\\]: Awesome\n");
  reg += "\\[Level 2\\]\\[[^]]*\\]: Awesome2\n$";
  EXPECT_TRUE(boost::regex_match(ss.str(), boost::regex(reg)));
}

void read_file(const char * filename, std::stringstream & ss)
{
  std::ifstream file(filename, std::ios::in | std::ios::binary);
  char buffer[2048];
  ssize_t size;
  while((size = file.readsome(buffer, 2048)) > 0)
    ss.write(buffer, size);
  file.close();
  remove(filename);
}

TEST(LogTest, FileOut)
{
  Log *lg = new Log;
  lg->add_output("test.out");
  lg->message("Awesome", 1);
  delete lg;

  std::stringstream ss;
  read_file("test.out", ss);

  std::string reg("^\\[Level 1\\]\\[[^]]*\\]: Awesome\n$");
  EXPECT_TRUE(boost::regex_match(ss.str(), boost::regex(reg)));
}

TEST(LogTest, MultiOut)
{
  Log *lg = new Log;
  std::stringstream ss;
  lg->add_output("test.out");
  lg->add_output(&ss);
  lg->message("Awesome", 1);
  delete lg;

  std::stringstream ss2;
  read_file("test.out", ss2);

  std::string reg("^\\[Level 1\\]\\[[^]]*\\]: Awesome\n$");
  EXPECT_TRUE(boost::regex_match(ss.str(), boost::regex(reg)));
  EXPECT_TRUE(boost::regex_match(ss2.str(), boost::regex(reg)));
}
