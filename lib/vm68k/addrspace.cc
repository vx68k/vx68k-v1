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

using namespace vm68k;
using namespace std;

/* Read a block of data from memory.  */
void
address_space::read (int fc, uint32 address, void *buf, size_t n) const
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

unsigned int
address_space::getb(int fc, uint32 address) const
{
  uint32 p = address >> PAGE_SHIFT & NPAGES - 1;
  return page_table[p]->getb(fc, address);
}

int
address_space::getb_signed(int fc, uint32 address) const
{
  unsigned int value = getb(fc, address);
  return value >= 1u << 7 ? -(int) ((1u << 8) - value) : (int) value;
}

/* Get a word from memory.  */
unsigned int
address_space::getw (int fc, uint32 address) const
{
  uint32 p = address >> PAGE_SHIFT & NPAGES - 1;
  return page_table[p]->getw (fc, address);
}

int
address_space::getw_signed(int fc, uint32 address) const
{
  unsigned int value = getw(fc, address);
  return value >= 1u << 15 ? -(int) ((1u << 16) - value) : (int) value;
}

/* Returns the long word value.  */
uint32
address_space::getl(int fc, uint32 address) const
{
  return getw(fc, address + 0) << 16 | getw(fc, address + 2);
}

int32
address_space::getl_signed(int fc, uint32 address) const
{
  uint32 value = getl(fc, address);
  return value >= 1u << 31 ? -(int32) ((1u << 32) - value) : (int32) value;
}

/* Write a block of data to memory.  */
void
address_space::write (int fc, uint32 address, const void *buf, size_t n)
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

void
address_space::putb(int fc, uint32 address, unsigned int value)
{
  uint32 p = address >> PAGE_SHIFT & NPAGES - 1;
  page_table[p]->putb(fc, address, value);
}

/* Put a word to memory.  */
void
address_space::putw (int fc, uint32 address, unsigned int value)
{
  uint32 p = address >> PAGE_SHIFT & NPAGES - 1;
  page_table[p]->putw (fc, address, value);
}

/* Stores the long word value.  */
void
address_space::putl(int fc, uint32 address, uint32 value)
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

