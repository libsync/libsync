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
#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <sys/mman.h>

#include "util.hxx"
#include "crypt.hxx"

Crypt::Crypt(const std::string & key)
  : c_func(EVP_aes_256_cbc()), h_func(EVP_sha512())
{
  key_len = EVP_CIPHER_key_length(c_func);

  this->key = (unsigned char *) malloc(key_len);
  mlock(this->key, key_len);
  derive_key(key, "fh$#WEFSdf4576", 1000, this->key, key_len);
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
  if (this != &crypt)
    {
      clear();
      copy(crypt);
    }
  return *this;
}

std::istream * Crypt::wrap(std::istream * stream)
{
  return new icstream(key, key_len, stream, c_func, h_func);
}

std::ostream * Crypt::wrap(std::ostream * stream)
{
  return new ocstream(key, key_len, stream, c_func, h_func);
}

std::string Crypt::encrypt(const std::string & ptext)
{
  EVP_CIPHER_CTX cipher;
  std::string ctext;
  int iv_len = EVP_CIPHER_iv_length(c_func), ctemp_len, written;
  unsigned char iv[iv_len], *ctemp;

  // Generate the IV
  rand(iv, iv_len);

  // Setup the ciphertext string
  ctext.resize(enc_len(ptext.length()));
  ctemp = (unsigned char *)ctext.data();
  ctemp_len = ctext.length();

  // Add the IV to the string
  memcpy(ctemp, iv, iv_len);
  ctemp += iv_len;
  ctemp_len -= iv_len;

  // Encrypt the message to the ctext
  EVP_CIPHER_CTX_init(&cipher);
  EVP_EncryptInit_ex(&cipher, c_func, NULL, key, iv);

  written = ctemp_len;
  EVP_EncryptUpdate(&cipher, ctemp, &written,
                    (unsigned char *)ptext.data(), ptext.length());
  ctemp += written;
  ctemp_len -= written;

  written = ctemp_len;
  EVP_EncryptFinal_ex(&cipher, ctemp, &written);
  ctemp_len -= written;

  EVP_CIPHER_CTX_cleanup(&cipher);

  return ctext;
}

std::string Crypt::decrypt(const std::string & ctext)
{
  EVP_CIPHER_CTX cipher;
  int iv_len = EVP_CIPHER_iv_length(c_func), ctemp_len, ptemp_len, written;
  unsigned char iv[iv_len], *ctemp, *ptemp;

  // Setup the ptext string
  ptemp = (unsigned char *) malloc(ctext.length());
  ptemp_len = 0;
  ctemp = (unsigned char *)ctext.data();
  ctemp_len = ctext.length();

  // Copy the IV
  if (ctemp_len < iv_len)
    {
      free(ptemp);
      throw "Cipher text is too short";
    }
  memcpy(iv, ctemp, iv_len);
  ctemp += iv_len;
  ctemp_len -= iv_len;

  // Decrypt the message
  EVP_CIPHER_CTX_init(&cipher);
  EVP_DecryptInit_ex(&cipher, c_func, NULL, key, iv);

  written = ptemp_len;
  if (EVP_DecryptUpdate(&cipher, ptemp + ptemp_len,
                        &written, ctemp, ctemp_len) != 1)
    {
      EVP_CIPHER_CTX_cleanup(&cipher);
      free(ptemp);
      throw "Cipher text is invalid";
    }
  ptemp_len += written;

  written = ptemp_len;
  if (EVP_DecryptFinal_ex(&cipher, ptemp + ptemp_len, &written) != 1)
    {
      EVP_CIPHER_CTX_cleanup(&cipher);
      free(ptemp);
      throw "Cipher text is invalid";
    }
  ptemp_len += written;

  EVP_CIPHER_CTX_cleanup(&cipher);

  // Get the plaintext string
  std::string ptext((char*)ptemp, ptemp_len);
  free(ptemp);

  return ptext;
}

std::string Crypt::hash(const std::string & msg)
{
  std::string out;
  EVP_MD_CTX md;

  // Setup the string
  out.resize(EVP_MD_size(h_func));

  // Hash the message
  EVP_MD_CTX_init(&md);
  EVP_DigestInit_ex(&md, h_func, NULL);
  EVP_DigestUpdate(&md, (unsigned char *)msg.data(), msg.length());
  EVP_DigestFinal_ex(&md, (unsigned char *)out.data(), NULL);
  EVP_MD_CTX_cleanup(&md);

  return out;
}

std::string Crypt::sign(const std::string & msg)
{
  std::string out;
  HMAC_CTX hmac;

  // Setup the string
  out.resize(EVP_MD_size(h_func));

  // Hash the message
  HMAC_CTX_init(&hmac);
  HMAC_Init_ex(&hmac, key, key_len, h_func, NULL);
  HMAC_Update(&hmac, (unsigned char *)msg.data(), msg.length());
  HMAC_Final(&hmac, (unsigned char *)out.data(), NULL);
  HMAC_CTX_cleanup(&hmac);

  return out;
}

