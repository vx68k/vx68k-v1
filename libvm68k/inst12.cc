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

  /* Handles an AND instruction.  */
  template <class Size, class Source> void
  m68k_and(uint16_type op, context &c, unsigned long data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\tand%s %s,%%d%u\n", Size::suffix(), ea1.text(c).c_str(), reg2);
#endif

    typename Size::svalue_type value1 = ea1.get(c);
    typename Size::svalue_type value2 = Size::get(c.regs.d[reg2]);
    typename Size::svalue_type value
      = Size::svalue(Size::uvalue(value2) & Size::uvalue(value1));
    Size::put(c.regs.d[reg2], value);
    c.regs.ccr.set_cc(value);
    ea1.finish(c);

    c.regs.pc += 2 + ea1.extension_size();
  }

  /* Handles an AND instruction (memory destination).  */
  template <class Size, class Destination> void
  m68k_and_m(uint16_type op, context &c, unsigned long data)
  {
    Destination ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\tand%s %%d%u,%s\n", Size::suffix(), reg2, ea1.text(c).c_str());
#endif

    typename Size::svalue_type value2 = Size::get(c.regs.d[reg2]);
    typename Size::svalue_type value1 = ea1.get(c);
    typename Size::svalue_type value
      = Size::svalue(Size::uvalue(value1) & Size::uvalue(value2));
    ea1.put(c, value);
    c.regs.ccr.set_cc(value);

    ea1.finish(c);
    c.regs.pc += 2 + ea1.extension_size();
  }

  /* Handles an EXG instruction (data registers).  */
  void
  m68k_exg_d_d(uint16_type op, context &c, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\texg%s %%d%u,%%d%u\n", long_word_size::suffix(), reg2, reg1);
#endif

    // The condition codes are not affected by this instruction.
    long_word_size::svalue_type value
      = long_word_size::get(c.regs.d[reg1]);
    long_word_size::put(c.regs.d[reg1], long_word_size::get(c.regs.d[reg2]));
    long_word_size::put(c.regs.d[reg2], value);

    c.regs.advance_pc(2);
  }

  /* Handles an EXG instruction (address registers).  */
  void
  m68k_exg_a_a(uint16_type op, context &c, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\texg%s %%a%u,%%a%u\n", long_word_size::suffix(), reg2, reg1);
#endif

    // The condition codes are not affected by this instruction.
    long_word_size::svalue_type value
      = long_word_size::get(c.regs.a[reg1]);
    long_word_size::put(c.regs.a[reg1], long_word_size::get(c.regs.a[reg2]));
    long_word_size::put(c.regs.a[reg2], value);

    c.regs.advance_pc(2);
  }

  /* Handles an EXG instruction (data register and address register).  */
  void
  m68k_exg_d_a(uint16_type op, context &c, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\texg%s %%d%u,%%a%u\n", long_word_size::suffix(), reg2, reg1);
#endif

    // The condition codes are not affected by this instruction.
    long_word_size::svalue_type value
      = long_word_size::get(c.regs.a[reg1]);
    long_word_size::put(c.regs.a[reg1], long_word_size::get(c.regs.d[reg2]));
    long_word_size::put(c.regs.d[reg2], value);

    c.regs.advance_pc(2);
  }

#if 0
  template <class Register2, class Register1> void
  exgl(uint16_type op, context &c, unsigned long data)
  {
    Register1 ea1(op & 0x7, 2);
    Register2 ea2(op >> 9 & 0x7, 2 + ea1.isize(4));
#ifdef HAVE_NANA_H
    L(" exgl %s", ea2.textl(c));
    L(",%s\n", ea1.textl(c));
#endif

    // The condition codes are not affected by this instruction.
    sint32_type value = ea1.getl(c);
    sint32_type xvalue = ea2.getl(c);
    ea1.putl(c, xvalue);
    ea2.putl(c, value);
    ea1.finishl(c);
    ea2.finishl(c);

    c.regs.pc += 2 + ea1.isize(4) + ea2.isize(4);
  }
