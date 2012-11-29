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

TEST(CryptTest, InStreamShort)
{
  Crypt c(KEY);
  std::stringstream ss;
  std::string in = "I am awesome";
  size_t len = c.enc_len(in.length()) + c.hash_len();
  std::istream *stream = c.wrap((std::istream*)&ss);

  return;

  // Create stream output
  ss << in;

  // Create the encrypted contents
  char *data = new char [len], *tmp = data;
  size_t tmp_len = len;
  std::streamsize red;

  while(tmp_len > 0)
    {
      std::cout << tmp_len << std::endl;
      red = stream->readsome(tmp, tmp_len);
      ASSERT_LT(0, red);
      tmp_len -= red;
      tmp += red;
    }
  delete stream;

  // Check proper message hashing
  std::string hash = c.sign(in), hash2(data + len - c.hash_len(), c.hash_len());
  EXPECT_EQ(hash, hash2);

  // Check that decrypted message matches
  std::string out(data, len - c.hash_len());
  out = c.decrypt(out);

  EXPECT_EQ(in, out);

  delete [] data;
}

TEST(CryptTest, InStreamLong)
{
}

TEST(CryptTest, StreamFailRand)
{
}

TEST(CryptTest, StreamFailSig)
{
}
