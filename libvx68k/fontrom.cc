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

using vx68k::font_rom;
using namespace vm68k::types;
using namespace std;

void
font_rom::copy_data(const console *c)
{
  unsigned char *p = data + 0x3a800;
  for (unsigned int i = 0; i != 0x100; ++i)
    {
      c->get_b16_image(i, p, 1);
      p += 16;
    }
}

uint_type
font_rom::get_16(int fc, uint32_type address) const
{
#ifdef HAVE_NANA_H
  L("class font_rom: get_16 fc=%d address=%#010x\n", fc, address);
#endif
  address &= 0xfffff;
  if (address >= 0xc0000)
    return 0;
  else
    {
      uint32_type i = address / 2;
      uint_type value = vm68k::getw(data + 2 * i);
      return value;
    }
}

uint_type
font_rom::get_8(int fc, uint32_type address) const
{
#ifdef HAVE_NANA_H
  L("class font_rom: get_8 fc=%d address=%#010x\n", fc, address);
#endif
  address &= 0xfffff;
  if (address >= 0xc0000)
    return 0;
  else
    {
      uint_type value = data[address];
      return value;
    }
}

void
font_rom::put_16(int, uint32_type, uint_type)
{
#ifdef HAVE_NANA_H
  L("class font_rom: FIXME: `put_16' not implemented\n");
#endif
}

void
font_rom::put_8(int, uint32_type, uint_type)
{
#ifdef HAVE_NANA_H
  L("class font_rom: FIXME: `put_8' not implemented\n");
#endif
}

font_rom::~font_rom()
{
  delete [] data;
}

font_rom::font_rom()
  : data(NULL)
{
  data = new unsigned char [0xc0000];
}
