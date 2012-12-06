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

#include <sstream>
#include <cstring>
#include <string>

#include "openssl/evp.h"
#include "openssl/hmac.h"

#include "util.hxx"

class CryptStream
{
public:
  /**
   * Creates a new crypto stream from the given key material and crypto funcs
   * @param dec True for a decryption stream and false for encryption
   * @param key The key data
   * @param key_len The key data length
   * @param c_func The openssl cipher function for crypting data
   * @param h_func The openssl hash function for hashing data
   */
  CryptStream(bool dec, unsigned char *key, size_t key_len,
              const EVP_CIPHER *c_func, const EVP_MD *h_func);
  ~CryptStream();

  /**
   * Reads data from the stream into a buffer
   * @param buff The buffer to read bytes into
   * @param size The size of the buffer
   * @return The amount of bytes read
   */
  ssize_t read(char * buff, size_t size);

  /**
   * Writes data from the buffer into the stream
   * @param buff The buffer to read bytes from
   * @param size The size of the buffer
   * @param The amount of bytes written to the stream
   */
  ssize_t write(const char * buff, size_t size);
private:
  bool dec, done;
  const EVP_CIPHER *c_func;
  const EVP_MD *h_func;
  std::stringstream stream;
  std::stringstream decbuff;
  EVP_CIPHER_CTX cipher;
  HMAC_CTX hmac;
  unsigned char *key, *iv;
  size_t key_len, iv_len;
};

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
   * Creates a stream for encrypting traffic
   * @return The encryption stream
   */
  CryptStream *ecstream();

  /**
   * Creates a stream for decrypting traffic
   * @return The decryption stream
   */
  CryptStream *dcstream();

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

  /**
   * Fills the data buffer with cryptographic secure random data
   * @param data The data buffer to write into
   * @param size The size of the buffer
   */
  static void rand(unsigned char *data, size_t size);
private:
  size_t key_len;
  unsigned char *key;
  const EVP_CIPHER *c_func;
  const EVP_MD *h_func;

  void copy(const Crypt & crypt);
  volatile void clear();
  void derive_key(const std::string & mat, const std::string & salt,
                  size_t iters, unsigned char *key, size_t key_len);
};

#endif
