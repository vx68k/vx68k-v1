/* vx68k - Virtual X68000
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

using vx68k::palettes_memory;
using vm68k::bus_error_exception;
using vm68k::SUPER_DATA;
using namespace vm68k::types;
using namespace std;

uint_type
palettes_memory::get_16(int fc, uint32_type address) const
{
#ifdef HAVE_NANA_H
  L("class palettes_memory: get_16 fc=%d address=%#010lx\n",
    fc, (unsigned long) address);
#endif
  if (fc != SUPER_DATA)
    throw bus_error_exception(true, fc, address);

  address &= 0x1fff;
  if (address < 256 * 2)
    return 0;
  else if (address < 512 * 2)
    {
      uint_type i = (address - 256 * 2) / 2;
      uint_type value = _tpalette[i];
      return value;
    }
  else
    {
      switch (address / 2)
	{
	case 0x400 / 2:
	  return 0;

	case 0x500 / 2:
	  return 0;

	case 0x600 / 2:
	  return 0;

	default:
	  return 0;
	}
    }
}

uint_type
palettes_memory::get_8(int fc, uint32_type address) const
{
#ifdef HAVE_NANA_H
  L("class palettes_memory: get_8 fc=%d address=%#010lx\n",
    fc, (unsigned long) address);
#endif
  uint_type w = get_16(fc, address);
  if (address % 2 != 0)
    return w >> 8 & 0xff;
  else
    return w & 0xff;
}

void
palettes_memory::put_16(int fc, uint32_type address, uint_type value)
{
#ifdef HAVE_NANA_H
  L("class palettes_memory: put_16 fc=%d address=%#010lx\n",
    fc, (unsigned long) address);
#endif
  if (fc != SUPER_DATA)
    throw bus_error_exception(false, fc, address);

  address &= 0x1fff;
  if (address < 256 * 2)
    ;
  else if (address < 512 * 2)
    {
      uint_type i = (address - 256 * 2) / 2;
      _tpalette[i] = value & 0xffffu;
    }
  else
    {
      switch (address / 2)
	{
	case 0x400 / 2:
	  break;

	case 0x500 / 2:
	  break;

	case 0x600 / 2:
	  break;

	default:
	  break;
	}
    }
}

void
palettes_memory::put_8(int, uint32_type, uint_type)
{
#ifdef HAVE_NANA_H
  L("class palettes_memory: FIXME: `put_8' not implemented\n");
#endif
}

palettes_memory::palettes_memory()
  : _tpalette(256, 0)
{
}