#endif

  /* Handles a MULS instruction.  */
  template <class Source> void
  m68k_muls(uint16_type op, context &c, unsigned long data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\tmuls%s %s,%%d%u\n", word_size::suffix(), ea1.text(c).c_str(), reg2);
#endif

    word_size::svalue_type value1 = ea1.get(c);
    word_size::svalue_type value2 = word_size::get(c.regs.d[reg2]);
    long_word_size::svalue_type value
      = (long_word_size::svalue_type(value2)
	 * long_word_size::svalue_type(value1));
    long_word_size::put(c.regs.d[reg2], value);
    c.regs.ccr.set_cc(value);	// FIXME.

    ea1.finish(c);
    c.regs.advance_pc(2 + Source::extension_size());
  }

  /* Handles a MULU instruction.  */
  template <class Source> void
  m68k_mulu(uint16_type op, context &c, unsigned long data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\tmulu%s %s,%%d%u\n", word_size::suffix(), ea1.text(c).c_str(), reg2);
#endif

    word_size::svalue_type value1 = ea1.get(c);
    word_size::svalue_type value2 = word_size::get(c.regs.d[reg2]);
    long_word_size::svalue_type value
      = (long_word_size::svalue
	 (long_word_size::uvalue(word_size::uvalue(value2))
	  * long_word_size::uvalue(word_size::uvalue(value1))));
    long_word_size::put(c.regs.d[reg2], value);
    c.regs.ccr.set_cc(value); // FIXME.

    ea1.finish(c);
    c.regs.advance_pc(2 + Source::extension_size());
  }
}

