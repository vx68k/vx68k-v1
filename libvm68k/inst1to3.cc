/* Virtual X68000 - X68000 virtual machine
   Copyright (C) 1998-2000 Hypercore Software Design, Ltd.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307, USA.  */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#undef const
#undef inline

#include <vm68k/addressing.h>
#include <vm68k/processor.h>

#include <cstdio>

#include "inst.h"

#ifdef HAVE_NANA_H
# include <nana.h>
#else
# include <cassert>
# define I assert
# define VL(EXPR)
#endif

using vm68k::exec_unit;
using vm68k::byte_size;
using vm68k::word_size;
using vm68k::long_word_size;
using namespace vm68k::types;
using namespace vm68k::addressing;

#ifdef HAVE_NANA_H
extern bool nana_instruction_trace;
# undef L_DEFAULT_GUARD
# define L_DEFAULT_GUARD nana_instruction_trace
#endif

namespace
{
  using vm68k::context;

  /* Handles a MOVE instruction.  */
  template <class Size, class Source, class Destination> void
  m68k_move(uint16_type op, context &c, unsigned long data)
  {
    Source ea1(op & 0x7, 2);
    Destination ea2(op >> 9 & 0x7, 2 + ea1.extension_size());
#ifdef HAVE_NANA_H
    L("\tmove%s %s,%s\n", Size::suffix(), ea1.text(c).c_str(),
      ea2.text(c).c_str());
#endif

    typename Size::svalue_type value = ea1.get(c);
    ea2.put(c, value);
    c.regs.ccr.set_cc(value);

    ea1.finish(c);
    ea2.finish(c);
    c.regs.pc += 2 + ea1.extension_size() + ea2.extension_size();
  }

  /* Handles a MOVEA instruction.  */
  template <class Size, class Source> void
  m68k_movea(uint16_type op, context &c, unsigned long data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\tmovea%s %s,%%a%u\n", Size::suffix(), ea1.text(c).c_str(), reg2);
#endif

    // The condition codes are not affected by this instruction.
    long_word_size::svalue_type value = ea1.get(c);
    long_word_size::put(c.regs.a[reg2], value);
    ea1.finish(c);

    c.regs.pc += 2 + ea1.extension_size();
  }
}

