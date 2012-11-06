/*
  Log file handler

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

#include <ctime>

#include "log.hxx"

Log log;

Log::Log() :
  level(1)
{}

Log::Log(int level) :
  level(level)
{}

Log::~Log()
{
  for (auto file = files.begin(); file != files.end(); file++)
    {
      (*file)->close();
      delete *file;
    }
}

void Log::add_output(std::ostream * out)
{
  mutex.lock();

  outs.push_back(out);

  mutex.unlock();
}

void Log::add_output(const std::string & filename)
{
  mutex.lock();

  std::ofstream * file = new std::ofstream(filename);

  files.push_back(file);
  outs.push_back(file);

  mutex.unlock();
}

void Log::set_level(int level)
{
  mutex.lock();

  this->level = level;

  mutex.unlock();
}

void Log::message(const std::string & message, int level)
{
  // Format the message string
  std::string output = message;

  mutex.lock();

  // Print the formatted message to all of the output streams
  if (level == this->level)
    for (auto out = outs.begin(); out != outs.end(); out++)
      **out << output;

  mutex.unlock();
}
