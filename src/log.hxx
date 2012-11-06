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

#ifndef __LOG_H__
#define __LOG_H__

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
   * The default log, has a message level of 1
   */
  Log();

  /**
   * Sets the default log level when the object is created
   * @param level The message display level
   */
  Log(int level);

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

private:
  int level;
  std::vector<std::ostream *> outs;
  std::vector<std::ofstream *> files;
  std::mutex mutex;
};

extern Log log;

#endif
