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
#include <vm68k/cpu.h>

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

  /* Handles a DIVU instruction.  */
  template <class Source> void
  m68k_divu(uint_type op, context &c, unsigned long data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\tdivu%s %s,%%d%u\n", word_size::suffix(), ea1.text(c).c_str(), reg2);
#endif

    word_size::svalue_type value1 = ea1.get(c);
    long_word_size::svalue_type value2 = long_word_size::get(c.regs.d[reg2]);
    long_word_size::svalue_type value
      = long_word_size::uvalue(value2) / word_size::uvalue(value1);
    long_word_size::svalue_type rem
      = long_word_size::uvalue(value2) % word_size::uvalue(value1);
    long_word_size::put(c.regs.d[reg2],
			long_word_size::uvalue(rem) << 16
			| long_word_size::uvalue(value) & 0xffff);
    c.regs.ccr.set_cc(value);	// FIXME.

    ea1.finish(c);
    c.regs.pc += 2 + Source::extension_size();
  }

  /* Handles an OR instruction.  */
  template <class Size, class Source> void
  m68k_or(uint_type op, context &c, unsigned long data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\tor%s %s,%%d%u\n", Size::suffix(), ea1.text(c).c_str(), reg2);
#endif

    typename Size::svalue_type value1 = ea1.get(c);
    typename Size::svalue_type value2 = Size::get(c.regs.d[reg2]);
    typename Size::svalue_type value
      = Size::svalue(Size::uvalue(value2) | Size::uvalue(value1));
    Size::put(c.regs.d[reg2], value);
    c.regs.ccr.set_cc(value);

    ea1.finish(c);
    c.regs.advance_pc(2 + Source::extension_size());
  }

