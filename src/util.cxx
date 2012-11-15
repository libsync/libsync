/*
  Useful IO and Datastructure Utilities

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

#include "util.hxx"

uint8_t Read::i8(uint8_t * & data, size_t & size)
{
  if (size < 1)
    throw "Metadata object too small to deserialize";
  size--;
  return *(data++);
}

uint16_t Read::i16(uint8_t * & data, size_t & size)
{
  if (size < 2)
    throw "Metadata object too small to deserialize";
  size -= 2;
  uint16_t out = *((uint16_t*)data);
  data += 2;
  return be16toh(out);
}

uint32_t Read::i32(uint8_t * & data, size_t & size)
{
  if (size < 4)
    throw "Metadata object too small to deserialize";
  size -= 4;
  uint32_t out = *((uint32_t*)data);
  data += 4;
  return be32toh(out);
}

uint64_t Read::i64(uint8_t * & data, size_t & size)
{
  if (size < 8)
    throw "Metadata object too small to deserialize";
  size -= 8;
  uint64_t out = *((uint64_t*)data);
  data += 8;
  return be64toh(out);
}

void Write::i8(uint8_t i, uint8_t * data, size_t & offset)
{
  *((uint8_t*)(data+offset)) = i;
  offset += 1;
}

void Write::i8(uint8_t i, std::string & data)
{
  data.append((char*)&i, 1);
}

void Write::i16(uint16_t i, uint8_t * data, size_t & offset)
{
  *((uint16_t*)(data+offset)) = htobe16(i);
  offset += 2;
}

void Write::i16(uint16_t i, std::string & data)
{
  i = htobe16(i);
  data.append((char*)&i, 2);
}

void Write::i32(uint32_t i, uint8_t * data, size_t & offset)
{
  *((uint32_t*)(data+offset)) = htobe32(i);
  offset += 2;
}

void Write::i32(uint32_t i, std::string & data)
{
  i = htobe32(i);
  data.append((char*)&i, 4);
}

void Write::i64(uint64_t i, uint8_t * data, size_t & offset)
{
  *((uint64_t*)(data+offset)) = htobe64(i);
  offset += 8;
}

void Write::i64(uint64_t i, std::string & data)
{
  i = htobe64(i);
  data.append((char*)&i, 8);
}
