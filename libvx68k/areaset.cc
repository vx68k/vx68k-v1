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

using vx68k::area_set;
using namespace vm68k::types;
using namespace std;

uint_type
area_set::get_16(int fc, uint32_type address) const
{
#ifdef HAVE_NANA_H
  L("class area_set: get_16 fc=%d address=%#010x\n", fc, address);
#endif
  return 0;
}

uint_type
area_set::get_8(int fc, uint32_type address) const
{
#ifdef HAVE_NANA_H
  L("class area_set: get_8 fc=%d address=%#010x\n", fc, address);
#endif
  return 0;
}

void
area_set::put_16(int, uint32_type, uint_type)
{
#ifdef HAVE_NANA_H
  L("class area_set: FIXME: `put_16' not implemented\n");
#endif
}

void
area_set::put_8(int, uint32_type, uint_type)
{
#ifdef HAVE_NANA_H
  L("class area_set: FIXME: `put_8' not implemented\n");
#endif
}

area_set::area_set(main_memory *mm)
  : _mm(mm)
{
}