#if 0
  template <class Source> void
  orb(uint_type op, context &ec, unsigned long data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L(" orb %s", ea1.textb(ec));
    L(",%%d%u\n", reg2);
#endif

    sint_type value1 = ea1.getb(ec);
    sint_type value2 = extsb(ec.regs.d[reg2]);
    sint_type value = extsb(uint_type(value2) | uint_type(value1));
    ec.regs.d[reg2]
      = ec.regs.d[reg2] & ~0xff | uint32_type(value) & 0xff;
    ec.regs.ccr.set_cc(value);
    ea1.finishb(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <class Source> void
  orw(uint_type op, context &ec, unsigned long data)
  {
    Source ea1(op & 0x7, 2);
    int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L(" orw %s", ea1.textw(ec));
    L(",%%d%d\n", reg2);
#endif

    uint_type value1 = ea1.getw(ec);
    uint_type value2 = ec.regs.d[reg2];
    uint_type value = value2 | value1;
    ec.regs.d[reg2]
      = ec.regs.d[reg2] & ~0xffff | uint32_type(value) & 0xffff;
    ec.regs.ccr.set_cc(value);
    ea1.finishw(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <class Source> void
  orl(uint_type op, context &ec, unsigned long data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L(" orl %s", ea1.textl(ec));
    L(",%%d%u\n", reg2);
#endif

    sint32_type value1 = ea1.getl(ec);
    sint32_type value2 = extsl(ec.regs.d[reg2]);
    sint32_type value = extsl(uint32_type(value2) | uint32_type(value1));
    ec.regs.d[reg2] = value;
    ec.regs.ccr.set_cc(value);
    ea1.finishl(ec);

    ec.regs.pc += 2 + ea1.isize(4);
  }
#endif

  /* Handles an OR instruction (memory destination).  */
  template <class Size, class Destination> void
  m68k_or_m(uint_type op, context &c, unsigned long data)
  {
    Destination ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\tor%s %%d%u,%s\n", Size::suffix(), reg2, ea1.text(c).c_str());
#endif

    typename Size::svalue_type value2 = Size::get(c.regs.d[reg2]);
    typename Size::svalue_type value1 = ea1.get(c);
    typename Size::svalue_type value
      = Size::svalue(Size::uvalue(value1) | Size::uvalue(value2));
    ea1.put(c, value);
    c.regs.ccr.set_cc(value);

    ea1.finish(c);
    c.regs.pc += 2 + ea1.extension_size();
  }
}

void
vm68k::install_instructions_8(exec_unit &eu, unsigned long data)
{
  static const instruction_map inst[]
    = {{0x8000, 0xe07, &m68k_or<byte_size, byte_d_register>},
       {0x8010, 0xe07, &m68k_or<byte_size, byte_indirect>},
       {0x8018, 0xe07, &m68k_or<byte_size, byte_postinc_indirect>},
       {0x8020, 0xe07, &m68k_or<byte_size, byte_predec_indirect>},
       {0x8028, 0xe07, &m68k_or<byte_size, byte_disp_indirect>},
       {0x8030, 0xe07, &m68k_or<byte_size, byte_index_indirect>},
       {0x8038, 0xe00, &m68k_or<byte_size, byte_abs_short>},
       {0x8039, 0xe00, &m68k_or<byte_size, byte_abs_long>},
       {0x803a, 0xe00, &m68k_or<byte_size, byte_disp_pc_indirect>},
       {0x803b, 0xe00, &m68k_or<byte_size, byte_index_pc_indirect>},
       {0x803c, 0xe00, &m68k_or<byte_size, byte_immediate>},
       {0x8040, 0xe07, &m68k_or<word_size, word_d_register>},
       {0x8050, 0xe07, &m68k_or<word_size, word_indirect>},
       {0x8058, 0xe07, &m68k_or<word_size, word_postinc_indirect>},
       {0x8060, 0xe07, &m68k_or<word_size, word_predec_indirect>},
       {0x8068, 0xe07, &m68k_or<word_size, word_disp_indirect>},
       {0x8070, 0xe07, &m68k_or<word_size, word_index_indirect>},
       {0x8078, 0xe00, &m68k_or<word_size, word_abs_short>},
       {0x8079, 0xe00, &m68k_or<word_size, word_abs_long>},
       {0x807a, 0xe00, &m68k_or<word_size, word_disp_pc_indirect>},
       {0x807b, 0xe00, &m68k_or<word_size, word_index_pc_indirect>},
       {0x807c, 0xe00, &m68k_or<word_size, word_immediate>},
       {0x8080, 0xe07, &m68k_or<long_word_size, long_word_d_register>},
       {0x8090, 0xe07, &m68k_or<long_word_size, long_word_indirect>},
       {0x8098, 0xe07, &m68k_or<long_word_size, long_word_postinc_indirect>},
       {0x80a0, 0xe07, &m68k_or<long_word_size, long_word_predec_indirect>},
       {0x80a8, 0xe07, &m68k_or<long_word_size, long_word_disp_indirect>},
       {0x80b0, 0xe07, &m68k_or<long_word_size, long_word_index_indirect>},
       {0x80b8, 0xe00, &m68k_or<long_word_size, long_word_abs_short>},
       {0x80b9, 0xe00, &m68k_or<long_word_size, long_word_abs_long>},
       {0x80ba, 0xe00, &m68k_or<long_word_size, long_word_disp_pc_indirect>},
       {0x80bb, 0xe00, &m68k_or<long_word_size, long_word_index_pc_indirect>},
       {0x80bc, 0xe00, &m68k_or<long_word_size, long_word_immediate>},
       {0x80c0, 0xe07, &m68k_divu<word_d_register>},
       {0x80d0, 0xe07, &m68k_divu<word_indirect>},
       {0x80d8, 0xe07, &m68k_divu<word_postinc_indirect>},
       {0x80e0, 0xe07, &m68k_divu<word_predec_indirect>},
       {0x80e8, 0xe07, &m68k_divu<word_disp_indirect>},
       {0x80f0, 0xe07, &m68k_divu<word_index_indirect>},
       {0x80f8, 0xe00, &m68k_divu<word_abs_short>},
       {0x80f9, 0xe00, &m68k_divu<word_abs_long>},
       {0x80fa, 0xe00, &m68k_divu<word_disp_pc_indirect>},
       {0x80fb, 0xe00, &m68k_divu<word_index_pc_indirect>},
       {0x80fc, 0xe00, &m68k_divu<word_immediate>},
       {0x8110, 0xe07, &m68k_or_m<byte_size, byte_indirect>},
       {0x8118, 0xe07, &m68k_or_m<byte_size, byte_postinc_indirect>},
       {0x8120, 0xe07, &m68k_or_m<byte_size, byte_predec_indirect>},
       {0x8128, 0xe07, &m68k_or_m<byte_size, byte_disp_indirect>},
       {0x8130, 0xe07, &m68k_or_m<byte_size, byte_index_indirect>},
       {0x8138, 0xe00, &m68k_or_m<byte_size, byte_abs_short>},
       {0x8139, 0xe00, &m68k_or_m<byte_size, byte_abs_long>},
       {0x8150, 0xe07, &m68k_or_m<word_size, word_indirect>},
       {0x8158, 0xe07, &m68k_or_m<word_size, word_postinc_indirect>},
       {0x8160, 0xe07, &m68k_or_m<word_size, word_predec_indirect>},
       {0x8168, 0xe07, &m68k_or_m<word_size, word_disp_indirect>},
       {0x8170, 0xe07, &m68k_or_m<word_size, word_index_indirect>},
       {0x8178, 0xe00, &m68k_or_m<word_size, word_abs_short>},
       {0x8179, 0xe00, &m68k_or_m<word_size, word_abs_long>},
       {0x8190, 0xe07, &m68k_or_m<long_word_size, long_word_indirect>},
       {0x8198, 0xe07, &m68k_or_m<long_word_size, long_word_postinc_indirect>},
       {0x81a0, 0xe07, &m68k_or_m<long_word_size, long_word_predec_indirect>},
       {0x81a8, 0xe07, &m68k_or_m<long_word_size, long_word_disp_indirect>},
       {0x81b0, 0xe07, &m68k_or_m<long_word_size, long_word_index_indirect>},
       {0x81b8, 0xe00, &m68k_or_m<long_word_size, long_word_abs_short>},
       {0x81b9, 0xe00, &m68k_or_m<long_word_size, long_word_abs_long>}};

  for (const instruction_map *i = inst + 0;
       i != inst + sizeof inst / sizeof inst[0]; ++i)
    eu.set_instruction(i->base, i->mask, make_pair(i->handler, data));
}
