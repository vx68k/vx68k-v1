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

using vx68k::dmac_memory;
using vm68k::memory;
using namespace vm68k::types;
using namespace std;

#ifdef HAVE_NANA_H
extern bool nana_iocs_call_trace;
#endif

uint_type
dmac_memory::get_16(function_code fc, uint32_type address) const
{
#ifdef HAVE_NANA_H
  DL("class dmac_memory: get_16: fc=%d address=0x%08x\n", fc, address + 0UL);
#endif
  static bool once;
  if (!once++)
    fprintf(stderr, "class dmac_memory: FIXME: `get_16' not implemented\n");
  return 0;
}

unsigned int
dmac_memory::get_8(function_code fc, uint32_type address) const
{
#ifdef HAVE_NANA_H
  DL("class dmac_memory: get_8: fc=%d address=0x%08x\n", fc, address + 0UL);
#endif
  static bool once;
  if (!once++)
    fprintf(stderr, "class dmac_memory: FIXME: `get_8' not implemented\n");
  return 0;
}

void
dmac_memory::put_16(function_code fc, uint32_type address, uint_type value)
{
#ifdef HAVE_NANA_H
  DL("class dmac_memory: put_16: fc=%d address=0x%08x value=0x%04x\n",
     fc, address + 0UL, value);
#endif
  static bool once;
  if (!once++)
    fprintf(stderr, "class dmac_memory: FIXME: `put_16' not implemented\n");
}

void
dmac_memory::put_8(function_code fc, uint32_type address, unsigned int value)
{
#ifdef HAVE_NANA_H
  DL("class dmac_memory: put_8: fc=%d address=0x%08x value=0x%02x\n",
     fc, address + 0UL, value);
#endif
  static bool once;
  if (!once++)
    fprintf(stderr, "class dmac_memory: FIXME: `put_8' not implemented\n");
}

namespace
{
  using vm68k::byte_size;
  using vm68k::word_size;
  using vm68k::long_word_size;
  using vm68k::context;
  using vx68k::system_rom;

  /* Handles a _DMAMOVE call.  */
  void
  iocs_dmamove(context &c, unsigned long data)
  {
    unsigned int mode = byte_size::get(c.regs.d[1]);
    uint32_type n = long_word_size::get(c.regs.d[2]);
    uint32_type i = long_word_size::get(c.regs.a[1]);
    uint32_type j = long_word_size::get(c.regs.a[2]);
#ifdef HAVE_NANA_H
    LG(nana_iocs_call_trace,
       "IOCS _DMAMOVE; %%d1:b=0x%02x %%d2=0x%08lx %%a1=0x%08lx %%a2=0x%08lx\n",
       mode, n + 0UL, i + 0UL, j + 0UL);
#endif

    static const int increments[4] = {0, 1, -1, 0};
    int i_inc = increments[mode & 0x3];
    int j_inc = increments[mode >> 2 & 0x3];

    if (mode >> 7 & 0x1)
      {
	// Copy from %a2@ to %a1@.
	while (n-- > 0)
	  {
	    byte_size::put(*c.mem, memory::SUPER_DATA, i,
			   byte_size::get(*c.mem, memory::SUPER_DATA, j));
	    j += j_inc;
	    i += i_inc;
	  }
      }
    else
      {
	// Copy from %a1@ to %a2@.
	while (n-- > 0)
	  {
	    byte_size::put(*c.mem, memory::SUPER_DATA, j,
			   byte_size::get(*c.mem, memory::SUPER_DATA, i));
	    i += i_inc;
	    j += j_inc;
	  }
      }
  }

  /* Installs serial and mouse IOCS calls to the BIOS ROM.  */
  void
  install_iocs_calls(system_rom &bios, unsigned long data)
  {
    bios.set_iocs_function(0x8a, make_pair(&iocs_dmamove, data));
    // 0x8b _DMAMOV_A
    // 0x8c _DMAMOV_L
    // 0x8d _DMAMODE
  }
}

dmac_memory::~dmac_memory()
{
}

dmac_memory::dmac_memory(system_rom &bios)
{
  install_iocs_calls(bios, reinterpret_cast<unsigned long>(this));
}
