/* -*-C++-*- */
/* vx68k - Virtual X68000
   Copyright (C) 1998 Hypercore Software Design, Ltd.

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

#ifndef VM68K_MEMORY_H
#define VM68K_MEMORY_H 1

#include <iterator>
#include "vm68k/types.h"

namespace vm68k
{

enum function_code
{
  USER_DATA = 1,
  USER_PROGRAM = 2,
  SUPER_DATA = 5,
  SUPER_PROGRAM = 6
};

class bus_error
{
};

const int PAGE_SIZE_SHIFT = 13;
const size_t PAGE_SIZE = 1 < PAGE_SIZE_SHIFT;

struct memory_page
{
  virtual ~memory_page () {}
  virtual void read (int, uint32, void *, size_t) const throw (bus_error) = 0;
  virtual void write (int, uint32, const void *, size_t) = 0;
  virtual uint16 read16 (int, uint32) const throw (bus_error) = 0;
  virtual uint32 read32 (int, uint32) const throw (bus_error);
  virtual uint8 read8 (int, uint32) const throw (bus_error) = 0;
  virtual void write16 (int, uint32, uint16) throw (bus_error) = 0;
  virtual void write32 (int, uint32, uint32) throw (bus_error);
  virtual void write8 (int, uint32, uint8) throw (bus_error) = 0;
};

class memory
{
public:
  struct iterator: bidirectional_iterator <uint16, int32>
  {
  };
  memory ();
  void set_memory_pages (int begin, int end, memory_page *);
  void read (int, uint32, void *, size_t) const;
  void write (int, uint32, const void *, size_t);
  uint16 read16 (int, uint32) const throw (bus_error);
  uint32 read32 (int, uint32) const throw (bus_error);
  uint8 read8 (int, uint32) const throw (bus_error);
  void write16 (int, uint32, uint16) throw (bus_error);
  void write32 (int, uint32, uint32) throw (bus_error);
  void write8 (int, uint32, uint8) throw (bus_error);
private:
  memory_page *page[1 << 24 - PAGE_SIZE_SHIFT];
};

};				// namespace vm68k

#endif