void
vm68k::install_instructions_1(exec_unit &eu, unsigned long data)
{
  static const instruction_map inst[]
    = {{0x1000, 0xe07, &m68k_move<byte_size, byte_d_register, byte_d_register>},
       {0x1010, 0xe07, &m68k_move<byte_size, byte_indirect, byte_d_register>},
       {0x1018, 0xe07, &m68k_move<byte_size, byte_postinc_indirect, byte_d_register>},
       {0x1020, 0xe07, &m68k_move<byte_size, byte_predec_indirect, byte_d_register>},
       {0x1028, 0xe07, &m68k_move<byte_size, byte_disp_indirect, byte_d_register>},
       {0x1030, 0xe07, &m68k_move<byte_size, byte_index_indirect, byte_d_register>},
       {0x1038, 0xe00, &m68k_move<byte_size, byte_abs_short, byte_d_register>},
       {0x1039, 0xe00, &m68k_move<byte_size, byte_abs_long, byte_d_register>},
       {0x103a, 0xe00, &m68k_move<byte_size, byte_disp_pc_indirect, byte_d_register>},
       {0x103b, 0xe00, &m68k_move<byte_size, byte_index_pc_indirect, byte_d_register>},
       {0x103c, 0xe00, &m68k_move<byte_size, byte_immediate, byte_d_register>},
       {0x1080, 0xe07, &m68k_move<byte_size, byte_d_register, byte_indirect>},
       {0x1090, 0xe07, &m68k_move<byte_size, byte_indirect, byte_indirect>},
       {0x1098, 0xe07, &m68k_move<byte_size, byte_postinc_indirect, byte_indirect>},
       {0x10a0, 0xe07, &m68k_move<byte_size, byte_predec_indirect, byte_indirect>},
       {0x10a8, 0xe07, &m68k_move<byte_size, byte_disp_indirect, byte_indirect>},
       {0x10b0, 0xe07, &m68k_move<byte_size, byte_index_indirect, byte_indirect>},
       {0x10b8, 0xe00, &m68k_move<byte_size, byte_abs_short, byte_indirect>},
       {0x10b9, 0xe00, &m68k_move<byte_size, byte_abs_long, byte_indirect>},
       {0x10ba, 0xe00, &m68k_move<byte_size, byte_disp_pc_indirect, byte_indirect>},
       {0x10bb, 0xe00, &m68k_move<byte_size, byte_index_pc_indirect, byte_indirect>},
       {0x10bc, 0xe00, &m68k_move<byte_size, byte_immediate, byte_indirect>},
       {0x10c0, 0xe07, &m68k_move<byte_size, byte_d_register, byte_postinc_indirect>},
       {0x10d0, 0xe07, &m68k_move<byte_size, byte_indirect, byte_postinc_indirect>},
       {0x10d8, 0xe07, &m68k_move<byte_size, byte_postinc_indirect, byte_postinc_indirect>},
       {0x10e0, 0xe07, &m68k_move<byte_size, byte_predec_indirect, byte_postinc_indirect>},
       {0x10e8, 0xe07, &m68k_move<byte_size, byte_disp_indirect, byte_postinc_indirect>},
       {0x10f0, 0xe07, &m68k_move<byte_size, byte_index_indirect, byte_postinc_indirect>},
       {0x10f8, 0xe00, &m68k_move<byte_size, byte_abs_short, byte_postinc_indirect>},
       {0x10f9, 0xe00, &m68k_move<byte_size, byte_abs_long, byte_postinc_indirect>},
       {0x10fa, 0xe00, &m68k_move<byte_size, byte_disp_pc_indirect, byte_postinc_indirect>},
       {0x10fb, 0xe00, &m68k_move<byte_size, byte_index_pc_indirect, byte_postinc_indirect>},
       {0x10fc, 0xe00, &m68k_move<byte_size, byte_immediate, byte_postinc_indirect>},
       {0x1100, 0xe07, &m68k_move<byte_size, byte_d_register, byte_predec_indirect>},
       {0x1110, 0xe07, &m68k_move<byte_size, byte_indirect, byte_predec_indirect>},
       {0x1118, 0xe07, &m68k_move<byte_size, byte_postinc_indirect, byte_predec_indirect>},
       {0x1120, 0xe07, &m68k_move<byte_size, byte_predec_indirect, byte_predec_indirect>},
       {0x1128, 0xe07, &m68k_move<byte_size, byte_disp_indirect, byte_predec_indirect>},
       {0x1130, 0xe07, &m68k_move<byte_size, byte_index_indirect, byte_predec_indirect>},
       {0x1138, 0xe00, &m68k_move<byte_size, byte_abs_short, byte_predec_indirect>},
       {0x1139, 0xe00, &m68k_move<byte_size, byte_abs_long, byte_predec_indirect>},
       {0x113a, 0xe00, &m68k_move<byte_size, byte_disp_pc_indirect, byte_predec_indirect>},
       {0x113b, 0xe00, &m68k_move<byte_size, byte_index_pc_indirect, byte_predec_indirect>},
       {0x113c, 0xe00, &m68k_move<byte_size, byte_immediate, byte_predec_indirect>},
       {0x1140, 0xe07, &m68k_move<byte_size, byte_d_register, byte_disp_indirect>},
       {0x1150, 0xe07, &m68k_move<byte_size, byte_indirect, byte_disp_indirect>},
       {0x1158, 0xe07, &m68k_move<byte_size, byte_postinc_indirect, byte_disp_indirect>},
       {0x1160, 0xe07, &m68k_move<byte_size, byte_predec_indirect, byte_disp_indirect>},
       {0x1168, 0xe07, &m68k_move<byte_size, byte_disp_indirect, byte_disp_indirect>},
       {0x1170, 0xe07, &m68k_move<byte_size, byte_index_indirect, byte_disp_indirect>},
       {0x1178, 0xe00, &m68k_move<byte_size, byte_abs_short, byte_disp_indirect>},
       {0x1179, 0xe00, &m68k_move<byte_size, byte_abs_long, byte_disp_indirect>},
       {0x117a, 0xe00, &m68k_move<byte_size, byte_disp_pc_indirect, byte_disp_indirect>},
       {0x117b, 0xe00, &m68k_move<byte_size, byte_index_pc_indirect, byte_disp_indirect>},
       {0x117c, 0xe00, &m68k_move<byte_size, byte_immediate, byte_disp_indirect>},
       {0x1180, 0xe07, &m68k_move<byte_size, byte_d_register, byte_index_indirect>},
       {0x1190, 0xe07, &m68k_move<byte_size, byte_indirect, byte_index_indirect>},
       {0x1198, 0xe07, &m68k_move<byte_size, byte_postinc_indirect, byte_index_indirect>},
       {0x11a0, 0xe07, &m68k_move<byte_size, byte_predec_indirect, byte_index_indirect>},
       {0x11a8, 0xe07, &m68k_move<byte_size, byte_disp_indirect, byte_index_indirect>},
       {0x11b0, 0xe07, &m68k_move<byte_size, byte_index_indirect, byte_index_indirect>},
       {0x11b8, 0xe00, &m68k_move<byte_size, byte_abs_short, byte_index_indirect>},
       {0x11b9, 0xe00, &m68k_move<byte_size, byte_abs_long, byte_index_indirect>},
       {0x11ba, 0xe00, &m68k_move<byte_size, byte_disp_pc_indirect, byte_index_indirect>},
       {0x11bb, 0xe00, &m68k_move<byte_size, byte_index_pc_indirect, byte_index_indirect>},
       {0x11bc, 0xe00, &m68k_move<byte_size, byte_immediate, byte_index_indirect>},
       {0x11c0,     7, &m68k_move<byte_size, byte_d_register, byte_abs_short>},
       {0x11d0,     7, &m68k_move<byte_size, byte_indirect, byte_abs_short>},
       {0x11d8,     7, &m68k_move<byte_size, byte_postinc_indirect, byte_abs_short>},
       {0x11e0,     7, &m68k_move<byte_size, byte_predec_indirect, byte_abs_short>},
       {0x11e8,     7, &m68k_move<byte_size, byte_disp_indirect, byte_abs_short>},
       {0x11f0,     7, &m68k_move<byte_size, byte_index_indirect, byte_abs_short>},
       {0x11f8,     0, &m68k_move<byte_size, byte_abs_short, byte_abs_short>},
       {0x11f9,     0, &m68k_move<byte_size, byte_abs_long, byte_abs_short>},
       {0x11fa,     0, &m68k_move<byte_size, byte_disp_pc_indirect, byte_abs_short>},
       {0x11fb,     0, &m68k_move<byte_size, byte_index_pc_indirect, byte_abs_short>},
       {0x11fc,     0, &m68k_move<byte_size, byte_immediate, byte_abs_short>},
       {0x13c0,     7, &m68k_move<byte_size, byte_d_register, byte_abs_long>},
       {0x13d0,     7, &m68k_move<byte_size, byte_indirect, byte_abs_long>},
       {0x13d8,     7, &m68k_move<byte_size, byte_postinc_indirect, byte_abs_long>},
       {0x13e0,     7, &m68k_move<byte_size, byte_predec_indirect, byte_abs_long>},
       {0x13e8,     7, &m68k_move<byte_size, byte_disp_indirect, byte_abs_long>},
       {0x13f0,     7, &m68k_move<byte_size, byte_index_indirect, byte_abs_long>},
       {0x13f8,     0, &m68k_move<byte_size, byte_abs_short, byte_abs_long>},
       {0x13f9,     0, &m68k_move<byte_size, byte_abs_long, byte_abs_long>},
       {0x13fa,     0, &m68k_move<byte_size, byte_disp_pc_indirect, byte_abs_long>},
       {0x13fb,     0, &m68k_move<byte_size, byte_index_pc_indirect, byte_abs_long>},
       {0x13fc,     0, &m68k_move<byte_size, byte_immediate, byte_abs_long>}};

  for (const instruction_map *i = inst + 0;
       i != inst + sizeof inst / sizeof inst[0]; ++i)
    eu.set_instruction(i->base, i->mask, make_pair(i->handler, data));
}

