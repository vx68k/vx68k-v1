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

#include "vm68k/memory.h"

#include <algorithm>
#include <cassert>

using vm68k::memory;
using vm68k::bus_error_exception;
using namespace vm68k::types;
using namespace std;

void
memory::generate_bus_error(bool read, int fc, uint32_type address) const
{
  throw bus_error_exception(read, fc, address);
}

uint32_type
memory::get_32(int fc, uint32_type address) const
{
  return (uint32_type(get_16(fc, address + 0)) << 16
	  | uint32_type(get_16(fc, address + 2)));
}

void
memory::put_32(int fc, uint32_type address, uint32_type value)
{
  put_16(fc, address + 0, value >> 16);
  put_16(fc, address + 2, value);
}
