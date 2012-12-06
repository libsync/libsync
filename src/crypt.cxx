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

#include "openssl/evp.h"
#include "openssl/rand.h"
#include "openssl/hmac.h"
#include <sys/mman.h>

#include "util.hxx"
#include "crypt.hxx"

CryptStream::CryptStream(bool dec, unsigned char *key, size_t key_len,
                         const EVP_CIPHER *c_func, const EVP_MD *h_func)
  : dec(dec), done(false), c_func(c_func), h_func(h_func),
    iv(NULL), iv_len(EVP_CIPHER_iv_length(c_func))
{
  // Setup the key and credentials
  this->key_len = key_len;
  this->key = new unsigned char[key_len];
  mlock(this->key, key_len);
  memcpy(this->key, key, key_len);

  EVP_CIPHER_CTX_init(&cipher);
  HMAC_CTX_init(&hmac);
}

CryptStream::~CryptStream()
{
  delete [] key;
  delete [] iv;
  HMAC_CTX_cleanup(&hmac);
  EVP_CIPHER_CTX_cleanup(&cipher);
}

ssize_t CryptStream::read(char * buff, size_t size)
{
  return stream.readsome(buff, size);
}

#include <iostream>
ssize_t CryptStream::write(const char * buff, size_t size)
{
  // If nothing is written finalize decrypt
  if (dec && size == 0)
    {
      if (iv == NULL)
        throw "Never got an IV";

      // Make sure we have an HMAC sum in the stream
      ssize_t md_size = EVP_MD_size(h_func), tmp_size = 0;
      if (decbuff.tellp() - decbuff.tellg() != md_size)
        throw "Decrypt stream missing hmac sum";

      // Finalize the cipher and write the rest of the ciphertext
      unsigned char data[md_size], data2[md_size];
      int dec_len = md_size;
      if (EVP_DecryptFinal_ex(&cipher, data, &dec_len) != 1)
        throw "Failed to decrypt final block";
      stream.write((char*)data, dec_len);
      HMAC_Update(&hmac, data, dec_len);

      // Check the HMAC sum against the provided one
      HMAC_Final(&hmac, data, NULL);
      while (tmp_size < md_size)
        tmp_size += decbuff.readsome((char*)data2+tmp_size, md_size-tmp_size);
      if (memcmp(data, data2, md_size) != 0)
        throw "HMAC sums do not match";
    }
  else if (dec)
    {
      decbuff.write(buff, size);

      // First get the iv from the stream
      ssize_t left = decbuff.tellp() - decbuff.tellg(), in_len;
      if (iv == NULL)
        {
          if (left >= (ssize_t)iv_len)
            {
              iv = new unsigned char[iv_len];
              in_len = 0;
              while (in_len < (ssize_t)iv_len)
                in_len += decbuff.readsome((char*)iv+in_len, iv_len-in_len);
              left -= iv_len;
              EVP_DecryptInit_ex(&cipher, c_func, NULL, key, iv);
              HMAC_Init_ex(&hmac, key, key_len, h_func, NULL);
            }
          else
            return size;
        }

      // Make sure we have enough ciphertext and leave hmac intact
      left -= EVP_MD_size(h_func);
      if (left <= 0)
        return size;

      // Process the encrypted ciphertext in the buffer
      int out_len = left + EVP_CIPHER_block_size(c_func);
      unsigned char *out = new unsigned char[out_len],
        *in = new unsigned char[left];
      in_len = 0;
      while (in_len < left)
        in_len += decbuff.readsome((char*)in + in_len, left-in_len);
      if (EVP_DecryptUpdate(&cipher, out, &out_len, in, left) != 1)
        {
          delete [] out;
          delete [] in;
          throw "Failed to decrypt more data.";
        }

      // Write the HMAC and stream
      HMAC_Update(&hmac, out, out_len);
      stream.write((char*)out, out_len);
      delete [] in;
      delete [] out;
   }
  else
    {
      // First encrypt should write the iv to the stream
      if (iv == NULL)
        {
          iv = new unsigned char[iv_len];
          Crypt::rand(iv, iv_len);
          stream.write((char*)iv, iv_len);
          EVP_EncryptInit_ex(&cipher, c_func, NULL, key, iv);
          HMAC_Init_ex(&hmac, key, key_len, h_func, NULL);
        }

      if (size == 0)
        {
          if (done)
            return size;

          done = true;

          // Finalize the encryption stream
          int out_len = EVP_MD_size(h_func);
          unsigned char out[out_len];
          EVP_EncryptFinal_ex(&cipher, out, &out_len);
          stream.write((char*)out, out_len);

          // Write the HMAC
          HMAC_Final(&hmac, out, NULL);
          stream.write((char*)out, EVP_MD_size(h_func));
        }
      else
        {
          // Encrypt the data given
          int out_len = size + EVP_CIPHER_block_size(c_func);
          unsigned char *out = new unsigned char[out_len];
          EVP_EncryptUpdate(&cipher, out, &out_len,
                            (unsigned char *)buff, size);
          HMAC_Update(&hmac, (unsigned char *)buff, size);
          stream.write((char*)out, out_len);
          delete [] out;
        }
    }

  return size;
}

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

CryptStream * Crypt::ecstream()
{
  return new CryptStream(false, key, key_len, c_func, h_func);
}

CryptStream * Crypt::dcstream()
{
  return new CryptStream(true, key, key_len, c_func, h_func);
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
  size_t md_size = EVP_MD_size(h_func);
  unsigned char out[md_size];
  EVP_MD_CTX md;

  // Hash the message
  EVP_MD_CTX_init(&md);
  EVP_DigestInit_ex(&md, h_func, NULL);
  EVP_DigestUpdate(&md, (unsigned char *)msg.data(), msg.length());
  EVP_DigestFinal_ex(&md, out, NULL);
  EVP_MD_CTX_cleanup(&md);

  return std::string((char*)out, md_size);
}

std::string Crypt::sign(const std::string & msg)
{
  size_t md_size = EVP_MD_size(h_func);
  unsigned char out[md_size];
  HMAC_CTX hmac;

  // Hash the message
  HMAC_CTX_init(&hmac);
  HMAC_Init_ex(&hmac, key, key_len, h_func, NULL);
  HMAC_Update(&hmac, (unsigned char *)msg.data(), msg.length());
  HMAC_Final(&hmac, out, NULL);
  HMAC_CTX_cleanup(&hmac);

  return std::string((char*)out, md_size);
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
