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

using vx68k::scc_memory;
using namespace vm68k::types;
using namespace std;

#ifdef HAVE_NANA_H
extern bool nana_iocs_call_trace;
#endif

uint_type
scc_memory::get_16(int fc, uint32_type address) const
{
#ifdef HAVE_NANA_H
  L("class scc_memory: get_16 fc=%d address=%#010x\n", fc, address);
#endif
  return get_8(fc, address | 1u);
}

uint_type
scc_memory::get_8(int fc, uint32_type address) const
{
#ifdef HAVE_NANA_H
  L("class scc_memory: get_8 fc=%d address=%#010x\n", fc, address);
#endif
  address &= 0x1fff;
  switch (address) {
  default:
    return 0;
  }
}

void
scc_memory::put_16(int, uint32_type, uint_type)
{
#ifdef HAVE_NANA_H
  L("class scc_memory: FIXME: `put_16' not implemented\n");
#endif
}

void
scc_memory::put_8(int, uint32_type, uint_type)
{
#ifdef HAVE_NANA_H
  L("class scc_memory: FIXME: `put_8' not implemented\n");
#endif
}

namespace
{
  using vm68k::word_size;
  using vm68k::long_word_size;
  using vm68k::context;
  using vx68k::system_rom;

  /* Handles a _MS_CURGT IOCS call.  */
  void
  iocs_ms_curgt(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    LG(nana_iocs_call_trace,
       "IOCS _MS_CURGT\n");
#endif

    static bool once;
    if (!once++)
      fprintf(stderr, "iocs_ms_curgt: FIXME: not implemented\n");
    long_word_size::put(c.regs.d[0], 0);
  }

  /* Handles a _MS_CURST IOCS call.  */
  void
  iocs_ms_curst(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    LG(nana_iocs_call_trace,
       "IOCS _MS_CURST; %%d1=0x%08lx\n",
       long_word_size::get(c.regs.d[1]) + 0UL);
#endif

    fprintf(stderr, "iocs_ms_curst: FIXME: not implemented\n");
  }

  /* Handles a _MS_GETDT IOCS call.  */
  void
  iocs_ms_getdt(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    LG(nana_iocs_call_trace,
       "IOCS _MS_GETDT\n");
#endif

    static bool once;
    if (!once++)
      fprintf(stderr, "iocs_ms_getdt: FIXME: not implemented\n");
    long_word_size::put(c.regs.d[0], 0);
  }

  /* Handles a _MS_INIT IOCS call.  */
  void
  iocs_ms_init(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    LG(nana_iocs_call_trace,
       "IOCS _MS_INIT\n");
#endif

    fprintf(stderr, "iocs_ms_init: FIXME: not implemented\n");
  }

  /* Handles a _MS_LIMIT IOCS call.  */
  void
  iocs_ms_limit(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    LG(nana_iocs_call_trace,
       "IOCS _MS_LIMIT; %%d1=0x%08lx %%d2=0x%08lx\n",
       long_word_size::get(c.regs.d[1]) + 0UL,
       long_word_size::get(c.regs.d[2]) + 0UL);
#endif

    fprintf(stderr, "iocs_ms_limit: FIXME: not implemented\n");
  }

  /* Handles a _SET232C IOCS call.  */
  void
  iocs_set232c(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    LG(nana_iocs_call_trace,
       "IOCS _SET232C; %%d1:w=0x%04x\n", word_size::get(c.regs.d[1]));
#endif

    fprintf(stderr, "iocs_set232c: FIXME: not implemented\n");
    c.regs.d[0] = 0;
  }

  /* Installs serial and mouse IOCS calls to the BIOS ROM.  */
  void
  install_iocs_calls(system_rom &bios, unsigned long data)
  {
    bios.set_iocs_function(0x30, make_pair(&iocs_set232c, data));
    // 0x31 _LOF232C
    // 0x32 _INP232C
    // 0x33 _ISNS232C
    // 0x34 _OSNS232C
    // 0x35 _OUT232C
    bios.set_iocs_function(0x70, make_pair(&iocs_ms_init, data));
    // 0x71 _MS_CURON
    // 0x72 _MS_CUROF
    // 0x73 _MS_STAT
    bios.set_iocs_function(0x74, make_pair(&iocs_ms_getdt, data));
    bios.set_iocs_function(0x75, make_pair(&iocs_ms_curgt, data));
    bios.set_iocs_function(0x76, make_pair(&iocs_ms_curst, data));
    bios.set_iocs_function(0x77, make_pair(&iocs_ms_limit, data));
    // 0x78 _MS_OFFTM
    // 0x79 _MS_ONTM
    // 0x7a _MS_PATST
    // 0x7b _MS_SEL
    // 0x7c _MS_SEL2
  }
}

scc_memory::~scc_memory()
{
}

scc_memory::scc_memory(system_rom &bios)
{
  install_iocs_calls(bios, reinterpret_cast<unsigned long>(this));
}
