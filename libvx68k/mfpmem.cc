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

using vx68k::mfp_memory;
using namespace vm68k::types;
using namespace std;

uint16_type
mfp_memory::get_16(uint32_type address, function_code fc) const
  throw (memory_exception)
{
#ifdef DL
  DL("class mfp_memory: get_16: fc=%d address=0x%08lx\n",
     fc, address + 0UL);
#endif

  static bool once;
  if (!once++)
    fprintf(stderr, "class mfp_memory: FIXME: `get_16' not implemented\n");
  return 0x7b;
}

int
mfp_memory::get_8(uint32_type address, function_code fc) const
  throw (memory_exception)
{
#ifdef DL
  DL("class mfp_memory: get_8: fc=%d address=0x%08lx\n",
     fc, address + 0UL);
#endif

  static bool once;
  if (!once++)
    fprintf(stderr, "class mfp_memory: FIXME: `get_8' not implemented\n");
  static unsigned int v = 0xfb;
  v ^= 0x90;
  return v;
}

void
mfp_memory::put_16(uint32_type address, uint16_type value, function_code fc)
  throw (memory_exception)
{
#ifdef DL
  DL("class opm_memory: put_16: fc=%d address=0x%08lx value=0x%04x\n",
     fc, address + 0UL, value);
#endif

  static bool once;
  if (!once++)
    fprintf(stderr, "class mfp_memory: FIXME: `put_16' not implemented\n");
}

void
mfp_memory::put_8(uint32_type address, int value, function_code fc)
  throw (memory_exception)
{
#ifdef DL
  DL("class opm_memory: put_8: fc=%d address=0x%08lx value=0x%02x\n",
     fc, address + 0UL, value);
#endif

  static bool once;
  if (!once++)
    fprintf(stderr, "class mfp_memory: FIXME: `put_8' not implemented\n");
}