void
vm68k::install_instructions_2(exec_unit &eu, unsigned long data)
{
  static const instruction_map inst[]
    = {{0x2000, 0xe07, &m68k_move<long_word_size, long_word_d_register, long_word_d_register>},
       {0x2008, 0xe07, &m68k_move<long_word_size, long_word_a_register, long_word_d_register>},
       {0x2010, 0xe07, &m68k_move<long_word_size, long_word_indirect, long_word_d_register>},
       {0x2018, 0xe07, &m68k_move<long_word_size, long_word_postinc_indirect, long_word_d_register>},
       {0x2020, 0xe07, &m68k_move<long_word_size, long_word_predec_indirect, long_word_d_register>},
       {0x2028, 0xe07, &m68k_move<long_word_size, long_word_disp_indirect, long_word_d_register>},
       {0x2030, 0xe07, &m68k_move<long_word_size, long_word_index_indirect, long_word_d_register>},
       {0x2038, 0xe00, &m68k_move<long_word_size, long_word_abs_short, long_word_d_register>},
       {0x2039, 0xe00, &m68k_move<long_word_size, long_word_abs_long, long_word_d_register>},
       {0x203a, 0xe00, &m68k_move<long_word_size, long_word_disp_pc_indirect, long_word_d_register>},
       {0x203b, 0xe00, &m68k_move<long_word_size, long_word_index_pc_indirect, long_word_d_register>},
       {0x203c, 0xe00, &m68k_move<long_word_size, long_word_immediate, long_word_d_register>},
       {0x2040, 0xe07, &m68k_movea<long_word_size, long_word_d_register>},
       {0x2048, 0xe07, &m68k_movea<long_word_size, long_word_a_register>},
       {0x2050, 0xe07, &m68k_movea<long_word_size, long_word_indirect>},
       {0x2058, 0xe07, &m68k_movea<long_word_size, long_word_postinc_indirect>},
       {0x2060, 0xe07, &m68k_movea<long_word_size, long_word_predec_indirect>},
       {0x2068, 0xe07, &m68k_movea<long_word_size, long_word_disp_indirect>},
       {0x2070, 0xe07, &m68k_movea<long_word_size, long_word_index_indirect>},
       {0x2078, 0xe00, &m68k_movea<long_word_size, long_word_abs_short>},
       {0x2079, 0xe00, &m68k_movea<long_word_size, long_word_abs_long>},
       {0x207a, 0xe00, &m68k_movea<long_word_size, long_word_disp_pc_indirect>},
       {0x207b, 0xe00, &m68k_movea<long_word_size, long_word_index_pc_indirect>},
       {0x207c, 0xe00, &m68k_movea<long_word_size, long_word_immediate>},
       {0x2080, 0xe07, &m68k_move<long_word_size, long_word_d_register, long_word_indirect>},
       {0x2088, 0xe07, &m68k_move<long_word_size, long_word_a_register, long_word_indirect>},
       {0x2090, 0xe07, &m68k_move<long_word_size, long_word_indirect, long_word_indirect>},
       {0x2098, 0xe07, &m68k_move<long_word_size, long_word_postinc_indirect, long_word_indirect>},
       {0x20a0, 0xe07, &m68k_move<long_word_size, long_word_predec_indirect, long_word_indirect>},
       {0x20a8, 0xe07, &m68k_move<long_word_size, long_word_disp_indirect, long_word_indirect>},
       {0x20b0, 0xe07, &m68k_move<long_word_size, long_word_index_indirect, long_word_indirect>},
       {0x20b8, 0xe00, &m68k_move<long_word_size, long_word_abs_short, long_word_indirect>},
       {0x20b9, 0xe00, &m68k_move<long_word_size, long_word_abs_long, long_word_indirect>},
       {0x20ba, 0xe00, &m68k_move<long_word_size, long_word_disp_pc_indirect, long_word_indirect>},
       {0x20bb, 0xe00, &m68k_move<long_word_size, long_word_index_pc_indirect, long_word_indirect>},
       {0x20bc, 0xe00, &m68k_move<long_word_size, long_word_immediate, long_word_indirect>},
       {0x20c0, 0xe07, &m68k_move<long_word_size, long_word_d_register, long_word_postinc_indirect>},
       {0x20c8, 0xe07, &m68k_move<long_word_size, long_word_a_register, long_word_postinc_indirect>},
       {0x20d0, 0xe07, &m68k_move<long_word_size, long_word_indirect, long_word_postinc_indirect>},
       {0x20d8, 0xe07, &m68k_move<long_word_size, long_word_postinc_indirect, long_word_postinc_indirect>},
       {0x20e0, 0xe07, &m68k_move<long_word_size, long_word_predec_indirect, long_word_postinc_indirect>},
       {0x20e8, 0xe07, &m68k_move<long_word_size, long_word_disp_indirect, long_word_postinc_indirect>},
       {0x20f0, 0xe07, &m68k_move<long_word_size, long_word_index_indirect, long_word_postinc_indirect>},
       {0x20f8, 0xe00, &m68k_move<long_word_size, long_word_abs_short, long_word_postinc_indirect>},
       {0x20f9, 0xe00, &m68k_move<long_word_size, long_word_abs_long, long_word_postinc_indirect>},
       {0x20fa, 0xe00, &m68k_move<long_word_size, long_word_disp_pc_indirect, long_word_postinc_indirect>},
       {0x20fb, 0xe00, &m68k_move<long_word_size, long_word_index_pc_indirect, long_word_postinc_indirect>},
       {0x20fc, 0xe00, &m68k_move<long_word_size, long_word_immediate, long_word_postinc_indirect>},
       {0x2100, 0xe07, &m68k_move<long_word_size, long_word_d_register, long_word_predec_indirect>},
       {0x2108, 0xe07, &m68k_move<long_word_size, long_word_a_register, long_word_predec_indirect>},
       {0x2110, 0xe07, &m68k_move<long_word_size, long_word_indirect, long_word_predec_indirect>},
       {0x2118, 0xe07, &m68k_move<long_word_size, long_word_postinc_indirect, long_word_predec_indirect>},
       {0x2120, 0xe07, &m68k_move<long_word_size, long_word_predec_indirect, long_word_predec_indirect>},
       {0x2128, 0xe07, &m68k_move<long_word_size, long_word_disp_indirect, long_word_predec_indirect>},
       {0x2130, 0xe07, &m68k_move<long_word_size, long_word_index_indirect, long_word_predec_indirect>},
       {0x2138, 0xe00, &m68k_move<long_word_size, long_word_abs_short, long_word_predec_indirect>},
       {0x2139, 0xe00, &m68k_move<long_word_size, long_word_abs_long, long_word_predec_indirect>},
       {0x213a, 0xe00, &m68k_move<long_word_size, long_word_disp_pc_indirect, long_word_predec_indirect>},
       {0x213b, 0xe00, &m68k_move<long_word_size, long_word_index_pc_indirect, long_word_predec_indirect>},
       {0x213c, 0xe00, &m68k_move<long_word_size, long_word_immediate, long_word_predec_indirect>},
       {0x2140, 0xe07, &m68k_move<long_word_size, long_word_d_register, long_word_disp_indirect>},
       {0x2148, 0xe07, &m68k_move<long_word_size, long_word_a_register, long_word_disp_indirect>},
       {0x2150, 0xe07, &m68k_move<long_word_size, long_word_indirect, long_word_disp_indirect>},
       {0x2158, 0xe07, &m68k_move<long_word_size, long_word_postinc_indirect, long_word_disp_indirect>},
       {0x2160, 0xe07, &m68k_move<long_word_size, long_word_predec_indirect, long_word_disp_indirect>},
       {0x2168, 0xe07, &m68k_move<long_word_size, long_word_disp_indirect, long_word_disp_indirect>},
       {0x2170, 0xe07, &m68k_move<long_word_size, long_word_index_indirect, long_word_disp_indirect>},
       {0x2178, 0xe00, &m68k_move<long_word_size, long_word_abs_short, long_word_disp_indirect>},
       {0x2179, 0xe00, &m68k_move<long_word_size, long_word_abs_long, long_word_disp_indirect>},
       {0x217a, 0xe00, &m68k_move<long_word_size, long_word_disp_pc_indirect, long_word_disp_indirect>},
       {0x217b, 0xe00, &m68k_move<long_word_size, long_word_index_pc_indirect, long_word_disp_indirect>},
       {0x217c, 0xe00, &m68k_move<long_word_size, long_word_immediate, long_word_disp_indirect>},
       {0x2180, 0xe07, &m68k_move<long_word_size, long_word_d_register, long_word_index_indirect>},
       {0x2188, 0xe07, &m68k_move<long_word_size, long_word_a_register, long_word_index_indirect>},
       {0x2190, 0xe07, &m68k_move<long_word_size, long_word_indirect, long_word_index_indirect>},
       {0x2198, 0xe07, &m68k_move<long_word_size, long_word_postinc_indirect, long_word_index_indirect>},
       {0x21a0, 0xe07, &m68k_move<long_word_size, long_word_predec_indirect, long_word_index_indirect>},
       {0x21a8, 0xe07, &m68k_move<long_word_size, long_word_disp_indirect, long_word_index_indirect>},
       {0x21b0, 0xe07, &m68k_move<long_word_size, long_word_index_indirect, long_word_index_indirect>},
       {0x21b8, 0xe00, &m68k_move<long_word_size, long_word_abs_short, long_word_index_indirect>},
       {0x21b9, 0xe00, &m68k_move<long_word_size, long_word_abs_long, long_word_index_indirect>},
       {0x21ba, 0xe00, &m68k_move<long_word_size, long_word_disp_pc_indirect, long_word_index_indirect>},
       {0x21bb, 0xe00, &m68k_move<long_word_size, long_word_index_pc_indirect, long_word_index_indirect>},
       {0x21bc, 0xe00, &m68k_move<long_word_size, long_word_immediate, long_word_index_indirect>},
       {0x21c0,     7, &m68k_move<long_word_size, long_word_d_register, long_word_abs_short>},
       {0x21c8,     7, &m68k_move<long_word_size, long_word_a_register, long_word_abs_short>},
       {0x21d0,     7, &m68k_move<long_word_size, long_word_indirect, long_word_abs_short>},
       {0x21d8,     7, &m68k_move<long_word_size, long_word_postinc_indirect, long_word_abs_short>},
       {0x21e0,     7, &m68k_move<long_word_size, long_word_predec_indirect, long_word_abs_short>},
       {0x21e8,     7, &m68k_move<long_word_size, long_word_disp_indirect, long_word_abs_short>},
       {0x21f0,     7, &m68k_move<long_word_size, long_word_index_indirect, long_word_abs_short>},
       {0x21f8,     0, &m68k_move<long_word_size, long_word_abs_short, long_word_abs_short>},
       {0x21f9,     0, &m68k_move<long_word_size, long_word_abs_long, long_word_abs_short>},
       {0x21fa,     0, &m68k_move<long_word_size, long_word_disp_pc_indirect, long_word_abs_short>},
       {0x21fb,     0, &m68k_move<long_word_size, long_word_index_pc_indirect, long_word_abs_short>},
       {0x21fc,     0, &m68k_move<long_word_size, long_word_immediate, long_word_abs_short>},
       {0x23c0,     7, &m68k_move<long_word_size, long_word_d_register, long_word_abs_long>},
       {0x23c8,     7, &m68k_move<long_word_size, long_word_a_register, long_word_abs_long>},
       {0x23d0,     7, &m68k_move<long_word_size, long_word_indirect, long_word_abs_long>},
       {0x23d8,     7, &m68k_move<long_word_size, long_word_postinc_indirect, long_word_abs_long>},
       {0x23e0,     7, &m68k_move<long_word_size, long_word_predec_indirect, long_word_abs_long>},
       {0x23e8,     7, &m68k_move<long_word_size, long_word_disp_indirect, long_word_abs_long>},
       {0x23f0,     7, &m68k_move<long_word_size, long_word_index_indirect, long_word_abs_long>},
       {0x23f8,     0, &m68k_move<long_word_size, long_word_abs_short, long_word_abs_long>},
       {0x23f9,     0, &m68k_move<long_word_size, long_word_abs_long, long_word_abs_long>},
       {0x23fa,     0, &m68k_move<long_word_size, long_word_disp_pc_indirect, long_word_abs_long>},
       {0x23fb,     0, &m68k_move<long_word_size, long_word_index_pc_indirect, long_word_abs_long>},
       {0x23fc,     0, &m68k_move<long_word_size, long_word_immediate, long_word_abs_long>}};

  for (const instruction_map *i = inst + 0;
       i != inst + sizeof inst / sizeof inst[0]; ++i)
    eu.set_instruction(i->base, i->mask, make_pair(i->handler, data));
}

