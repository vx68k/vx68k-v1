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

#include <vx68k/memory.h>

#include <cstdlib>

using namespace vx68k;
using namespace std;

size_t
main_memory_page::read(int fc, uint32 address, void *data, size_t size) const
{
  if (address >= end)
    throw bus_error(fc, address);
  if (address + size >= end)
    size = end - address;

  uint8 *p = static_cast<uint8 *>(data);
  uint8 *last = p + size;
  if (p != last && (address & 1) != 0)
    *p++ = getb(fc, address++);
  while (last - p >> 1 != 0)
    {
      vm68k::putw(p, array[address >> 1]);
      address += 2;
      p += 2;
    }
  if (p != last)
    *p++ = getb(fc, address++);

  return size;
}

uint8
main_memory_page::getb (int fc, uint32 address) const
{
  uint16 wvalue = getw(fc, address & ~0x1);
  return address & 0x1 != 0 ? wvalue : wvalue >> 8;
}

uint16
main_memory_page::getw (int fc, uint32 address) const
{
  // Address error?
  if (address >= end)
    throw bus_error(fc, address);
  return array[address >> 1];
}

size_t
main_memory_page::write(int fc, uint32 address, const void *data, size_t size)
{
  if (address >= end)
    throw bus_error(fc, address);
  if (address + size >= end)
    size = end - address;

  uint8 *p = static_cast<uint8 *>(data);
  uint8 *last = p + size;
  if (p != last && (address & 1) != 0)
    putb(fc, address++, *p++);
  while (last - p >> 1 != 0)
    {
      array[address >> 1] = vm68k::getw(p);
      p += 2;
      address += 2;
    }
  if (p != last)
    putb(fc, address++, *p++);

  return size;
}

void
main_memory_page::putb(int fc, uint32 address, uint8 value)
{
  // FIXME.  Is it slow?
  uint16 wvalue = getw(fc, address & ~0x1);
  if (address & 0x1 != 0)
    {
      wvalue &= ~0x00ff;
      wvalue |= value;
    }
  else
    {
      wvalue &= ~0xff00;
      wvalue |= value << 8;
    }
  putw(fc, address & ~0x1, wvalue);
}

void
main_memory_page::putw (int fc, uint32 address, uint16 value)
{
  // Address error?
  if (address >= end)
    throw bus_error(fc, address);
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

x68k_address_space::x68k_address_space (size_t n)
  : main_memory (n)
{
  using vm68k::PAGE_SHIFT;
  set_pages (0, n >> PAGE_SHIFT, &main_memory);
#if 0
  set_pages (0xc00000 >> PAGE_SHIFT, 0xe00000 >> PAGE_SHIFT, &graphic_vram);
  set_pages (0xe00000 >> PAGE_SHIFT, 0xe80000 >> PAGE_SHIFT, &text_vram);
#endif
}
