/*
  The cryptographic stream implementation based on OpenSSL

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

#ifndef __CRYPT_HXX__
#define __CRYPT_HXX__

#include <cstring>
#include <iostream>
#include <string>

class Crypt
{
public:
  Crypt(const std::string & key);
  Crypt(const Crypt & crypt);
  ~Crypt();
  Crypt & operator=(const Crypt & crypt);

  std::iostream * wrap(const std::iostream & stream);
  std::istream * wrap(const std::istream & stream);
  std::ostream * wrap(const std::ostream & stream);

  std::string encrypt(const std::string & ptext);
  std::string decrypt(const std::string & ctext);

  std::string hash(const std::string & msg);
  std::string sign(const std::string & msg);
  bool check(const std::string & sig);

  size_t enc_len(size_t len);
  size_t hash_len();
private:
  class ocstream : public std::ostream
  {
  };

  class icstream : public std::istream
  {
  };

  class cstream : public icstream, public ocstream
  {
  };

  size_t key_len;
  unsigned char * key;
  const EVP_CIPHER *cipher;

  void copy(const Crypt & crypt);
  void clear();
  void derive_key(const std::string & mat, unsigned char *key, size_t key_len);
  void rand(unsigned char *data, size_t size);
};

#endif
