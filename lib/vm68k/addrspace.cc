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

#include <vm68k/memory.h>

#include <algorithm>

#ifdef HAVE_NANA_H
# include <nana.h>
# include <cstdio>
#else
# include <cassert>
# define I assert
#endif

using namespace vm68k;
using namespace std;

/* Read a block of data from memory.  */
void
address_space::read(int fc, uint32 address, void *data, size_t size) const
{
  while (size != 0)
    {
      address &= ((uint32) 1 << ADDRESS_BIT) - 1;
      uint32 p = address >> PAGE_SHIFT;
      size_t done = page_table[p]->read(fc, address, data, size);
      I(done != 0);
      I(done <= size);
      address += done;
      data = static_cast<uint8 *>(data) + done;
      size -= done;
    }
}

uint_type
address_space::getw(int fc, uint32_type address) const
{
  // FIXME: Unaligned address must be handled.
  return getw_aligned(fc, address);
}

/* Returns the long word value.  */
uint32
address_space::getl(int fc, uint32 address) const
{
  // FIXME: Unaligned address must be handled.
  address = canonical_address(address);
  uint32_type address2 = canonical_address(address + 2);
  memory_page *p = page_table[address >> PAGE_SHIFT];
  memory_page *p2 = page_table[address2 >> PAGE_SHIFT];
  if (p2 != p)
    return p->getw(fc, address) << 16 | p2->getw(fc, address2);
  else
    return p->getl(fc, address);
}

/* Write a block of data to memory.  */
void
address_space::write(int fc, uint32 address, const void *data, size_t size)
{
  while (size != 0)
    {
      address &= ((uint32) 1 << ADDRESS_BIT) - 1;
      uint32 p = address >> PAGE_SHIFT;
      size_t done = page_table[p]->write(fc, address, data, size);
      I(done != 0);
      I(done <= size);
      address += done;
      data = static_cast<uint8 *>(data) + done;
      size -= done;
    }
}

void
address_space::putb(int fc, uint32 address, unsigned int value)
{
  address &= ((uint32) 1 << ADDRESS_BIT) - 1;
  uint32 p = address >> PAGE_SHIFT;
  page_table[p]->putb(fc, address, value);
}

/* Put a word to memory.  */
void
address_space::putw (int fc, uint32 address, unsigned int value)
{
  address &= ((uint32) 1 << ADDRESS_BIT) - 1;
  uint32 p = address >> PAGE_SHIFT;
  page_table[p]->putw (fc, address, value);
}

/* Stores the long word value.  */
void
address_space::putl(int fc, uint32 address, uint32 value)
{
  // FIXME: memroy_page::putl should be used.
  putw(fc, address + 0, value >> 16);
  putw(fc, address + 2, value);
}

void
address_space::set_pages (size_t first, size_t last, memory_page *p)
{
  I(first <= last);
  I(last <= NPAGES);
  fill (page_table + first, page_table + last, p);
}

address_space::address_space ()
{
  fill (page_table + 0, page_table + NPAGES, &default_page);
}

