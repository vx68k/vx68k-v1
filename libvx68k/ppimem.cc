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

#include <cstdio>

#ifdef HAVE_NANA_H
# include <nana.h>
#else
# include <cassert>
# define I assert
#endif

using vx68k::ppi_memory;
using namespace vm68k::types;
using namespace std;

uint16_type
ppi_memory::get_16(uint32_type address, function_code fc) const
{
#ifdef HAVE_NANA_H
  DL("class ppi_memory: get_16: fc=%d address=0x%08lx\n", fc, address + 0UL);
#endif

  return get_8(address | 1u, fc);
}

int
ppi_memory::get_8(uint32_type address, function_code fc) const
{
  address &= 0xffffffffU;

#ifdef HAVE_NANA_H
  DL("class ppi_memory: get_8: fc=%d address=0x%08lx\n", fc, address + 0UL);
#endif
  unsigned int i = address % 0x2000;

  switch (i) {
  case 0x5:
    return 0;
  default:
    return 0xff;
  }
}

void
ppi_memory::put_16(uint32_type address, uint16_type value, function_code fc)
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
ppi_memory::put_8(uint32_type address, int value, function_code fc)
{
#ifdef HAVE_NANA_H
  DL("class ppi_memory: put_8: fc=%d address=0x%08lx value=0x%02x\n",
     fc, address + 0UL, value);
#endif

  static bool once;
  if (!once++)
    fprintf(stderr, "class ppi_memory: FIXME: `put_8' not implemented\n");
}
