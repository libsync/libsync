/*
  The entry point into the file synchronization client

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

#include <cstdlib>
#include <thread>
#include <iostream>

#include "../src/log.hxx"

int main(int argc, char ** argv)
{
  log.add_output(&std::cout);
  log.add_output("out.log");
  log.message("hi", 1);
  return EXIT_SUCCESS;
}
