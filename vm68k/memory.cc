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

uint_type
vm68k::getw(const void *address)
{
  const uint8 *p = static_cast<const uint8 *>(address);
  return p[0] << 8 | p[1];
}

uint32_type
vm68k::getl(const void *address)
{
  const uint8 *p = static_cast<const uint8 *>(address);
  return uint32_type(getw(p + 0)) << 16 | uint32_type(getw(p + 2));
}

void
vm68k::putw(void *address, uint_type value)
{
  uint8 *p = static_cast<uint8 *>(address);
  p[0] = value >> 8;
  p[1] = value;
}

void
vm68k::putl(void *address, uint32_type value)
{
  uint8 *p = static_cast<uint8 *>(address);
  putw(p + 0, value >> 16);
  putw(p + 2, value);
}

bus_error::bus_error (int s, uint32 a)
  : status (s),
    address (a)
{
}

void
memory::generate_bus_error(int fc, uint32_type address) const
{
  throw bus_error(fc, address);
}

uint32_type
memory::getl(int fc, uint32_type address) const
{
  return (uint32_type(getw(fc, address + 0)) << 16
	  | uint32_type(getw(fc, address + 2)));
}

void
memory::putl(int fc, uint32_type address, uint32_type value)
{
  putw(fc, address + 0, value >> 16);
  putw(fc, address + 2, value);
}

size_t
no_memory::read(int fc, uint32_type address, void *, size_t) const
{
  generate_bus_error(fc | bus_error::READ, address);
  abort();
}

uint_type
no_memory::getb(int fc, uint32_type address) const
{
  generate_bus_error(fc + bus_error::READ, address);
  abort();
}

uint_type
no_memory::getw(int fc, uint32_type address) const
{
  generate_bus_error(fc + bus_error::READ, address);
  abort();
}

size_t
no_memory::write(int fc, uint32_type address, const void *, size_t)
{
  generate_bus_error(fc | bus_error::WRITE, address);
  abort();
}

void
no_memory::putb(int fc, uint32_type address, uint_type)
{
  generate_bus_error(fc + bus_error::WRITE, address);
  abort();
}

void
no_memory::putw(int fc, uint32_type address, uint_type)
{
  generate_bus_error(fc + bus_error::WRITE, address);
  abort();
}

