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

#include "instr.h"

#include <vm68k/addressing.h>
#include <vm68k/processor.h>

#include <cstdio>

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

  /* Handles a CMP instruction.  */
  template <class Size, class Source> void
  m68k_cmp(uint16_type op, context &c, unsigned long data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\tcmp%s %s,%%d%u\n", Size::suffix(), ea1.text(c).c_str(), reg2);
#endif

    typename Size::svalue_type value1 = ea1.get(c);
    typename Size::svalue_type value2 = Size::get(c.regs.d[reg2]);
    typename Size::svalue_type value = Size::svalue(value2 - value1);
    c.regs.ccr.set_cc_cmp(value, value2, value1);
    ea1.finish(c);

    c.regs.pc += 2 + ea1.extension_size();
  }

  /* Handles a CMPA instruction.  */
  template <class Size, class Source> void
  m68k_cmpa(uint16_type op, context &c, unsigned long data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\tcmpa%s %s,%%a%u\n", Size::suffix(), ea1.text(c).c_str(), reg2);
#endif

    long_word_size::svalue_type value1 = ea1.get(c);
    long_word_size::svalue_type value2 = long_word_size::get(c.regs.a[reg2]);
    long_word_size::svalue_type value
      = long_word_size::svalue(value2 - value1);
    c.regs.ccr.set_cc_cmp(value, value2, value1);

    ea1.finish(c);
    long_word_size::put(c.regs.pc, c.regs.pc + 2 + Source::extension_size());
  }

  /* Handles a CMPM instruction.  */
  template <class Size> void
  m68k_cmpm(uint16_type op, context &c, unsigned long data)
  {
    basic_postinc_indirect<Size> ea1(op & 0x7, 2);
    basic_postinc_indirect<Size> ea2(op >> 9 & 0x7, 2 + ea1.extension_size());
#ifdef HAVE_NANA_H
    L("\tcmpm%s %s,%s\n", Size::suffix(), ea1.text(c).c_str(),
      ea2.text(c).c_str());
#endif

    typename Size::svalue_type value1 = ea1.get(c);
    typename Size::svalue_type value2 = ea2.get(c);
    typename Size::svalue_type value = Size::svalue(value2 - value1);
    c.regs.ccr.set_cc_cmp(value, value2, value1);

    ea1.finish(c);
    ea2.finish(c);
    c.regs.pc += 2 + ea1.extension_size() + ea2.extension_size();
  }

  /* Handles an EOR instruction (memory destination).  */
  template <class Size, class Destination> void
  m68k_eor_m(uint16_type op, context &c, unsigned long data)
  {
    Destination ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\teor%s %%d%u,%s\n", Size::suffix(), reg2, ea1.text(c).c_str());
#endif

    typename Size::svalue_type value1 = ea1.get(c);
    typename Size::svalue_type value2 = Size::get(c.regs.d[reg2]);
    typename Size::svalue_type value
      = Size::svalue(Size::uvalue(value1) ^ Size::uvalue(value2));
    ea1.put(c, value);
    c.regs.ccr.set_cc(value);

    ea1.finish(c);
    long_word_size::put(c.regs.pc,
			c.regs.pc + 2 + Destination::extension_size());
  }
}

