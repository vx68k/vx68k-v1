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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#undef const
#undef inline

#include "vx68k/memory.h"

#include <cstdlib>

using namespace std;

namespace vx68k
{
#if 0
};
#endif

void
main_memory_page::read (int fc, uint32 address, void *, size_t) const
  throw (bus_error)
{
  abort ();			// FIXME
}

void
main_memory_page::write (int fc, uint32 address, const void *, size_t)
  throw (bus_error)
{
  abort ();			// FIXME
}

uint8
main_memory_page::getb (int fc, uint32 address) const
  throw (bus_error)
{
  abort ();			// FIXME
}

uint16
main_memory_page::getw (int fc, uint32 address) const
  throw (bus_error)
{
  // Address error?
  if (address >= end)
    abort ();			// FIXME
  return array[address >> 1];
}

void
main_memory_page::putb (int fc, uint32 address, uint8)
  throw (bus_error)
{
  abort ();			// FIXME
}

void
main_memory_page::putw (int fc, uint32 address, uint16 value)
  throw (bus_error)
{
  // Address error?
  if (address >= end)
    abort ();			// FIXME
  array[address >> 1] = value;
}

main_memory_page::~main_memory_page ()
{
  free (array);
}

main_memory_page::main_memory_page (size_t n)
  : end (n),
    array (NULL)
{
  array = static_cast <uint16 *> (malloc (n));
}

address_space::address_space (size_t n)
  : main_memory (n)
{
  using vm68k::PAGE_SHIFT;
  set_pages (0, 0xc00000 >> PAGE_SHIFT, &main_memory);
#if 0
  set_pages (0xc00000 >> PAGE_SHIFT, 0xe00000 >> PAGE_SHIFT, &graphic_vram);
  set_pages (0xe00000 >> PAGE_SHIFT, 0xe80000 >> PAGE_SHIFT, &text_vram);
#endif
}

};				// namespace vx68k
