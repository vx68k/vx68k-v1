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

using namespace vx68k;
using namespace std;

size_t
main_memory::read(int fc, uint32_type address, void *data, size_t size) const
{
  if (address >= end)
    {
      generate_bus_error(fc, address);
      abort();
    }

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

uint_type
main_memory::getb(int fc, uint32_type address) const
{
  if (address >= end)
    {
      generate_bus_error(fc, address);
      abort();
    }

  uint_type w = array[address >> 1];
  return address & 0x1 != 0 ? w & 0xffu : w >> 8;
}

uint_type
main_memory::getw(int fc, uint32_type address) const
{
  // Address error?
  if (address >= end)
    {
      generate_bus_error(fc, address);
      abort();
    }

  return array[address >> 1];
}

uint32_type
main_memory::getl(int fc, uint32_type address) const
{
  // Address error?
  uint32_type address2 = address + 2;
  if (address2 >= end)
    {
      generate_bus_error(fc, address2);
      abort();
    }

  return uint32_type(array[address >> 1]) << 16 | array[address2 >> 1];
}

size_t
main_memory::write(int fc, uint32_type address, const void *data, size_t size)
{
  if (address >= end)
    {
      generate_bus_error(fc, address);
      abort();
    }

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
main_memory::putb(int fc, uint32_type address, uint_type value)
{
  if (address >= end)
    {
      generate_bus_error(fc, address);
      abort();
    }

  uint_type w = array[address >> 1];
  if (address & 0x1 != 0)
    w = w & ~0xffu | value & 0xffu;
  else
    w = value << 8 | w & 0xffu;
  array[address >> 1] = w & 0xffffu;
}

void
main_memory::putw(int fc, uint32_type address, uint_type value)
{
  // Address error?
  if (address >= end)
    {
      generate_bus_error(fc, address);
      abort();
    }

  array[address >> 1] = value;
}

main_memory::~main_memory()
{
  delete [] array;
}

main_memory::main_memory(size_t n)
  : end((n + 1u) & ~1u),
    array(NULL)
{
  array = new uint16 [end >> 1];
}

