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

#include <openssl/evp.h>
#include <sys/mman.h>

#include "crypt.hxx"

Crypt::Crypt(const std::string & key)
{
  key_len = key.length();
  this->key = (unsigned char *) malloc(key_len);
  mlock(this->key, key_len);
  memcpy(this->key, key.data(), key_len);
}

Crypt::Crypt(const Crypt & crypt)
{
  copy(crypt);
}

Crypt::~Crypt()
{
  clear();
}

Crypt & Crypt::operator=(const Crypt & crypt)
{
  if (this == &crypt)
    return *this;

  clear();
  copy(crypt);
}

std::iostream * Crypt::wrap(const std::iostream & stream)
{
  return NULL;
}

std::istream * Crypt::wrap(const std::istream & stream)
{
  return NULL;
}

std::ostream * Crypt::wrap(const std::ostream & stream)
{
  return NULL;
}

std::string Crypt::encrypt(const std::string & ptext)
{
  return "";
}

std::string Crypt::decrypt(const std::string & ctext)
{
  return "";
}

std::string Crypt::hash(const std::string & msg)
{
  return "";
}

std::string Crypt::sign(const std::string & msg)
{
  return "";
}

bool Crypt::check(const std::string & sig)
{
  return false;
}

size_t Crypt::enc_len(size_t len)
{
  return 0;
}

size_t Crypt::hash_len()
{
  return 0;
}

void Crypt::copy(const Crypt & crypt)
{
  key_len = crypt.key_len;
  key = (unsigned char *) malloc(key_len);
  mlock(key, key_len);
  memcpy(key, crypt.key, key_len);
}

void Crypt::clear()
{
  munlock(key, key_len);
  free(key);
}
