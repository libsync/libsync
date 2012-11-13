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

#include "messages.hxx"

template <typename Type, typename Data>
void Messages<Type, Data>::push(Type type, Data data)
{
  lock.lock();
  messages.push_back(std::pair<Type, Data>(type, data));
  lock.unlock();
}

template <typename Type, typename Data>
Data Messages<Type, Data>::wait(Type type)
{
  lock.lock();
  while(true)
    {
      for (auto it = messages.begin(), end = messages.end; it != end; it++)
        if (it->first == type)
          {
            Data data = it->second;
            messages.erase(it);
            lock.unlock();
            return data;
          }
      cond.wait(lock);
    }
  lock.unlock();
}
