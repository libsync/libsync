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

Thread::Thread(Runnable method) :
  func(method)
{}

Runnable *
Thread::start()
{
  if (pthread_create(&thread, NULL, runner, NULL) != 0)
    return NULL;
  return &func;
}

Runnable *
Thread::wait()
{
  if (pthread_join(&thread, NULL) != 0)
    return NULL;
  return &func;
}

void *
Thread::runner(void * args)
{
  func->run();
  return NULL;
}

Lock::Lock()
{
  pthread_mutex_init(&lock, NULL, NULL);
}

Lock::~Lock()
{
  pthread_mutex_destroy(&lock);
}

void
Lock::lock()
{
  pthread_mutex_lock(&lock);
}

bool
Lock::try_lock()
{
  return pthread_mutex_trylock(&lock) == 0;
}

void
Lock::unlock()
{
  pthread_mutex_unlock(&lock);
}

Condition::Condition()
{
  pthread_mutex_init(&lock, NULL);
  pthread_cond_init(&cond, NULL);
}

Condition::~Condition()
{
  pthread_cond_destroy(&cond);
  pthread_mutex_destroy(&lock);
}

Condition::lock()
{
  pthread_mutex_lock(&lock);
}

Condition::wait()
{
  pthread_cond_wait(&cond, &lock);
}

Condition::signal()
{
  pthread_cond_signal(&cond);
  pthread_mutex_unlock(&lock);
}
