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

#include <sstream>
#include <ctime>

#include "log.hxx"

#define DATE_SIZE 20

std::mutex Log::global_mutex;
Log global_log;

Log::Log() :
  level(1)
{}

Log::Log(int level) :
  level(level)
{}

Log::Log(const Log & other)
{
  copy(other);
}

Log & Log::operator=(const Log & other)
{
  if (this != &other)
    {
      mutex.lock();
      clear();
      copy(other);
      mutex.unlock();
    }
  return *this;
}

Log::~Log()
{
  mutex.lock();

  clear();

  mutex.unlock();
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

  // Try to create a logfile for writing
  std::ofstream * file = new std::ofstream(filename,
                                           std::ios::out | std::ios::app);
  if (file->fail())
    {
      file->close();
      delete file;
      throw "Failed to open the log for writing.";
    }

  // Create a new File for proper file garbage collection
  File * outfile = new File;
  outfile->file = file;
  outfile->handles = 1;
  files.push_back(outfile);

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
  time_t rawtime;
  char date[DATE_SIZE];
  time(&rawtime);
  strftime(date, DATE_SIZE, "%x %X", localtime(&rawtime));

  std::stringstream build;
  build << "[Level " << level << "][" << date;
  build << "]: " << message << "\n";
  std::string output = build.str();

  mutex.lock();

  if (level <= this->level)
    {
      // Print the formatted message to all of the output streams
      global_mutex.lock();
      for (auto out = outs.begin(); out != outs.end(); out++)
        {
          **out << output;
          (*out)->flush();
        }
      global_mutex.unlock();

      // Print the formatted message to all of the files
      for (auto out = files.begin(); out!= files.end(); out++)
        {
          (*out)->mutex.lock();
          *(*out)->file << output;
          (*out)->file->flush();
          (*out)->mutex.unlock();
        }
    }

  mutex.unlock();
}

void Log::message(const char * message, int level)
{
  this->message(std::string(message), level);
}

void Log::clear()
{
  for (auto file = files.begin(); file != files.end(); file++)
    {
      // Delete only files which have no other log handles
      (*file)->mutex.lock();
      if ((*file)->handles > 1)
        {
          (*file)->handles--;
          (*file)->mutex.unlock();
          continue;
        }
      (*file)->file->close();
      delete (*file)->file;
      delete *file;

    }

  files.clear();
  outs.clear();
}

void Log::copy(const Log & other)
{
  for (auto file = other.files.begin(); file != other.files.end(); file++)
    {
      // Make sure the other logs know we have a handle
      (*file)->mutex.lock();
      (*file)->handles++;
      (*file)->mutex.unlock();

      files.push_back(*file);
    }
  for (auto out = other.outs.begin(); out == other.outs.end(); out++)
    outs.push_back(*out);
}
