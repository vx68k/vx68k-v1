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

namespace
{
  default_memory null_memory;
} // namespace (unnamed)

/* Read a block of data from memory.  */
void
address_space::read(int fc, uint32_type address,
		    void *data, size_t size) const
{
  unsigned char *i = static_cast<unsigned char *>(data);
  unsigned char *last = i + size;

  while (i != last)
    *i++ = getb(fc, address++);
}

uint_type
address_space::getw(int fc, uint32_type address) const
{
  if (address & 0x1)
    throw address_error_exception(true, fc, address);

  return getw_aligned(fc, address);
}

uint32_type
address_space::getl(int fc, uint32_type address) const
{
  if (address % 2 != 0)
    throw address_error_exception(true, fc, address);

  if (address / 2 % 2 != 0)
    {
      uint32_type value = (uint32_type(getw_aligned(fc, address)) << 16
			   | getw_aligned(fc, address + 2));
      return value;
    }
  else
    {
      address = canonical_address(address);
      const memory *p = find_page(address);
      uint32_type value = p->get_32(fc, address);
      return value;
    }
}

string
address_space::gets(int fc, uint32_type address) const
{
  address = canonical_address(address);
  string s;
  for (;;)
    {
      uint_type c = getb(fc, address++);
      if (c == 0)
	break;
      s += c;
    }

  return s;
}

/* Write a block of data to memory.  */
void
address_space::write(int fc, uint32_type address,
		     const void *data, size_t size)
{
  unsigned char *i = static_cast<unsigned char *>(data);
  unsigned char *last = i + size;

  while (i != last)
    putb(fc, address++, *i++);
}

void
address_space::putw(int fc, uint32_type address, uint_type value)
{
  if (address & 0x1)
    throw address_error_exception(false, fc, address);

  putw_aligned(fc, address, value);
}

void
address_space::putl(int fc, uint32_type address, uint32_type value)
{
  if (address % 2 != 0)
    throw address_error_exception(false, fc, address);

  if (address / 2 % 2 != 0)
    {
      putw_aligned(fc, address, value >> 16);
      putw_aligned(fc, address + 2, value);
    }
  else
    {
      address = canonical_address(address);
      memory *p = find_page(address);
      p->put_32(fc, address, value);
    }
}

void
address_space::puts(int fc, uint32_type address, const string &s)
{
  for (string::const_iterator i = s.begin();
       i != s.end();
       ++i)
    putb(fc, address++, *i);

  putb(fc, address++, 0);
}

void
address_space::set_pages(size_t first, size_t last, memory *p)
{
  I(first <= last);
  I(last <= NPAGES);
  fill(page_table + first, page_table + last, p);
}

address_space::address_space()
{
  fill(page_table + 0, page_table + NPAGES, &null_memory);
}
