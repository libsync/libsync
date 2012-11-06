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

#include <pthread.h>

#include "thread.hxx"

template < class Runnable >
Thread<Runnable>::Thread(Runnable * method) :
  func(method)
{}

template < class Runnable >
Runnable * Thread<Runnable>::start()
{
  if (pthread_create(&thread, NULL, runner, NULL) != 0)
    return NULL;
  return &func;
}

template < class Runnable >
Runnable * Thread<Runnable>::wait()
{
  if (pthread_join(&thread, NULL) != 0)
    return NULL;
  return &func;
}

template < class Runnable >
void * Thread<Runnable>::runner(void * args)
{
  func->run();
  return NULL;
}

Lock::Lock()
{
  pthread_mutex_init(&lck, NULL);
}

Lock::~Lock()
{
  pthread_mutex_destroy(&lck);
}

void Lock::lock()
{
  pthread_mutex_lock(&lck);
}

bool Lock::try_lock()
{
  return pthread_mutex_trylock(&lck) == 0;
}

void Lock::unlock()
{
  pthread_mutex_unlock(&lck);
}

Condition::Condition()
{
  pthread_mutex_init(&lck, NULL);
  pthread_cond_init(&cond, NULL);
}

Condition::~Condition()
{
  pthread_cond_destroy(&cond);
  pthread_mutex_destroy(&lck);
}

void Condition::lock()
{
  pthread_mutex_lock(&lck);
}

void Condition::wait()
{
  pthread_cond_wait(&cond, &lck);
}

void Condition::signal()
{
  pthread_cond_signal(&cond);
  pthread_mutex_unlock(&lck);
}
