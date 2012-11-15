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

#ifndef __LOG_HXX__
#define __LOG_HXX__

#include <vector>
#include <ostream>
#include <fstream>
#include <mutex>

/**
 * Synchronized Logfile handler
 * Allows the program to print messages to the user console
 */
class Log
{
public:
  /**
   */
  static const int ERROR = 1, WARNING = 2, NOTICE = 3, DEBUG = 4;

  /**
   * The default log, has a message level of 1
   */
  Log();

  /**
   * Sets the default log level when the object is created
   * @param level The message display level
   */
  Log(int level);

  /**
   * Makes a copy of the other log
   * @param other The other log to copy
   */
  Log(const Log & other);

  /**
   * Copies the other log
   * @param other The other log to copy
   */
  Log & operator=(const Log & other);

  /**
   * Destroys the log and any handles to files
   */
  ~Log();

  /**
   * Adds the output stream as one of the print streams
   * @param out The output stream the log will print to
   */
  void add_output(std::ostream * out);

  /**
   * Adds the output file as one of the print streams
   * @param filename The name of the file to use as a log
   */
  void add_output(const std::string & filename);

  /**
   * Changes the level of the logging output
   * 0 means no logging, 1 is the default all the way up to 9
   * @param level The level of logging to enforce
   */
  void set_level(int level);

  /**
   * Writes a message to all of the output streams
   * @param message The message to write
   * @param level The importance level of the message
   */
  void message(const std::string & message, int level);

  /**
   * Writes a message to all of the output streams
   * @param message The message to write
   * @param level The importance level of the message
   */
  void message(const char * message, int level);

private:
  struct File
  {
    std::ofstream *file;
    size_t handles;
    std::mutex mutex;
  };

  static std::mutex global_mutex;

  int level;
  std::vector<std::ostream *> outs;
  std::vector<File *> files;
  std::mutex mutex;

  /**
   * Destroys any memory allocated by the current log
   */
  void clear();

  /**
   * Copies the streams and output level from another log
   * @param other The other log to copy from
   */
  void copy(const Log & other);
};

extern Log global_log;

#endif
