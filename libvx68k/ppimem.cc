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

using vx68k::ppi_memory;
using namespace vm68k::types;
using namespace std;

uint_type
ppi_memory::get_16(int fc, uint32_type address) const
{
#ifdef HAVE_NANA_H
  DL("class ppi_memory: get_16: fc=%d address=0x%08lx\n", fc, address + 0UL);
#endif

  return get_8(fc, address | 1u);
}

uint_type
ppi_memory::get_8(int fc, uint32_type address) const
{
#ifdef HAVE_NANA_H
  DL("class ppi_memory: get_8: fc=%d address=0x%08lx\n", fc, address + 0UL);
#endif

  address &= 0x1fff;
  switch (address) {
  default:
    return 0;
  }
}

void
ppi_memory::put_16(int fc, uint32_type address, uint_type value)
{
#ifdef HAVE_NANA_H
  DL("class ppi_memory: put_16: fc=%d address=0x%08lx value=0x%04x\n",
     fc, address + 0UL, value);
#endif

  static bool once;
  if (!once++)
    fprintf(stderr, "class ppi_memory: FIXME: `put_16' not implemented\n");
}

void
ppi_memory::put_8(int fc, uint32_type address, uint_type value)
{
#ifdef HAVE_NANA_H
  DL("class ppi_memory: put_8: fc=%d address=0x%08lx value=0x%02x\n",
     fc, address + 0UL, value);
#endif

  static bool once;
  if (!once++)
    fprintf(stderr, "class ppi_memory: FIXME: `put_8' not implemented\n");
}
