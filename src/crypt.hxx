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

#include <openssl/evp.h>
#include <openssl/hmac.h>

class Crypt
{
public:
  /**
   * Creates a new crypto object with the given key material
   * @param key The keying material
   */
  Crypt(const std::string & key);
  Crypt(const Crypt & crypt);
  ~Crypt();
  Crypt & operator=(const Crypt & crypt);

  /**
   * Wraps the stream in a crypto stream for encrypting traffic
   * @param stream The input stream to encrypt
   * @return The encryption stream
   */
  std::istream * wrap(std::istream * stream);

  /**
   * Wraps the stream in a crypto stream for decrypting traffic
   * @param stream The output stream to decrypt into
   * @return The decrypting stream
   */
  std::ostream * wrap(std::ostream * stream);

  /**
   * Converts the plaintext into an encrypted string using the internal key
   * @param ptext The plain text to encrypt
   * @return The cipher text of the message
   */
  std::string encrypt(const std::string & ptext);

  /**
   * Converts the ciphertext into a decrypted plaintext string
   * @param ctext The ciphertext to decrypt
   * @return The plaintext message
   */
  std::string decrypt(const std::string & ctext);

  /**
   * A fixed length one way function which computes a unique value for a message
   * @param msg The message to hash
   * @return The hashed bytes of the message
   */
  std::string hash(const std::string & msg);

  /**
   * A fixed length signing algorithm based on the hash and internal key
   * @param msg The message to sign
   * @return The signature of the message
   */
  std::string sign(const std::string & msg);

  /**
   * Gets the length of an encrypted block for the plaintext
   * @param len The length of the plaintext
   * @return The length of the corresponding ciphertext
   */
  size_t enc_len(size_t len);

  /**
   * @return The length of a hash string
   */
  size_t hash_len();
private:
  class ocstream : public std::ostream
  {
  public:
    ocstream(const unsigned char *key, size_t key_len,
             std::ostream * ostream,
             const EVP_CIPHER *c_func, const EVP_MD *h_func);
    ~ocstream();
    std::ostream & write(const char *s, std::streamsize n);
  private:
    std::ostream *ostream;
    unsigned char *key, *iv;
    size_t key_len, iv_len;
    EVP_CIPHER_CTX cipher;
    HMAC_CTX hmac;
  };

  class icstream : public std::istream
  {
  public:
    icstream(const unsigned char *key, size_t key_len,
             std::istream * istream,
             const EVP_CIPHER *c_func, const EVP_MD *h_func);
    ~icstream();
    std::streamsize readsome(char *s, std::streamsize n);
  private:
    std::istream *istream;
    unsigned char *key, *iv;
    size_t key_len, iv_len;
    EVP_CIPHER_CTX cipher;
    HMAC_CTX hmac;
    bool first, last;
  };

  class cstreambuf : public std::streambuf
  {
  public:
    cstreambuf();
    ~cstreambuf();
  };

  size_t key_len;
  unsigned char *key;
  const EVP_CIPHER *c_func;
  const EVP_MD *h_func;

  void copy(const Crypt & crypt);
  volatile void clear();
  void derive_key(const std::string & mat, const std::string & salt,
                  size_t iters, unsigned char *key, size_t key_len);
  static void rand(unsigned char *data, size_t size);
};

#endif
