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

using vx68k::crtc_memory;
using namespace vm68k::types;
using namespace std;

uint_type
crtc_memory::get_16(int fc, uint32_type address) const
{
#ifdef HAVE_NANA_H
  L("class crtc_memory: get_16 fc=%d address=%#010x\n", fc, address);
#endif
  address &= 0x1fff;
  switch (address) {
  case 0x28:
    return 0x417;
  default:
    return 0;
  }
}

uint_type
crtc_memory::get_8(int fc, uint32_type address) const
{
#ifdef HAVE_NANA_H
  L("class crtc_memory: get_8 fc=%d address=%#010x\n", fc, address);
#endif
  uint_type w = get_16(fc, address & ~1u);
  if (address & 1u)
    return w >> 8 & 0xff;
  else
    return w & 0xff;
}

void
crtc_memory::put_16(int, uint32_type, uint_type)
{
#ifdef HAVE_NANA_H
  L("class crtc_memory: FIXME: `put_16' not implemented\n");
#endif
}

void
crtc_memory::put_8(int, uint32_type, uint_type)
{
#ifdef HAVE_NANA_H
  L("class crtc_memory: FIXME: `put_8' not implemented\n");
#endif
}
