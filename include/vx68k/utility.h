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

#ifndef _VX68K_UTILITY_H
#define _VX68K_UTILITY_H 1

#include <pthread.h>

namespace vx68k
{
  template <class Mutex> class auto_lock
  {
  protected:
    static void unlock(Mutex *mutex);

  private:
    Mutex *_mutex;

  public:
    explicit auto_lock(Mutex *mutex = NULL) throw ();

    auto_lock(auto_lock &x) throw ();

    ~auto_lock() throw ();

  public:
    auto_lock &operator=(auto_lock &x) throw ();

  public:
    Mutex *get() throw () {return _mutex;}

    Mutex *release() throw ();
  };

  template <class Mutex> inline void
  auto_lock<Mutex>::unlock(Mutex *mutex)
  {
    if (mutex != NULL)
      pthread_mutex_unlock(mutex);
  }

  template <class Mutex> inline Mutex *
  auto_lock<Mutex>::release() throw ()
  {
    Mutex *tmp = _mutex;
    _mutex = NULL;
    return tmp;
  }

  template <class Mutex> inline auto_lock<Mutex> &
  auto_lock<Mutex>::operator=(auto_lock &x) throw ()
  {
    unlock(_mutex);
    _mutex = x.release();
  }

  template <class Mutex> inline
  auto_lock<Mutex>::~auto_lock() throw ()
  {
    unlock(_mutex);
  }

  template <class Mutex> inline
  auto_lock<Mutex>::auto_lock(Mutex *mutex) throw ()
    : _mutex(mutex)
  {
    if (_mutex != NULL)
      pthread_mutex_lock(_mutex);
  }

  template <class Mutex> inline
  auto_lock<Mutex>::auto_lock(auto_lock &x) throw ()
    : _mutex(x.release())
  {}
} // namespace vx68k

#endif /* not _VX68K_UTILITY_H */
