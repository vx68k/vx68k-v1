/* -*- C++ -*- */
/* Virtual X68000 - X68000 virtual machine
   Copyright (C) 1998-2000 Hypercore Software Design, Ltd.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef _VM68K_MUTEX_H
#define _VM68K_MUTEX_H 1

#include <pthread.h>

namespace vm68k
{
  /* Locking helper for pthread mutex.  */
  class mutex_lock
  {
  protected:
    static void unlock(pthread_mutex_t *mutex);

  private:
    pthread_mutex_t *_mutex;

  public:
    explicit mutex_lock(pthread_mutex_t *mutex = 0) throw ();

    mutex_lock(mutex_lock &x) throw ();

    ~mutex_lock() throw ();

  public:
    mutex_lock &operator=(mutex_lock &x) throw ();

  public:
    pthread_mutex_t *get() throw () {return _mutex;}

    pthread_mutex_t *release() throw ();
  };

  inline void
  mutex_lock::unlock(pthread_mutex_t *mutex)
  {
    if (mutex != 0)
      pthread_mutex_unlock(mutex);
  }

  inline pthread_mutex_t *
  mutex_lock::release() throw ()
  {
    pthread_mutex_t *tmp = _mutex;
    _mutex = 0;
    return tmp;
  }

  inline mutex_lock &
  mutex_lock::operator=(mutex_lock &x) throw ()
  {
    unlock(_mutex);
    _mutex = x.release();
    return *this;
  }

  inline
  mutex_lock::~mutex_lock() throw ()
  {
    unlock(_mutex);
  }

  inline
  mutex_lock::mutex_lock(pthread_mutex_t *mutex) throw ()
    : _mutex(mutex)
  {
    if (_mutex != 0)
      pthread_mutex_lock(_mutex);
  }

  inline
  mutex_lock::mutex_lock(mutex_lock &x) throw ()
    : _mutex(x.release())
  {}
} // namespace

#endif /* not _VM68K_MUTEX_H */