size_t Crypt::enc_len(size_t len)
{
  size_t bs = EVP_CIPHER_block_size(c_func), rem = len % bs;
  return EVP_CIPHER_iv_length(c_func) + len - rem + bs;
}

size_t Crypt::hash_len()
{
  return EVP_MD_size(h_func);
}

Crypt::ocstream::ocstream(const unsigned char *key, size_t key_len,
                          std::ostream * ostream,
                          const EVP_CIPHER *c_func, const EVP_MD *h_func)
  : ostream(ostream)
{
  EVP_CIPHER_CTX_init(&cipher);
}

Crypt::ocstream::~ocstream()
{
  EVP_CIPHER_CTX_cleanup(&cipher);
}

std::ostream & Crypt::ocstream::write(const char *s, std::streamsize n)
{
}

Crypt::icstream::icstream(const unsigned char *key, size_t key_len,
                          std::istream * istream,
                          const EVP_CIPHER *c_func, const EVP_MD *h_func)
  : istream(istream), first(true), last(false)
{
  // Setup the key and credentials
  this->iv_len = EVP_CIPHER_iv_length(c_func);
  this->key_len = key_len;

  this->key = (unsigned char *)malloc(key_len);
  mlock(this->key, key_len);
  this->iv = (unsigned char *)malloc(iv_len);

  memcpy(this->key, key, key_len);
  rand(this->iv, iv_len);

  // Setup the crypto
  EVP_CIPHER_CTX_init(&cipher);
  EVP_EncryptInit_ex(&cipher, c_func, NULL, key, iv);
  HMAC_CTX_init(&hmac);
  HMAC_Init_ex(&hmac, key, key_len, h_func, NULL);
}

Crypt::icstream::~icstream()
{
  EVP_CIPHER_CTX_cleanup(&cipher);
  HMAC_CTX_cleanup(&hmac);

  munlock(key, key_len);
  free(key);
  free(iv);
}

std::streamsize Crypt::icstream::readsome(char *s, std::streamsize n)
{
  std::streamsize len;
  int elen;
  unsigned int uelen;

  // If this is the first just send the iv
  if (first)
    {
      memcpy(s, iv, iv_len);
      first = false;
      return iv_len;
    }

  // Read from the input stream
  unsigned char *in = new unsigned char[n];
  len = istream->readsome((char*)in, n);

  // Encrypt the data
  elen = n;
  if (len == 0)
    {
      delete in;

      if (last)
        return 0;

      last = true;
      EVP_EncryptFinal_ex(&cipher, (unsigned char *)s, &elen);
      uelen = n - elen;
      HMAC_Final(&hmac, (unsigned char *)s+elen, &uelen);
      return elen + uelen;
    }
  else
    {
      EVP_EncryptUpdate(&cipher, (unsigned char *)s, &elen, in, len);
      HMAC_Update(&hmac, in, len);

      delete in;
      return elen;
    }
}

void Crypt::copy(const Crypt & crypt)
{
  c_func = crypt.c_func;
  h_func = crypt.h_func;

  key_len = crypt.key_len;
  key = (unsigned char *) malloc(key_len);
  mlock(key, key_len);
  memcpy(key, crypt.key, key_len);
}

volatile void Crypt::clear()
{
  volatile unsigned char *chars;
  size_t i;

  // Zero the memory of the key
  chars = (volatile unsigned char *)key;
  for (i = 0; i < key_len; i++)
    chars[i] = 0;

  munlock(key, key_len);
  free(key);
}

void Crypt::derive_key(const std::string & mat, const std::string & salt,
                       size_t iters, unsigned char *key, size_t key_len)
{
  size_t md_size = EVP_MD_size(h_func), written, i, salt_len, times = 0;
  unsigned char buff[md_size], *stemp;
  HMAC_CTX hmac;

  // Setup the modifiable salt structure
  salt_len = salt.length() + sizeof(uint64_t);
  stemp = new unsigned char[salt_len];
  memcpy(stemp, salt.data(), salt.length());

  HMAC_CTX_init(&hmac);
  while (key_len > 0)
    {
      // Create the key material for this hash segment
      *(uint64_t*)(stemp + salt.length()) = htobe64(times);

      // Perform iterations of the key hash
      HMAC_Init_ex(&hmac, stemp, salt_len, h_func, NULL);
      HMAC_Update(&hmac, (unsigned char *)mat.data(), mat.length());
      HMAC_Final(&hmac, buff, NULL);
      for (i = 1; i < iters; i++)
        {
          HMAC_Init_ex(&hmac, buff, md_size, h_func, NULL);
          HMAC_Update(&hmac, (unsigned char *)mat.data(), mat.length());
          HMAC_Final(&hmac, buff, NULL);
        }

      // Write the digest material to the keyspace
      written = key_len < md_size ? key_len : md_size;
      memcpy(key, buff, written);
      key_len -= written;
      key += written;

      times++;
    }
  HMAC_CTX_cleanup(&hmac);
  delete [] stemp;
}

void Crypt::rand(unsigned char *data, size_t size)
{
  RAND_bytes(data, size);
}
