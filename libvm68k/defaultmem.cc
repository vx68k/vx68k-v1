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

#include <cstdio>

#ifdef HAVE_NANA_H
# include <nana.h>
# undef assert
# define assert I
#else
# include <cassert>
#endif

using vm68k::default_memory;
using vm68k::bus_error_exception;
using namespace vm68k::types;
using namespace std;

int
default_memory::get_8(uint32_type address, function_code fc) const
{
  throw bus_error_exception(true, fc, address);
}

uint16_type
default_memory::get_16(uint32_type address, function_code fc) const
{
  assert((address & 1) == 0);
  throw bus_error_exception(true, fc, address);
}

void
default_memory::put_8(uint32_type address, int value, function_code fc)
{
  throw bus_error_exception(false, fc, address);
}

void
default_memory::put_16(uint32_type address, uint16_type value,
		       function_code fc)
{
  assert((address & 1) == 0);
  throw bus_error_exception(false, fc, address);
}
