/* vx68k - Virtual X68000
   Copyright (C) 1998, 1999 Hypercore Software Design, Ltd.

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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#undef const
#undef inline

#include "vm68k/memory.h"

#include <algorithm>
#include <cassert>

using namespace std;

namespace vm68k
{
#if 0
};
#endif

uint16
getw (const void *address)
{
  const uint8 *p = static_cast <const uint8 *> (address);
  return p[0] << 8 | p[1];
}

uint32
getl (const void *address)
{
  const uint8 *p = static_cast <const uint8 *> (address);
  return uint32 (getw (p + 0)) << 16 | uint32 (getw (p + 2));
}

void
putw (void *address, uint16 value)
{
  uint8 *p = static_cast <uint8 *> (address);
  p[0] = value >> 8;
  p[1] = value;
}

void
putl (void *address, uint32 value)
{
  uint8 *p = static_cast <uint8 *> (address);
  putw (p + 0, value >> 16);
  putw (p + 2, value);
}

bus_error::bus_error (int s, uint32 a)
  : status (s),
    address (a)
{
}

uint32
memory_page::getl (int fc, uint32 address) const
  throw (bus_error)
{
  return (uint32 (getw (fc, address + 0)) << 16
	  | uint32 (getw (fc, address + 2)));
}

void
memory_page::putl (int fc, uint32 address, uint32 value)
  throw (bus_error)
{
  putw (fc, address + 0, value >> 16);
  putw (fc, address + 2, value);
}

void
bus_error_page::read (int fc, uint32 address, void *, size_t) const
  throw (bus_error)
{
  throw bus_error (fc + bus_error::READ, address);
}

void
bus_error_page::write (int fc, uint32 address, const void *, size_t)
  throw (bus_error)
{
  throw bus_error (fc + bus_error::WRITE, address);
}

uint8
bus_error_page::getb (int fc, uint32 address) const
  throw (bus_error)
{
  throw bus_error (fc + bus_error::READ, address);
}

uint16
bus_error_page::getw (int fc, uint32 address) const
  throw (bus_error)
{
  throw bus_error (fc + bus_error::READ, address);
}

void
bus_error_page::putb (int fc, uint32 address, uint8)
  throw (bus_error)
{
  throw bus_error (fc + bus_error::WRITE, address);
}

void
bus_error_page::putw (int fc, uint32 address, uint16)
  throw (bus_error)
{
  throw bus_error (fc + bus_error::WRITE, address);
}

/* Read a block of data from memory.  */
void
address_space::read (int fc, uint32 address, void *buf, size_t n) const
  throw (bus_error)
{
  // This code is slow!
  uint8 *p = static_cast <uint8 *> (buf);
  assert ((n & 1) == 0);
  while (n != 0)
    {
      vm68k::putw (p, getw (fc, address));
      address += 2;
      p += 2;
      n -= 2;
    }
}

/* Write a block of data to memory.  */
void
address_space::write (int fc, uint32 address, const void *buf, size_t n)
  throw (bus_error)
{
  // This code is slow!
  const uint8 *p = static_cast <const uint8 *> (buf);
  assert ((n & 1) == 0);
  while (n != 0)
    {
      putw (fc, address, vm68k::getw (p));
      p += 2;
      address += 2;
      n -= 2;
    }
}

/* Get a word from memory.  */
uint16
address_space::getw (int fc, uint32 address) const
  throw (bus_error)
{
  uint32 p = address >> PAGE_SHIFT & NPAGES - 1;
  return page_table[p]->getw (fc, address);
}

/* Put a word to memory.  */
void
address_space::putw (int fc, uint32 address, uint16 value)
  throw (bus_error)
{
  uint32 p = address >> PAGE_SHIFT & NPAGES - 1;
  page_table[p]->putw (fc, address, value);
}

/* Returns the long word value.  */
uint32
address_space::getl(int fc, uint32 address) const
  throw (bus_error)
{
  return getw(fc, address + 0) << 16 | getw(fc, address + 2);
}

/* Stores the long word value.  */
void
address_space::putl(int fc, uint32 address, uint32 value)
  throw (bus_error)
{
  putw(fc, address + 0, value >> 16);
  putw(fc, address + 2, value);
}

void
address_space::set_pages (size_t first, size_t last, memory_page *p)
{
  assert (first <= last);
  assert (last <= NPAGES);
  fill (page_table + first, page_table + last, p);
}

address_space::address_space ()
{
  fill (page_table + 0, page_table + NPAGES, &default_page);
}

};				// namespace vm68k

