/*
  A wrapper thread library for portability between OS's

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

#ifndef __THREAD_HXX__
#define __THREAD_HXX__

#include "runnable.hxx"

template < class Runnable >
class Thread
{
public:
  Thread(Runnable * method);

  Runnable * join();
private:
  pthread_t thread;
  Runnable func;
  void * runner(void * args);
};

class Lock
{
public:
  Lock();
  ~Lock();
  void lock();
  bool try_lock();
  void unlock();
private:
  pthread_mutex_t lock;
};

class Condition
{
public:
  Condition();
  ~Condition();
  void lock();
  void wait();
  void signal();
private:
  pthread_mutex_t lock;
  pthread_cond_t cond;
};

#endif