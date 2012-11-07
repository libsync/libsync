/*
  Configuration Parser test suite

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

#include <gtest/gtest.h>
#include "config.hxx"

TEST(ConfigTest, Basic)
{
  Config conf("test/config/basic");

  EXPECT_EQ("awesome", conf.get_str("basic"));
}

TEST(ConfigTest, Many)
{
  Config conf("test/config/many");

  EXPECT_EQ("win1", conf.get_str("first"));
  EXPECT_EQ("win2", conf.get_str("second"));
  EXPECT_EQ("win3", conf.get_str("third"));
}

TEST(ConfigTest, Comment)
{
  Config conf("test/config/comment");

  EXPECT_EQ("awesome", conf.get_str("basic"));
  EXPECT_EQ("3awesome", conf.get_str("basic2"));
}

TEST(ConfigTest, Duplicate)
{
  Config conf("test/config/duplicate");

  EXPECT_EQ("win2", conf.get_str("basic"));
}

TEST(ConfigTest, Quotes)
{
  Config conf("test/config/quotes");

  EXPECT_EQ("ten", conf.get_str("str2"));
  EXPECT_EQ(1, conf.get_int("str"));
  EXPECT_EQ("g\"g", conf.get_str("str3"));
}

TEST(ConfigTest, Whitespace)
{
  Config conf("test/config/whitespace");

  EXPECT_EQ(" hi ", conf.get_str("str"));
  EXPECT_EQ("hi", conf.get_str("str2"));
  EXPECT_EQ("sent\t ance", conf.get_str("str3"));
}

TEST(ConfigTest, Merge)
{
  Config conf("test/config/merge1");
  conf.read("test/config/merge2");

  EXPECT_EQ("basic", conf.get_str("str"));
  EXPECT_EQ("basic2", conf.get_str("str2"));
  EXPECT_EQ("basic3", conf.get_str("str3"));
}

TEST(ConfigTest, InvalidQuote)
{
  Config conf;
  ASSERT_ANY_THROW(conf.read("test/config/invqu"));
}

TEST(ConfigTest, InvalidEqual)
{
  Config conf;
  ASSERT_ANY_THROW(conf.read("test/config/inveq"));
}
