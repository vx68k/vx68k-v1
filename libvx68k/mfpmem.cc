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

uint_type
mfp_memory::get_16(int fc, uint32_type address) const
{
#ifdef HAVE_NANA_H
  L("class mfp_memory: get_16: fc=%d address=0x%08lx\n",
    fc, (unsigned long) address);
#endif
  fprintf(stderr,
	  "class mfp_memory: FIXME: `get_16' not implemented\n");
  return 0x7b;
}

unsigned int
mfp_memory::get_8(int fc, uint32_type address) const
{
#ifdef HAVE_NANA_H
  L("class mfp_memory: get_8: fc=%d address=0x%08lx\n",
    fc, (unsigned long) address);
#endif
  fprintf(stderr,
	  "class mfp_memory: FIXME: `get_8' not implemented\n");
  static unsigned int v = 0xfb;
  v ^= 0x90;
  return v;
}

void
mfp_memory::put_16(int fc, uint32_type address, uint_type value)
{
#ifdef HAVE_NANA_H
  L("class opm_memory: put_16: fc=%d address=0x%08lx value=0x%04x\n",
    fc, (unsigned long) address, value);
#endif
  fprintf(stderr,
	  "class mfp_memory: FIXME: `put_16' not implemented\n");
}

void
mfp_memory::put_8(int fc, uint32_type address, unsigned int value)
{
#ifdef HAVE_NANA_H
  L("class opm_memory: put_8: fc=%d address=0x%08lx value=0x%02x\n",
    fc, (unsigned long) address, value);
#endif
  fprintf(stderr,
	  "class mfp_memory: FIXME: `put_8' not implemented\n");
}
