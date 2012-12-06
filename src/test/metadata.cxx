/*
  Metadata test suite

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
#include "metadata.hxx"
#include <iostream>

TEST(MetadataTest, BasicDir)
{
  Metadata meta("test/config");

  Metadata::Data f = meta.get_file("blarg");
  EXPECT_EQ(0, f.modified);

  f = meta.get_file("/basic");
  EXPECT_LT(0, f.modified);
  EXPECT_FALSE(f.deleted);
}

TEST(MetadataTest, InvalidDir)
{
  ASSERT_ANY_THROW(Metadata("test/rsdfsdf"));
}

TEST(MetadataTest, BasicSerial)
{
  Metadata meta("test/config");
  size_t len;
  uint8_t * serial = meta.serialize(len);
  Metadata meta2(serial, len);
  delete serial;

  Metadata::Data f = meta2.get_file("/basic");
  EXPECT_LT(0, f.modified);
  EXPECT_FALSE(f.deleted);
}

TEST(MetadataTest, BasicMods)
{
  Metadata meta("test/config");
  meta.delete_file("/basic", 10);
  meta.new_file("/bin", 0, 11);

  Metadata::Data f = meta.get_file("/basic");
  EXPECT_EQ(10, f.modified);
  EXPECT_TRUE(f.deleted);

  f = meta.get_file("/bin");
  EXPECT_EQ(11, f.modified);
  EXPECT_FALSE(f.deleted);
}

TEST(MetadataTest, ComplexSerial)
{
  Metadata meta("test/config");
  meta.delete_file("/basic", 10);
  meta.new_file("/bin", 0, 11);
  size_t len;
  uint8_t * serial = meta.serialize(len);
  Metadata meta2(serial, len);
  delete serial;

  Metadata::Data f = meta2.get_file("/basic");
  EXPECT_EQ(10, f.modified);
  EXPECT_TRUE(f.deleted);

  f = meta2.get_file("/bin");
  EXPECT_EQ(11, f.modified);
  EXPECT_FALSE(f.deleted);
}
