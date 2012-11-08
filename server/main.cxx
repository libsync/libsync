/*
  The entry point into the file storage server

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
#include <string>
#include <iostream>
#include "../src/net.hxx"
#include "../src/log.hxx"

int main(int argc, char * argv[])
{
  global_log.add_output(&std::cerr);
  global_log.set_level(3);
  try
    {
      NetServer ns("localhost", 17654);
      Net * net = ns.accept();
      net->write("Hello World\n");
      uint8_t s[16];
      net->read(s, 16);
      delete net;
      std::cout << (char*)s << std::endl;
    }
  catch (const std::string & e)
    {
      std::cerr << e << std::endl;
      return EXIT_FAILURE;
    }
  catch (const char * e)
    {
      std::cerr << e << std::endl;
      return EXIT_FAILURE;
    }
  return EXIT_SUCCESS;
}
