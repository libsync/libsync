/*
  Synchronized message buffer

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

#ifndef __MESSAGES_HXX__
#define __MESSAGES_HXX__

#include <list>
#include <utility>
#include <mutex>
#include <condition_variable>

template <typename Type, typename Data>
class Messages
{
public:
  void push(Type type, Data data);
  Data wait(Type type);
private:
  std::mutex lock;
  std::condition_variable_any cond;
  std::list< std::pair<Type,Data> > messages;
};

#endif
