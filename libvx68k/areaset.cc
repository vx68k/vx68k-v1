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

#include <vx68k/memory.h>

#ifdef HAVE_NANA_H
# include <nana.h>
# include <cstdio>
#else
# include <cassert>
# define I assert
#endif

using vx68k::area_set;
using vm68k::bus_error_exception;
using namespace vm68k::types;
using namespace std;

uint_type
area_set::get_16(function_code fc, uint32_type address) const
{
#ifdef HAVE_NANA_H
  L("class area_set: get_16 fc=%d address=%#010x\n", fc, address);
#endif
  return 0;
}

uint_type
area_set::get_8(function_code fc, uint32_type address) const
{
#ifdef HAVE_NANA_H
  L("class area_set: get_8 fc=%d address=%#010x\n", fc, address);
#endif
  return 0;
}

void
area_set::put_8(function_code fc, uint32_type address, unsigned int value)
{
  address &= 0xffffffffu;
  value &= 0xffu;
#ifdef HAVE_NANA_H
  L("class area_set: put_8: fc=%d address=0x%08lx value=0x%02x\n",
    fc, (unsigned long) address, value);
#endif

  I(fc != memory::SUPER_PROGRAM);
  if (fc != memory::SUPER_DATA)
    throw bus_error_exception(false, fc, address);

  uint_type i = address & 0x1fff;
  switch (i)
    {
    case 0x1:
      {
	uint32_type area = uint32_type(value + 1) * 0x2000;
	_mm->set_super_area(area);
	break;
      }

    default:
      throw bus_error_exception(false, fc, address);
    }
}

void
area_set::put_16(function_code fc, uint32_type address, uint_type value)
{
  this->put_8(fc, address / 2 * 2 + 1, value);
}

area_set::area_set(main_memory *mm)
  : _mm(mm)
{
}