void
vm68k::install_instructions_3(exec_unit &eu, unsigned long data)
{
  static const instruction_map inst[]
    = {{0x3000, 0xe07, &m68k_move<word_size, word_d_register, word_d_register>},
       {0x3008, 0xe07, &m68k_move<word_size, word_a_register, word_d_register>},
       {0x3010, 0xe07, &m68k_move<word_size, word_indirect, word_d_register>},
       {0x3018, 0xe07, &m68k_move<word_size, word_postinc_indirect, word_d_register>},
       {0x3020, 0xe07, &m68k_move<word_size, word_predec_indirect, word_d_register>},
       {0x3028, 0xe07, &m68k_move<word_size, word_disp_indirect, word_d_register>},
       {0x3030, 0xe07, &m68k_move<word_size, word_index_indirect, word_d_register>},
       {0x3038, 0xe00, &m68k_move<word_size, word_abs_short, word_d_register>},
       {0x3039, 0xe00, &m68k_move<word_size, word_abs_long, word_d_register>},
       {0x303a, 0xe00, &m68k_move<word_size, word_disp_pc_indirect, word_d_register>},
       {0x303b, 0xe00, &m68k_move<word_size, word_index_pc_indirect, word_d_register>},
       {0x303c, 0xe00, &m68k_move<word_size, word_immediate, word_d_register>},
       {0x3040, 0xe07, &m68k_movea<word_size, word_d_register>},
       {0x3048, 0xe07, &m68k_movea<word_size, word_a_register>},
       {0x3050, 0xe07, &m68k_movea<word_size, word_indirect>},
       {0x3058, 0xe07, &m68k_movea<word_size, word_postinc_indirect>},
       {0x3060, 0xe07, &m68k_movea<word_size, word_predec_indirect>},
       {0x3068, 0xe07, &m68k_movea<word_size, word_disp_indirect>},
       {0x3070, 0xe07, &m68k_movea<word_size, word_index_indirect>},
       {0x3078, 0xe00, &m68k_movea<word_size, word_abs_short>},
       {0x3079, 0xe00, &m68k_movea<word_size, word_abs_long>},
       {0x307a, 0xe00, &m68k_movea<word_size, word_disp_pc_indirect>},
       {0x307b, 0xe00, &m68k_movea<word_size, word_index_pc_indirect>},
       {0x307c, 0xe00, &m68k_movea<word_size, word_immediate>},
       {0x3080, 0xe07, &m68k_move<word_size, word_d_register, word_indirect>},
       {0x3088, 0xe07, &m68k_move<word_size, word_a_register, word_indirect>},
       {0x3090, 0xe07, &m68k_move<word_size, word_indirect, word_indirect>},
       {0x3098, 0xe07, &m68k_move<word_size, word_postinc_indirect, word_indirect>},
       {0x30a0, 0xe07, &m68k_move<word_size, word_predec_indirect, word_indirect>},
       {0x30a8, 0xe07, &m68k_move<word_size, word_disp_indirect, word_indirect>},
       {0x30b0, 0xe07, &m68k_move<word_size, word_index_indirect, word_indirect>},
       {0x30b8, 0xe00, &m68k_move<word_size, word_abs_short, word_indirect>},
       {0x30b9, 0xe00, &m68k_move<word_size, word_abs_long, word_indirect>},
       {0x30ba, 0xe00, &m68k_move<word_size, word_disp_pc_indirect, word_indirect>},
       {0x30bb, 0xe00, &m68k_move<word_size, word_index_pc_indirect, word_indirect>},
       {0x30bc, 0xe00, &m68k_move<word_size, word_immediate, word_indirect>},
       {0x30c0, 0xe07, &m68k_move<word_size, word_d_register, word_postinc_indirect>},
       {0x30c8, 0xe07, &m68k_move<word_size, word_a_register, word_postinc_indirect>},
       {0x30d0, 0xe07, &m68k_move<word_size, word_indirect, word_postinc_indirect>},
       {0x30d8, 0xe07, &m68k_move<word_size, word_postinc_indirect, word_postinc_indirect>},
       {0x30e0, 0xe07, &m68k_move<word_size, word_predec_indirect, word_postinc_indirect>},
       {0x30e8, 0xe07, &m68k_move<word_size, word_disp_indirect, word_postinc_indirect>},
       {0x30f0, 0xe07, &m68k_move<word_size, word_index_indirect, word_postinc_indirect>},
       {0x30f8, 0xe00, &m68k_move<word_size, word_abs_short, word_postinc_indirect>},
       {0x30f9, 0xe00, &m68k_move<word_size, word_abs_long, word_postinc_indirect>},
       {0x30fa, 0xe00, &m68k_move<word_size, word_disp_pc_indirect, word_postinc_indirect>},
       {0x30fb, 0xe00, &m68k_move<word_size, word_index_pc_indirect, word_postinc_indirect>},
       {0x30fc, 0xe00, &m68k_move<word_size, word_immediate, word_postinc_indirect>},
       {0x3100, 0xe07, &m68k_move<word_size, word_d_register, word_predec_indirect>},
       {0x3108, 0xe07, &m68k_move<word_size, word_a_register, word_predec_indirect>},
       {0x3110, 0xe07, &m68k_move<word_size, word_indirect, word_predec_indirect>},
       {0x3118, 0xe07, &m68k_move<word_size, word_postinc_indirect, word_predec_indirect>},
       {0x3120, 0xe07, &m68k_move<word_size, word_predec_indirect, word_predec_indirect>},
       {0x3128, 0xe07, &m68k_move<word_size, word_disp_indirect, word_predec_indirect>},
       {0x3130, 0xe07, &m68k_move<word_size, word_index_indirect, word_predec_indirect>},
       {0x3138, 0xe00, &m68k_move<word_size, word_abs_short, word_predec_indirect>},
       {0x3139, 0xe00, &m68k_move<word_size, word_abs_long, word_predec_indirect>},
       {0x313a, 0xe00, &m68k_move<word_size, word_disp_pc_indirect, word_predec_indirect>},
       {0x313b, 0xe00, &m68k_move<word_size, word_index_pc_indirect, word_predec_indirect>},
       {0x313c, 0xe00, &m68k_move<word_size, word_immediate, word_predec_indirect>},
       {0x3140, 0xe07, &m68k_move<word_size, word_d_register, word_disp_indirect>},
       {0x3148, 0xe07, &m68k_move<word_size, word_a_register, word_disp_indirect>},
       {0x3150, 0xe07, &m68k_move<word_size, word_indirect, word_disp_indirect>},
       {0x3158, 0xe07, &m68k_move<word_size, word_postinc_indirect, word_disp_indirect>},
       {0x3160, 0xe07, &m68k_move<word_size, word_predec_indirect, word_disp_indirect>},
       {0x3168, 0xe07, &m68k_move<word_size, word_disp_indirect, word_disp_indirect>},
       {0x3170, 0xe07, &m68k_move<word_size, word_index_indirect, word_disp_indirect>},
       {0x3178, 0xe00, &m68k_move<word_size, word_abs_short, word_disp_indirect>},
       {0x3179, 0xe00, &m68k_move<word_size, word_abs_long, word_disp_indirect>},
       {0x317a, 0xe00, &m68k_move<word_size, word_disp_pc_indirect, word_disp_indirect>},
       {0x317b, 0xe00, &m68k_move<word_size, word_index_pc_indirect, word_disp_indirect>},
       {0x317c, 0xe00, &m68k_move<word_size, word_immediate, word_disp_indirect>},
       {0x3180, 0xe07, &m68k_move<word_size, word_d_register, word_index_indirect>},
       {0x3188, 0xe07, &m68k_move<word_size, word_a_register, word_index_indirect>},
       {0x3190, 0xe07, &m68k_move<word_size, word_indirect, word_index_indirect>},
       {0x3198, 0xe07, &m68k_move<word_size, word_postinc_indirect, word_index_indirect>},
       {0x31a0, 0xe07, &m68k_move<word_size, word_predec_indirect, word_index_indirect>},
       {0x31a8, 0xe07, &m68k_move<word_size, word_disp_indirect, word_index_indirect>},
       {0x31b0, 0xe07, &m68k_move<word_size, word_index_indirect, word_index_indirect>},
       {0x31b8, 0xe00, &m68k_move<word_size, word_abs_short, word_index_indirect>},
       {0x31b9, 0xe00, &m68k_move<word_size, word_abs_long, word_index_indirect>},
       {0x31ba, 0xe00, &m68k_move<word_size, word_disp_pc_indirect, word_index_indirect>},
       {0x31bb, 0xe00, &m68k_move<word_size, word_index_pc_indirect, word_index_indirect>},
       {0x31bc, 0xe00, &m68k_move<word_size, word_immediate, word_index_indirect>},
       {0x31c0,     7, &m68k_move<word_size, word_d_register, word_abs_short>},
       {0x31c8,     7, &m68k_move<word_size, word_a_register, word_abs_short>},
       {0x31d0,     7, &m68k_move<word_size, word_indirect, word_abs_short>},
       {0x31d8,     7, &m68k_move<word_size, word_postinc_indirect, word_abs_short>},
       {0x31e0,     7, &m68k_move<word_size, word_predec_indirect, word_abs_short>},
       {0x31e8,     7, &m68k_move<word_size, word_disp_indirect, word_abs_short>},
       {0x31f0,     7, &m68k_move<word_size, word_index_indirect, word_abs_short>},
       {0x31f8,     0, &m68k_move<word_size, word_abs_short, word_abs_short>},
       {0x31f9,     0, &m68k_move<word_size, word_abs_long, word_abs_short>},
       {0x31fa,     0, &m68k_move<word_size, word_disp_pc_indirect, word_abs_short>},
       {0x31fb,     0, &m68k_move<word_size, word_index_pc_indirect, word_abs_short>},
       {0x31fc,     0, &m68k_move<word_size, word_immediate, word_abs_short>},
       {0x33c0,     7, &m68k_move<word_size, word_d_register, word_abs_long>},
       {0x33c8,     7, &m68k_move<word_size, word_a_register, word_abs_long>},
       {0x33d0,     7, &m68k_move<word_size, word_indirect, word_abs_long>},
       {0x33d8,     7, &m68k_move<word_size, word_postinc_indirect, word_abs_long>},
       {0x33e0,     7, &m68k_move<word_size, word_predec_indirect, word_abs_long>},
       {0x33e8,     7, &m68k_move<word_size, word_disp_indirect, word_abs_long>},
       {0x33f0,     7, &m68k_move<word_size, word_index_indirect, word_abs_long>},
       {0x33f8,     0, &m68k_move<word_size, word_abs_short, word_abs_long>},
       {0x33f9,     0, &m68k_move<word_size, word_abs_long, word_abs_long>},
       {0x33fa,     0, &m68k_move<word_size, word_disp_pc_indirect, word_abs_long>},
       {0x33fb,     0, &m68k_move<word_size, word_index_pc_indirect, word_abs_long>},
       {0x33fc,     0, &m68k_move<word_size, word_immediate, word_abs_long>}};

  for (const instruction_map *i = inst + 0;
       i != inst + sizeof inst / sizeof inst[0]; ++i)
    eu.set_instruction(i->base, i->mask, make_pair(i->handler, data));
}
