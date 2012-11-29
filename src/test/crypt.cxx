/*
  Crypto Engine test suite

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

#include <sstream>
#include <cstdio>
#include <gtest/gtest.h>
#include "crypt.hxx"

#define KEY "i am awesome"

TEST(CryptTest, EncLen)
{
  Crypt c(KEY);
  EXPECT_EQ(32, c.enc_len(0));
  EXPECT_EQ(32, c.enc_len(2));
  EXPECT_EQ(32, c.enc_len(5));
  EXPECT_EQ(48, c.enc_len(16));
  EXPECT_EQ(128, c.enc_len(110));
}

TEST(CryptTest, HashLen)
{
  Crypt c(KEY);
  EXPECT_EQ(64, c.hash_len());
}

TEST(CryptTest, Hash)
{
  Crypt c(KEY);
}

TEST(CryptTest, SignSuccess)
{
  Crypt c(KEY);
}

TEST(CryptTest, SignFail)
{
  Crypt c(KEY);
}

TEST(CryptTest, EncDecReg)
{
  Crypt c(KEY);
  std::string in;
  in.resize(64);
  EXPECT_EQ(in, c.decrypt(c.encrypt(in)));
}

TEST(CryptTest, EncDecIrreg)
{
  Crypt c(KEY);
  std::string in("i am a random str");
  EXPECT_EQ(in, c.decrypt(c.encrypt(in)));
}

TEST(CryptTest, EncDecFail)
{
  Crypt c(KEY);
  EXPECT_ANY_THROW(c.decrypt("i am a random str"));
}

TEST(CryptTest, Copy)
{
  Crypt c(KEY), d("i other");
  d = c;
  std::string in("i am a random str");
  EXPECT_EQ(in, d.decrypt(c.encrypt(in)));
}

TEST(CryptTest, EncStreamShort)
{
  Crypt c(KEY);
  CryptStream *cs = c.ecstream();
  std::string in = "I am awesome";
  size_t len = c.enc_len(in.length()) + c.hash_len(), data_len = len;
  char *data = new char[len], *tmp_data = data;
  ssize_t red;

  // Create the encrypted contents
  cs->write(in.data(), in.length());
  cs->write(NULL, 0);
  while (data_len > 0)
    {
      red = cs->read(tmp_data, data_len);
      ASSERT_LT(0, red);
      tmp_data += red;
      data_len -= red;
    }

  // Check proper message hashing
  std::string hash = c.sign(in), hash2(data + len - c.hash_len(), c.hash_len());
  EXPECT_EQ(hash, hash2);

  // Check that decrypted message matches
  std::string out(data, len - c.hash_len());
  out = c.decrypt(out);

  EXPECT_EQ(in, out);

  delete [] data;
  delete cs;
}

TEST(CryptTest, DecStreamShort)
{
  Crypt c(KEY);
  CryptStream *cs = c.dcstream();
  std::string in = "I am awesome", enc = c.encrypt(in) + c.sign(in);
  size_t len = in.length(), data_len = len;
  char *data = new char[len], *tmp_data = data;
  ssize_t red;

  // Create the encrypted contents
  try
    {
      cs->write(enc.data(), enc.length());
      cs->write(NULL, 0);
      while (data_len > 0)
        {
          red = cs->read(tmp_data, data_len);
          ASSERT_LT(0, red);
          tmp_data += red;
          data_len -= red;
        }
    }
  catch(const char * e)
    {
      std::cerr << e << std::endl;
      throw "";
    }

  // Check that decrypted message matches
  std::string out(data, len);
  EXPECT_EQ(in, out);

  delete [] data;
  delete cs;
}

TEST(CryptTest, DecStreamFailGarbage)
{
  Crypt c(KEY);
  CryptStream *cs = c.dcstream();
  std::string enc = "Impossible" + c.sign("blah");

  // Create the encrypted contents
  cs->write(enc.data(), enc.length());
  ASSERT_ANY_THROW(cs->write(NULL, 0));

  delete cs;
}

TEST(CryptTest, DecStreamFailSig)
{
  Crypt c(KEY);
  CryptStream *cs = c.dcstream();
  std::string in = "I am awesome", enc = c.encrypt(in) + c.sign("blah");

  // Create the encrypted contents
  cs->write(enc.data(), enc.length());
  ASSERT_ANY_THROW(cs->write(NULL, 0));

  delete cs;
}