void
vm68k::install_instructions_12(exec_unit &eu, unsigned long data)
{
  static const instruction_map inst[]
    = {{0xc000, 0xe07, &m68k_and<byte_size, byte_d_register>},
       {0xc010, 0xe07, &m68k_and<byte_size, byte_indirect>},
       {0xc018, 0xe07, &m68k_and<byte_size, byte_postinc_indirect>},
       {0xc020, 0xe07, &m68k_and<byte_size, byte_predec_indirect>},
       {0xc028, 0xe07, &m68k_and<byte_size, byte_disp_indirect>},
       {0xc030, 0xe07, &m68k_and<byte_size, byte_index_indirect>},
       {0xc038, 0xe00, &m68k_and<byte_size, byte_abs_short>},
       {0xc039, 0xe00, &m68k_and<byte_size, byte_abs_long>},
       {0xc03a, 0xe00, &m68k_and<byte_size, byte_disp_pc_indirect>},
       {0xc03b, 0xe00, &m68k_and<byte_size, byte_index_pc_indirect>},
       {0xc03c, 0xe00, &m68k_and<byte_size, byte_immediate>},
       {0xc040, 0xe07, &m68k_and<word_size, word_d_register>},
       {0xc050, 0xe07, &m68k_and<word_size, word_indirect>},
       {0xc058, 0xe07, &m68k_and<word_size, word_postinc_indirect>},
       {0xc060, 0xe07, &m68k_and<word_size, word_predec_indirect>},
       {0xc068, 0xe07, &m68k_and<word_size, word_disp_indirect>},
       {0xc070, 0xe07, &m68k_and<word_size, word_index_indirect>},
       {0xc078, 0xe00, &m68k_and<word_size, word_abs_short>},
       {0xc079, 0xe00, &m68k_and<word_size, word_abs_long>},
       {0xc07a, 0xe00, &m68k_and<word_size, word_disp_pc_indirect>},
       {0xc07b, 0xe00, &m68k_and<word_size, word_index_pc_indirect>},
       {0xc07c, 0xe00, &m68k_and<word_size, word_immediate>},
       {0xc080, 0xe07, &m68k_and<long_word_size, long_word_d_register>},
       {0xc090, 0xe07, &m68k_and<long_word_size, long_word_indirect>},
       {0xc098, 0xe07, &m68k_and<long_word_size, long_word_postinc_indirect>},
       {0xc0a0, 0xe07, &m68k_and<long_word_size, long_word_predec_indirect>},
       {0xc0a8, 0xe07, &m68k_and<long_word_size, long_word_disp_indirect>},
       {0xc0b0, 0xe07, &m68k_and<long_word_size, long_word_index_indirect>},
       {0xc0b8, 0xe00, &m68k_and<long_word_size, long_word_abs_short>},
       {0xc0b9, 0xe00, &m68k_and<long_word_size, long_word_abs_long>},
       {0xc0ba, 0xe00, &m68k_and<long_word_size, long_word_disp_pc_indirect>},
       {0xc0bb, 0xe00, &m68k_and<long_word_size, long_word_index_pc_indirect>},
       {0xc0bc, 0xe00, &m68k_and<long_word_size, long_word_immediate>},
       {0xc0c0, 0xe07, &m68k_mulu<word_d_register>},
       {0xc0d0, 0xe07, &m68k_mulu<word_indirect>},
       {0xc0d8, 0xe07, &m68k_mulu<word_postinc_indirect>},
       {0xc0e0, 0xe07, &m68k_mulu<word_predec_indirect>},
       {0xc0e8, 0xe07, &m68k_mulu<word_disp_indirect>},
       {0xc0f0, 0xe07, &m68k_mulu<word_index_indirect>},
       {0xc0f8, 0xe00, &m68k_mulu<word_abs_short>},
       {0xc0f9, 0xe00, &m68k_mulu<word_abs_long>},
       {0xc0fa, 0xe00, &m68k_mulu<word_disp_pc_indirect>},
       {0xc0fb, 0xe00, &m68k_mulu<word_index_pc_indirect>},
       {0xc0fc, 0xe00, &m68k_mulu<word_immediate>},
       {0xc110, 0xe07, &m68k_and_m<byte_size, byte_indirect>},
       {0xc118, 0xe07, &m68k_and_m<byte_size, byte_postinc_indirect>},
       {0xc120, 0xe07, &m68k_and_m<byte_size, byte_predec_indirect>},
       {0xc128, 0xe07, &m68k_and_m<byte_size, byte_disp_indirect>},
       {0xc130, 0xe07, &m68k_and_m<byte_size, byte_index_indirect>},
       {0xc138, 0xe00, &m68k_and_m<byte_size, byte_abs_short>},
       {0xc139, 0xe00, &m68k_and_m<byte_size, byte_abs_long>},
       {0xc140, 0xe07, &m68k_exg_d_d},
       {0xc148, 0xe07, &m68k_exg_a_a},
       {0xc150, 0xe07, &m68k_and_m<word_size, word_indirect>},
       {0xc158, 0xe07, &m68k_and_m<word_size, word_postinc_indirect>},
       {0xc160, 0xe07, &m68k_and_m<word_size, word_predec_indirect>},
       {0xc168, 0xe07, &m68k_and_m<word_size, word_disp_indirect>},
       {0xc170, 0xe07, &m68k_and_m<word_size, word_index_indirect>},
       {0xc178, 0xe00, &m68k_and_m<word_size, word_abs_short>},
       {0xc179, 0xe00, &m68k_and_m<word_size, word_abs_long>},
       {0xc188, 0xe07, &m68k_exg_d_a},
       {0xc190, 0xe07, &m68k_and_m<long_word_size, long_word_indirect>},
       {0xc198, 0xe07, &m68k_and_m<long_word_size, long_word_postinc_indirect>},
       {0xc1a0, 0xe07, &m68k_and_m<long_word_size, long_word_predec_indirect>},
       {0xc1a8, 0xe07, &m68k_and_m<long_word_size, long_word_disp_indirect>},
       {0xc1b0, 0xe07, &m68k_and_m<long_word_size, long_word_index_indirect>},
       {0xc1b8, 0xe00, &m68k_and_m<long_word_size, long_word_abs_short>},
       {0xc1b9, 0xe00, &m68k_and_m<long_word_size, long_word_abs_long>},
       {0xc1c0, 0xe07, &m68k_muls<word_d_register>},
       {0xc1d0, 0xe07, &m68k_muls<word_indirect>},
       {0xc1d8, 0xe07, &m68k_muls<word_postinc_indirect>},
       {0xc1e0, 0xe07, &m68k_muls<word_predec_indirect>},
       {0xc1e8, 0xe07, &m68k_muls<word_disp_indirect>},
       {0xc1f0, 0xe07, &m68k_muls<word_index_indirect>},
       {0xc1f8, 0xe00, &m68k_muls<word_abs_short>},
       {0xc1f9, 0xe00, &m68k_muls<word_abs_long>},
       {0xc1fa, 0xe00, &m68k_muls<word_disp_pc_indirect>},
       {0xc1fb, 0xe00, &m68k_muls<word_index_pc_indirect>},
       {0xc1fc, 0xe00, &m68k_muls<word_immediate>}};

  for (const instruction_map *i = inst + 0;
       i != inst + sizeof inst / sizeof inst[0]; ++i)
    eu.set_instruction(i->base, i->mask, make_pair(i->handler, data));
}