void
vm68k::install_instructions_11(exec_unit &eu, unsigned long data)
{
  static const instruction_map inst[]
    = {{0xb000, 0xe07, &m68k_cmp<byte_size, byte_d_register>},
       {0xb010, 0xe07, &m68k_cmp<byte_size, byte_indirect>},
       {0xb018, 0xe07, &m68k_cmp<byte_size, byte_postinc_indirect>},
       {0xb020, 0xe07, &m68k_cmp<byte_size, byte_predec_indirect>},
       {0xb028, 0xe07, &m68k_cmp<byte_size, byte_disp_indirect>},
       {0xb030, 0xe07, &m68k_cmp<byte_size, byte_index_indirect>},
       {0xb038, 0xe00, &m68k_cmp<byte_size, byte_abs_short>},
       {0xb039, 0xe00, &m68k_cmp<byte_size, byte_abs_long>},
       {0xb03a, 0xe00, &m68k_cmp<byte_size, byte_disp_pc_indirect>},
       {0xb03b, 0xe00, &m68k_cmp<byte_size, byte_index_pc_indirect>},
       {0xb03c, 0xe00, &m68k_cmp<byte_size, byte_immediate>},
       {0xb040, 0xe07, &m68k_cmp<word_size, word_d_register>},
       {0xb048, 0xe07, &m68k_cmp<word_size, word_a_register>},
       {0xb050, 0xe07, &m68k_cmp<word_size, word_indirect>},
       {0xb058, 0xe07, &m68k_cmp<word_size, word_postinc_indirect>},
       {0xb060, 0xe07, &m68k_cmp<word_size, word_predec_indirect>},
       {0xb068, 0xe07, &m68k_cmp<word_size, word_disp_indirect>},
       {0xb070, 0xe07, &m68k_cmp<word_size, word_index_indirect>},
       {0xb078, 0xe00, &m68k_cmp<word_size, word_abs_short>},
       {0xb079, 0xe00, &m68k_cmp<word_size, word_abs_long>},
       {0xb07a, 0xe00, &m68k_cmp<word_size, word_disp_pc_indirect>},
       {0xb07b, 0xe00, &m68k_cmp<word_size, word_index_pc_indirect>},
       {0xb07c, 0xe00, &m68k_cmp<word_size, word_immediate>},
       {0xb080, 0xe07, &m68k_cmp<long_word_size, long_word_d_register>},
       {0xb088, 0xe07, &m68k_cmp<long_word_size, long_word_a_register>},
       {0xb090, 0xe07, &m68k_cmp<long_word_size, long_word_indirect>},
       {0xb098, 0xe07, &m68k_cmp<long_word_size, long_word_postinc_indirect>},
       {0xb0a0, 0xe07, &m68k_cmp<long_word_size, long_word_predec_indirect>},
       {0xb0a8, 0xe07, &m68k_cmp<long_word_size, long_word_disp_indirect>},
       {0xb0b0, 0xe07, &m68k_cmp<long_word_size, long_word_index_indirect>},
       {0xb0b8, 0xe00, &m68k_cmp<long_word_size, long_word_abs_short>},
       {0xb0b9, 0xe00, &m68k_cmp<long_word_size, long_word_abs_long>},
       {0xb0ba, 0xe00, &m68k_cmp<long_word_size, long_word_disp_pc_indirect>},
       {0xb0bb, 0xe00, &m68k_cmp<long_word_size, long_word_index_pc_indirect>},
       {0xb0bc, 0xe00, &m68k_cmp<long_word_size, long_word_immediate>},
       {0xb0c0, 0xe07, &m68k_cmpa<word_size, word_d_register>},
       {0xb0c8, 0xe07, &m68k_cmpa<word_size, word_a_register>},
       {0xb0d0, 0xe07, &m68k_cmpa<word_size, word_indirect>},
       {0xb0d8, 0xe07, &m68k_cmpa<word_size, word_postinc_indirect>},
       {0xb0e0, 0xe07, &m68k_cmpa<word_size, word_predec_indirect>},
       {0xb0e8, 0xe07, &m68k_cmpa<word_size, word_disp_indirect>},
       {0xb0f0, 0xe07, &m68k_cmpa<word_size, word_index_indirect>},  
       {0xb0f8, 0xe00, &m68k_cmpa<word_size, word_abs_short>},
       {0xb0f9, 0xe00, &m68k_cmpa<word_size, word_abs_long>},
       {0xb0fa, 0xe00, &m68k_cmpa<word_size, word_disp_pc_indirect>},
       {0xb0fb, 0xe00, &m68k_cmpa<word_size, word_index_pc_indirect>},
       {0xb0fc, 0xe00, &m68k_cmpa<word_size, word_immediate>},
       {0xb100, 0xe07, &m68k_eor_m<byte_size, byte_d_register>},
       {0xb108, 0xe07, &m68k_cmpm<byte_size>},
       {0xb110, 0xe07, &m68k_eor_m<byte_size, byte_indirect>},
       {0xb118, 0xe07, &m68k_eor_m<byte_size, byte_postinc_indirect>},
       {0xb120, 0xe07, &m68k_eor_m<byte_size, byte_predec_indirect>},
       {0xb128, 0xe07, &m68k_eor_m<byte_size, byte_disp_indirect>},
       {0xb130, 0xe07, &m68k_eor_m<byte_size, byte_index_indirect>},
       {0xb138, 0xe00, &m68k_eor_m<byte_size, byte_abs_short>},
       {0xb139, 0xe00, &m68k_eor_m<byte_size, byte_abs_long>},
       {0xb140, 0xe07, &m68k_eor_m<word_size, word_d_register>},
       {0xb148, 0xe07, &m68k_cmpm<word_size>},
       {0xb150, 0xe07, &m68k_eor_m<word_size, word_indirect>},
       {0xb158, 0xe07, &m68k_eor_m<word_size, word_postinc_indirect>},
       {0xb160, 0xe07, &m68k_eor_m<word_size, word_predec_indirect>},
       {0xb168, 0xe07, &m68k_eor_m<word_size, word_disp_indirect>},
       {0xb170, 0xe07, &m68k_eor_m<word_size, word_index_indirect>},
       {0xb178, 0xe00, &m68k_eor_m<word_size, word_abs_short>},
       {0xb179, 0xe00, &m68k_eor_m<word_size, word_abs_long>},
       {0xb180, 0xe07, &m68k_eor_m<long_word_size, long_word_d_register>},
       {0xb188, 0xe07, &m68k_cmpm<long_word_size>},
       {0xb190, 0xe07, &m68k_eor_m<long_word_size, long_word_indirect>},
       {0xb198, 0xe07, &m68k_eor_m<long_word_size, long_word_postinc_indirect>},
       {0xb1a0, 0xe07, &m68k_eor_m<long_word_size, long_word_predec_indirect>},
       {0xb1a8, 0xe07, &m68k_eor_m<long_word_size, long_word_disp_indirect>},
       {0xb1b0, 0xe07, &m68k_eor_m<long_word_size, long_word_index_indirect>},
       {0xb1b8, 0xe00, &m68k_eor_m<long_word_size, long_word_abs_short>},
       {0xb1b9, 0xe00, &m68k_eor_m<long_word_size, long_word_abs_long>},
       {0xb1c0, 0xe07, &m68k_cmpa<long_word_size, long_word_d_register>},
       {0xb1c8, 0xe07, &m68k_cmpa<long_word_size, long_word_a_register>},
       {0xb1d0, 0xe07, &m68k_cmpa<long_word_size, long_word_indirect>},
       {0xb1d8, 0xe07, &m68k_cmpa<long_word_size, long_word_postinc_indirect>},
       {0xb1e0, 0xe07, &m68k_cmpa<long_word_size, long_word_predec_indirect>},
       {0xb1e8, 0xe07, &m68k_cmpa<long_word_size, long_word_disp_indirect>},
       {0xb1f0, 0xe07, &m68k_cmpa<long_word_size, long_word_index_indirect>},
       {0xb1f8, 0xe00, &m68k_cmpa<long_word_size, long_word_abs_short>},
       {0xb1f9, 0xe00, &m68k_cmpa<long_word_size, long_word_abs_long>},
       {0xb1fa, 0xe00, &m68k_cmpa<long_word_size, long_word_disp_pc_indirect>},
       {0xb1fb, 0xe00, &m68k_cmpa<long_word_size, long_word_index_pc_indirect>},
       {0xb1fc, 0xe00, &m68k_cmpa<long_word_size, long_word_immediate>}};

  for (const instruction_map *i = inst + 0;
       i != inst + sizeof inst / sizeof inst[0]; ++i)
    eu.set_instruction(i->base, i->mask, make_pair(i->handler, data));
}
