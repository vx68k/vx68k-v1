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
#include <cstdlib>

#ifdef HAVE_NANA_H
# include <nana.h>
#else
# include <cassert>
# define I assert
#endif

using vx68k::fdc_memory;
using namespace vm68k::types;
using namespace std;

namespace vx68k
{
  bool trace_fdc_memory = getenv("VX68K_TRACE_FDC_MEMORY") != 0;
}

void
fdc_memory::install_iocs_calls(system_rom &rom)
{
  //unsigned long data = reinterpret_cast<unsigned long>(this);
}

int
fdc_memory::get_8(uint32_type address, function_code fc) const
{
  address &= 0xffffffff;
#ifdef LG
  LG(trace_fdc_memory, "class fdc_memory: get_8: fc=%d address=0x%08lx\n", fc,
     address + 0L);
#endif

  static bool once;
  if (!once++)
    fprintf(stderr, "class fdc_memory: FIXME: `get_8' not implemented\n");
  static int value = 0;
  value ^= 0xd0;
  return value;
}

uint16_type
fdc_memory::get_16(uint32_type address, function_code fc) const
{
  address &= 0xffffffff & ~1;
#ifdef LG
  LG(trace_fdc_memory, "class fdc_memory: get_16: fc=%d address=0x%08lx\n", fc,
     address + 0L);
#endif

  static bool once;
  if (!once++)
    fprintf(stderr,
	    "class fdc_memory: FIXME: `get_16' not implemented\n");
  return 0;
}

void
fdc_memory::put_8(uint32_type address, int value, function_code fc)
{
  address &= 0xffffffff;
  value &= 0xff;
#ifdef LG
  LG(trace_fdc_memory,
     "class fdc_memory: put_8: fc=%d address=0x%08lx value=0x%02x\n", fc,
     address + 0L, value);
#endif

  static bool once;
  if (!once++)
    fprintf(stderr, "class fdc_memory: FIXME: `put_8' not implemented\n");
}

void
fdc_memory::put_16(uint32_type address, uint16_type value, function_code fc)
{
  address &= 0xffffffff & ~1;
  value &= 0xff;
#ifdef LG
  LG(trace_fdc_memory,
     "class fdc_memory: put_16: fc=%d address=0x%08lx value=0x%02x\n", fc,
     address + 0L, value);
#endif

  static bool once;
  if (!once++)
    fprintf(stderr, "class fdc_memory: FIXME: `put_16' not implemented\n");
}
