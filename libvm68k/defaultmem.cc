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

#ifdef HAVE_NANA_H
# include <nana.h>
# include <cstdio>
#else
# include <cassert>
# define I assert
#endif

using vm68k::default_memory;
using vm68k::bus_error_exception;
using namespace vm68k::types;
using namespace std;

uint16_type
default_memory::get_16(function_code fc, uint32_type address) const
{
  address &= 0xfffffffeU;
  throw bus_error_exception(true, fc, address);
}

int
default_memory::get_8(function_code fc, uint32_type address) const
{
  address &= 0xffffffffU;
  throw bus_error_exception(true, fc, address);
}

void
default_memory::put_16(function_code fc, uint32_type address, uint16_type)
{
  address &= 0xfffffffeU;
  throw bus_error_exception(false, fc, address);
}

void
default_memory::put_8(function_code fc, uint32_type address, int)
{
  address &= 0xffffffffU;
  throw bus_error_exception(false, fc, address);
}
