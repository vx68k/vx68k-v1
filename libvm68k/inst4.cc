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
  using vm68k::privilege_violation_exception;
  using vm68k::memory;
  using vm68k::context;

  /* Handles a CLR instruction.  */
  template <class Size, class Destination> void
  m68k_clr(uint16_type op, context &c, unsigned long data)
  {
    Destination ea1(op & 0x7, 2);
#ifdef HAVE_NANA_H
    L("\tclr%s %s\n", Size::suffix(), ea1.text(c).c_str());
#endif

    ea1.put(c, 0);
    c.regs.ccr.set_cc(0);

    ea1.finish(c);
    long_word_size::put(c.regs.pc,
			c.regs.pc + 2 + Destination::extension_size());
  }

#if 0
  template <class Destination> void
  clrb(uint16_type op, context &ec, unsigned long data)
  {
    Destination ea1(op & 0x7, 2);
#ifdef HAVE_NANA_H
    VL((" clrb %s\n", ea1.textb(ec)));
#endif

    ea1.putb(ec, 0);
    ea1.finishb(ec);
    ec.regs.ccr.set_cc(0);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <class Destination> void
  clrw(uint16_type op, context &ec, unsigned long data)
  {
    Destination ea1(op & 0x7, 2);
#ifdef HAVE_NANA_H
    VL((" clrw %s\n", ea1.textw(ec)));
#endif

    ea1.putw(ec, 0);
    ea1.finishw(ec);
    ec.regs.ccr.set_cc(0);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <> void clrw<address_register>(unsigned int, context &);
  // XXX: Address register cannot be the destination.

  template <class Destination> void
  clrl(uint16_type op, context &ec, unsigned long data)
  {
    Destination ea1(op & 0x7, 2);
#ifdef HAVE_NANA_H
    VL((" clrl %s\n", ea1.textl(ec)));
#endif

    ea1.putl(ec, 0);
    ea1.finishl(ec);
    ec.regs.ccr.set_cc(0);

    ec.regs.pc += 2 + ea1.isize(2);
  }
#endif

  /* Handles an EXT instruction.  */
  template <class Size1, class Size2> void
  m68k_ext(uint16_type op, context &c, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
#ifdef HAVE_NANA_H
    L("\text%s %%d%u\n", Size2::suffix(), reg1);
#endif

    typename Size2::svalue_type value = Size1::get(c.regs.d[reg1]);
    Size2::put(c.regs.d[reg1], value);
    c.regs.ccr.set_cc(value);

    c.regs.advance_pc(2);
  }

  /* Handles a JMP instruction.  */
  template <class Destination> void
  m68k_jmp(uint16_type op, context &c, unsigned long data)
  {
    Destination ea1(op & 0x7, 2);
#ifdef HAVE_NANA_H
    L("\tjmp %s\n", ea1.text(c).c_str());
#endif

    // The condition codes are not affected by this instruction.
    uint32_type address = ea1.address(c);

    long_word_size::put(c.regs.pc, address);
  }

  /* Handles a JSR instruction.  */
  template <class Destination> void
  m68k_jsr(uint16_type op, context &c, unsigned long data)
  {
    Destination ea1(op & 0x7, 2);
#ifdef HAVE_NANA_H
    L("\tjsr %s\n", ea1.text(c).c_str());
#endif

    // XXX: The condition codes are not affected.
    uint32_type address = ea1.address(c);
    memory::function_code fc = c.data_fc();
    uint32_type sp = c.regs.a[7] - long_word_size::aligned_value_size();
    long_word_size::put(*c.mem, fc, sp,
			c.regs.pc + 2 + Destination::extension_size());
    long_word_size::put(c.regs.a[7], sp);

    long_word_size::put(c.regs.pc, address);
  }

  /* Handles a LEA instruction.  */
  template <class Destination> void
  m68k_lea(uint16_type op, context &c, unsigned long data)
  {
    Destination ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\tlea%s %s,%%a%u\n", long_word_size::suffix(), ea1.text(c).c_str(),
      reg2);
#endif

    // XXX: The condition codes are not affected.
    uint32_type address = ea1.address(c);
    long_word_size::put(c.regs.a[reg2], address);

    long_word_size::put(c.regs.pc,
			c.regs.pc + 2 + Destination::extension_size());
  }

  /* Handles a LINK instruction.  */
  void
  m68k_link(uint16_type op, context &c, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    word_size::svalue_type disp = c.fetch(word_size(), 2);
#ifdef HAVE_NANA_H
    L("\tlink %%a%u,#%#x\n", reg1, word_size::uvalue(disp));
#endif

    // XXX: The condition codes are not affected.
    memory::function_code fc = c.data_fc();
    uint32_type fp = c.regs.a[7] - long_word_size::aligned_value_size();
    long_word_size::put(*c.mem, fc, fp, c.regs.a[reg1]);
    long_word_size::put(c.regs.a[7], fp + disp);
    long_word_size::put(c.regs.a[reg1], fp);

    long_word_size::put(c.regs.pc,
			c.regs.pc + 2 + word_size::aligned_value_size());
  }

  /* Handles a MOVE-from-SR instruction.  */
  template <class Destination> void
  m68k_move_from_sr(uint16_type op, context &c, unsigned long data)
  {
    Destination ea1(op & 0x7, 2);
#ifdef HAVE_NANA_H
    L("\tmove%s %%sr,%s\n", word_size::suffix(), ea1.text(c).c_str());
#endif

    // This instruction is not privileged on MC68000.
    // This instruction does not affect the condition codes.
    word_size::uvalue_type value = c.sr();
    ea1.put(c, value);
    ea1.finish(c);

    c.regs.pc += 2 + ea1.extension_size();
  }

  /* Handles a MOVE-to-SR instruction.  */
  template <class Source> void
  m68k_move_to_sr(uint16_type op, context &c, unsigned long data)
  {
    Source ea1(op & 0x7, 2);
#ifdef HAVE_NANA_H
    L("\tmove%s %s,%%sr\n", word_size::suffix(), ea1.text(c).c_str());
#endif

    // This instruction is privileged.
    if (!c.supervisor_state())
      throw privilege_violation_exception();

    // This instruction sets the condition codes.
    word_size::uvalue_type value = ea1.get(c);
    c.set_sr(value);
    ea1.finish(c);

    c.regs.pc += 2 + ea1.extension_size();
  }

  /* Handles a MOVE-from-USP instruction.  */
  void
  m68k_move_from_usp(uint16_type op, context &c, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
#ifdef HAVE_NANA_H
    L("\tmove%s %%usp,%%a%u\n", long_word_size::suffix(), reg1);
#endif

    // This instruction is privileged.
    if (!c.supervisor_state())
      throw privilege_violation_exception();

    // The condition codes are not affected by this instruction.
    c.regs.a[reg1] = c.regs.usp;

    c.regs.pc += 2;
  }

  /* Handles a MOVE-to-USP instruction.  */
  void
  m68k_move_to_usp(uint16_type op, context &c, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
#ifdef HAVE_NANA_H
    L("\tmove%s ", long_word_size::suffix());
    L("%%a%u,", reg1);
    L("%%usp\n");
#endif

    // This instruction is privileged.
    if (!c.supervisor_state())
      throw privilege_violation_exception();

    // This instruction does not affect the condition codes.
    long_word_size::put(c.regs.usp, long_word_size::get(c.regs.a[reg1]));

    c.regs.pc += 2;
  }

  /* Handles a MOVEM instruction (register to memory) */
  template <class Size, class Destination> void
  m68k_movem_r_m(uint16_type op, context &c, unsigned long data)
  {
    word_size::uvalue_type mask = c.ufetch(word_size(), 2);
    Destination ea1(op & 0x7, 2 + 2);
#ifdef HAVE_NANA_H
    L("\tmovem%s #%#x,%s\n", Size::suffix(), mask, ea1.text(c).c_str());
#endif

    // This instruction does not affect the condition codes.
    uint16_type m = 1;
    memory::function_code fc = c.data_fc();
    uint32_type address = ea1.address(c);
    for (uint32_type *i = c.regs.d + 0; i != c.regs.d + 8; ++i)
      {
	if (mask & m)
	  {
	    Size::put(*c.mem, fc, address, Size::get(*i));
	    address += Size::value_size();
	  }
	m <<= 1;
      }
    for (uint32_type *i = c.regs.a + 0; i != c.regs.a + 8; ++i)
      {
	if (mask & m)
	  {
	    Size::put(*c.mem, fc, address, Size::get(*i));
	    address += Size::value_size();
	  }
	m <<= 1;
      }

    c.regs.pc += 2 + 2 + ea1.extension_size();
  }

  /* Handles a MOVEM instruction (register to predec memory).  */
  template <class Size> void
  m68k_movem_r_predec(uint16_type op, context &c, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    word_size::uvalue_type mask = c.ufetch(word_size(), 2);
#ifdef HAVE_NANA_H
    L("\tmovem%s #%#x,%%a%u@-\n", Size::suffix(), mask, reg1);
#endif

    // This instruction does not affect the condition codes.
    uint16_type m = 1;
    memory::function_code fc = c.data_fc();
    sint32_type address = long_word_size::get(c.regs.a[reg1]);
    // This instruction iterates registers in reverse.
    for (uint32_type *i = c.regs.a + 8; i != c.regs.a + 0; --i)
      {
	if (mask & m)
	  {
	    address -= Size::value_size();
	    Size::put(*c.mem, fc, address, long_word_size::get(*(i - 1)));
	  }
	m <<= 1;
      }
    for (uint32_type *i = c.regs.d + 8; i != c.regs.d + 0; --i)
      {
	if (mask & m)
	  {
	    address -= Size::value_size();
	    Size::put(*c.mem, fc, address, long_word_size::get(*(i - 1)));
	  }
	m <<= 1;
      }

    long_word_size::put(c.regs.a[reg1], address);
    c.regs.pc += 2 + 2;
  }

  /* Handles a MOVEM instruction (memory to register).  */
  template <class Size, class Source> void
  m68k_movem_m_r(uint16_type op, context &c, unsigned long data)
  {
    word_size::uvalue_type mask = c.ufetch(word_size(), 2);
    Source ea1(op & 0x7, 2 + word_size::aligned_value_size());
#ifdef HAVE_NANA_H
    L("\tmovem%s %s,#%#x\n", Size::suffix(), ea1.text(c).c_str(), mask);
#endif

    // XXX: The condition codes are not affected.
    uint16_type m = 1;
    memory::function_code fc = c.data_fc();
    uint32_type address = ea1.address(c);
    for (uint32_type *i = c.regs.d + 0; i != c.regs.d + 8; ++i)
      {
	if (mask & m)
	  {
	    long_word_size::put(*i, Size::get(*c.mem, fc, address));
	    address += Size::value_size();
	  }
	m <<= 1;
      }
    for (uint32_type *i = c.regs.a + 0; i != c.regs.a + 8; ++i)
      {
	if (mask & m)
	  {
	    long_word_size::put(*i, Size::get(*c.mem, fc, address));
	    address += Size::value_size();
	  }
	m <<= 1;
      }

    c.regs.advance_pc(2 + word_size::aligned_value_size()
		      + Source::extension_size());
  }

#if 0
  /* moveml instruction (memory to register) */
  template <class Source> void
  moveml_mr(uint16_type op, context &ec, unsigned long data)
  {
    Source ea1(op & 0x7, 4);
    unsigned int bitmap = ec.fetch(word_size(), 2);
#ifdef HAVE_NANA_H
    L(" moveml %s", ea1.textl(ec));
    L(",#0x%04x\n", bitmap);
#endif

    // XXX: The condition codes are not affected.
    uint32_type address = ea1.address(ec);
    memory::function_code fc = ec.data_fc();
    for (uint32_type *i = ec.regs.d + 0; i != ec.regs.d + 8; ++i)
      {
	if ((bitmap & 1) != 0)
	  {
	    *i = ec.mem->get_32(fc, address);
	    address += 4;
	  }
	bitmap >>= 1;
      }
    for (uint32_type *i = ec.regs.a + 0; i != ec.regs.a + 8; ++i)
      {
	if ((bitmap & 1) != 0)
	  {
	    *i = ec.mem->get_32(fc, address);
	    address += 4;
	  }
	bitmap >>= 1;
      }

    ec.regs.pc += 4 + ea1.isize(4);
  }
#endif

  /* Handles a MOVEM instruction (postinc memory to register).  */
  template <class Size> void
  m68k_movem_postinc_r(uint16_type op, context &c, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    word_size::uvalue_type mask = c.ufetch(word_size(), 2);
#ifdef HAVE_NANA_H
    L("\tmovem%s %%a%u@+,#%#x\n", Size::suffix(), reg1, mask);
#endif

    // This instruction does not affect the condition codes.
    uint16_type m = 1;
    memory::function_code fc = c.data_fc();
    sint32_type address = long_word_size::get(c.regs.a[reg1]);
    // This instruction sign-extends words to long words.
    for (uint32_type *i = c.regs.d + 0; i != c.regs.d + 8; ++i)
      {
	if (mask & m)
	  {
	    long_word_size::put(*i, Size::get(*c.mem, fc, address));
	    address += Size::value_size();
	  }
	m <<= 1;
      }
    for (uint32_type *i = c.regs.a + 0; i != c.regs.a + 8; ++i)
      {
	if (mask & m)
	  {
	    long_word_size::put(*i, Size::get(*c.mem, fc, address));
	    address += Size::value_size();
	  }
	m <<= 1;
      }

    c.regs.a[reg1] = address;
    c.regs.pc += 2 + 2;
  }

  /* Handles a NEG instruction.  */
  template <class Size, class Destination> void
  m68k_neg(uint16_type op, context &c, unsigned long data)
  {
    Destination ea1(op & 0x7, 2);
#ifdef HAVE_NANA_H
    L("\tneg%s %s\n", Size::suffix(), ea1.text(c).c_str());
#endif

    typename Size::svalue_type value1 = ea1.get(c);
    typename Size::svalue_type value = Size::svalue(-value1);
    ea1.put(c, value);
    c.regs.ccr.set_cc_sub(value, 0, value1);

    ea1.finish(c);
    c.regs.advance_pc(2 + Destination::extension_size());
  }

  /* Handles a NOP instruction.  */
  void
  m68k_nop(uint16_type op, context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    L("\tnop\n");
#endif

    c.regs.pc += 2;
  }

  /* Handles a NOT instruction.  */
  template <class Size, class Destination> void
  m68k_not(uint16_type op, context &c, unsigned long data)
  {
    Destination ea1(op & 0x7, 2);
#ifdef HAVE_NANA_H
    L("\tnot%s %s\n", Size::suffix(), ea1.text(c).c_str());
#endif

    typename Size::svalue_type value1 = ea1.get(c);
    typename Size::svalue_type value = Size::svalue(~Size::uvalue(value1));
    ea1.put(c, value);
    c.regs.ccr.set_cc(value);

    ea1.finish(c);
    c.regs.pc += 2 + ea1.extension_size();
  }

  /* Handles a PEA instruction.  */
  template <class Destination> void
  m68k_pea(uint16_type op, context &c, unsigned long data)
  {
    Destination ea1(op & 0x7, 2);
#ifdef HAVE_NANA_H
    L("\tpea%s %s\n", long_word_size::suffix(), ea1.text(c).c_str());
#endif

    // XXX: The condition codes are not affected.
    uint32_type address = ea1.address(c);
    memory::function_code fc = c.data_fc();
    uint32_type sp = c.regs.a[7] - long_word_size::aligned_value_size();
    long_word_size::put(*c.mem, fc, sp, address);
    long_word_size::put(c.regs.a[7], sp);

    c.regs.advance_pc(2 + Destination::extension_size());
  }

  /* Handles a RTE instruction.  */
  void
  m68k_rte(uint16_type op, context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    L("\trte\n");
#endif

    // This instruction is privileged.
    if (!c.supervisor_state())
      throw privilege_violation_exception();

    uint16_type status = word_size::uget(*c.mem, memory::SUPER_DATA,
					 c.regs.a[7] + 0);
    uint32_type value = long_word_size::uget(*c.mem, memory::SUPER_DATA,
					     c.regs.a[7] + 2);
    c.regs.a[7] += 6;
    c.set_sr(status);
    c.regs.pc = value;
  }

  void
  m68k_rts(uint16_type op, context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    L("\trts\n");
#endif

    // XXX: The condition codes are not affected.
    memory::function_code fc = c.data_fc();
    sint32_type value = long_word_size::get(*c.mem, fc, c.regs.a[7]);
    long_word_size::put(c.regs.a[7], c.regs.a[7] + 4);

    long_word_size::put(c.regs.pc, value);
  }

  /* Handles a SWAP instruction.  */
  void
  m68k_swap(uint16_type op, context &c, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
#ifdef HAVE_NANA_H
    L("\tswap%s %%d%u\n", word_size::suffix(), reg1);
#endif

    long_word_size::svalue_type value1 = long_word_size::get(c.regs.d[reg1]);
    long_word_size::svalue_type value
      = long_word_size::svalue(long_word_size::uvalue(value1) << 16
			       | (long_word_size::uvalue(value1) >> 16
				  & 0xffff));
    long_word_size::put(c.regs.d[reg1], value);
    c.regs.ccr.set_cc(value);

    long_word_size::put(c.regs.pc, c.regs.pc + 2);
  }

  /* Handles a TST instruction.  */
  template <class Size, class Destination> void
  m68k_tst(uint16_type op, context &c, unsigned long data)
  {
    Destination ea1(op & 0x7, 2);
#ifdef HAVE_NANA_H
    L("\ttst%s %s\n", Size::suffix(), ea1.text(c).c_str());
#endif

    typename Size::svalue_type value = ea1.get(c);
    c.regs.ccr.set_cc(value);

    ea1.finish(c);
    long_word_size::put(c.regs.pc,
			c.regs.pc + 2 + Destination::extension_size());
  }

#if 0
  template <class Destination> void
  tstb(uint16_type op, context &ec, unsigned long data)
  {
    Destination ea1(op & 0x7, 2);
#ifdef HAVE_NANA_H
    VL((" tstb %s\n", ea1.textb(ec)));
#endif

    int value = ea1.getb(ec);
    ec.regs.ccr.set_cc(value);
    ea1.finishb(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <class Destination> void
  tstw(uint16_type op, context &ec, unsigned long data)
  {
    Destination ea1(op & 0x7, 2);
#ifdef HAVE_NANA_H
    L(" tstw %s\n", ea1.textw(ec));
#endif

    sint_type value = ea1.getw(ec);
    ec.regs.ccr.set_cc(value);
    ea1.finishw(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <class Destination> void
  tstl(uint16_type op, context &ec, unsigned long data)
  {
    Destination ea1(op & 0x7, 2);
#ifdef HAVE_NANA_H
    VL((" tstl %s\n", ea1.textl(ec)));
#endif

    sint32_type value = ea1.getl(ec);
    ec.regs.ccr.set_cc(value);
    ea1.finishl(ec);

    ec.regs.pc += 2 + ea1.isize(4);
  }
#endif

  void
  m68k_unlk(uint16_type op, context &c, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
#ifdef HAVE_NANA_H
    L("\tunlk %%a%u\n", reg1);
#endif

    // XXX: The condition codes are not affected.
    memory::function_code fc = c.data_fc();
    uint32_type fp = c.regs.a[reg1];
    long_word_size::put(c.regs.a[reg1], long_word_size::get(*c.mem, fc, fp));
    long_word_size::put(c.regs.a[7],
			fp + long_word_size::aligned_value_size());

    long_word_size::put(c.regs.pc, c.regs.pc + 2);
  }
}

void
vm68k::install_instructions_4(exec_unit &eu, unsigned long data)
{
  static const instruction_map inst[]
    = {{0x40c0,     7, &m68k_move_from_sr<word_d_register>},
       {0x40d0,     7, &m68k_move_from_sr<word_indirect>},
       {0x40d8,     7, &m68k_move_from_sr<word_postinc_indirect>},
       {0x40e0,     7, &m68k_move_from_sr<word_predec_indirect>},
       {0x40e8,     7, &m68k_move_from_sr<word_disp_indirect>},
       {0x40f0,     7, &m68k_move_from_sr<word_index_indirect>},
       {0x40f8,     0, &m68k_move_from_sr<word_abs_short>},
       {0x40f9,     0, &m68k_move_from_sr<word_abs_long>},
       {0x41d0, 0xe07, &m68k_lea<word_indirect>},
       {0x41e8, 0xe07, &m68k_lea<word_disp_indirect>},
       {0x41f0, 0xe07, &m68k_lea<word_index_indirect>},
       {0x41f8, 0xe00, &m68k_lea<word_abs_short>},
       {0x41f9, 0xe00, &m68k_lea<word_abs_long>},
       {0x41fa, 0xe00, &m68k_lea<word_disp_pc_indirect>},
       {0x41fb, 0xe00, &m68k_lea<word_index_pc_indirect>},
       {0x4200,     7, &m68k_clr<byte_size, byte_d_register>},
       {0x4210,     7, &m68k_clr<byte_size, byte_indirect>},
       {0x4218,     7, &m68k_clr<byte_size, byte_postinc_indirect>},
       {0x4220,     7, &m68k_clr<byte_size, byte_predec_indirect>},
       {0x4228,     7, &m68k_clr<byte_size, byte_disp_indirect>},
       {0x4230,     7, &m68k_clr<byte_size, byte_index_indirect>},
       {0x4238,     0, &m68k_clr<byte_size, byte_abs_short>},
       {0x4239,     0, &m68k_clr<byte_size, byte_abs_long>},
       {0x4240,     7, &m68k_clr<word_size, word_d_register>},
       {0x4250,     7, &m68k_clr<word_size, word_indirect>},
       {0x4258,     7, &m68k_clr<word_size, word_postinc_indirect>},
       {0x4260,     7, &m68k_clr<word_size, word_predec_indirect>},
       {0x4268,     7, &m68k_clr<word_size, word_disp_indirect>},
       {0x4270,     7, &m68k_clr<word_size, word_index_indirect>},
       {0x4278,     0, &m68k_clr<word_size, word_abs_short>},
       {0x4279,     0, &m68k_clr<word_size, word_abs_long>},
       {0x4280,     7, &m68k_clr<long_word_size, long_word_d_register>},
       {0x4290,     7, &m68k_clr<long_word_size, long_word_indirect>},
       {0x4298,     7, &m68k_clr<long_word_size, long_word_postinc_indirect>},
       {0x42a0,     7, &m68k_clr<long_word_size, long_word_predec_indirect>},
       {0x42a8,     7, &m68k_clr<long_word_size, long_word_disp_indirect>},
       {0x42b0,     7, &m68k_clr<long_word_size, long_word_index_indirect>},
       {0x42b8,     0, &m68k_clr<long_word_size, long_word_abs_short>},
       {0x42b9,     0, &m68k_clr<long_word_size, long_word_abs_long>},
       {0x4400,     7, &m68k_neg<byte_size, byte_d_register>},
       {0x4410,     7, &m68k_neg<byte_size, byte_indirect>},
       {0x4418,     7, &m68k_neg<byte_size, byte_postinc_indirect>},
       {0x4420,     7, &m68k_neg<byte_size, byte_predec_indirect>},
       {0x4428,     7, &m68k_neg<byte_size, byte_disp_indirect>},
       {0x4430,     7, &m68k_neg<byte_size, byte_index_indirect>},
       {0x4438,     0, &m68k_neg<byte_size, byte_abs_short>},
       {0x4439,     0, &m68k_neg<byte_size, byte_abs_long>},
       {0x4440,     7, &m68k_neg<word_size, word_d_register>},
       {0x4450,     7, &m68k_neg<word_size, word_indirect>},
       {0x4458,     7, &m68k_neg<word_size, word_postinc_indirect>},
       {0x4460,     7, &m68k_neg<word_size, word_predec_indirect>},
       {0x4468,     7, &m68k_neg<word_size, word_disp_indirect>},
       {0x4470,     7, &m68k_neg<word_size, word_index_indirect>},
       {0x4478,     0, &m68k_neg<word_size, word_abs_short>},
       {0x4479,     0, &m68k_neg<word_size, word_abs_long>},
       {0x4480,     7, &m68k_neg<long_word_size, long_word_d_register>},
       {0x4490,     7, &m68k_neg<long_word_size, long_word_indirect>},
       {0x4498,     7, &m68k_neg<long_word_size, long_word_postinc_indirect>},
       {0x44a0,     7, &m68k_neg<long_word_size, long_word_predec_indirect>},
       {0x44a8,     7, &m68k_neg<long_word_size, long_word_disp_indirect>},
       {0x44b0,     7, &m68k_neg<long_word_size, long_word_index_indirect>},
       {0x44b8,     0, &m68k_neg<long_word_size, long_word_abs_short>},
       {0x44b9,     0, &m68k_neg<long_word_size, long_word_abs_long>},
       {0x4600,     7, &m68k_not<byte_size, byte_d_register>},
       {0x4610,     7, &m68k_not<byte_size, byte_indirect>},
       {0x4618,     7, &m68k_not<byte_size, byte_postinc_indirect>},
       {0x4620,     7, &m68k_not<byte_size, byte_predec_indirect>},
       {0x4628,     7, &m68k_not<byte_size, byte_disp_indirect>},
       {0x4630,     7, &m68k_not<byte_size, byte_index_indirect>},
       {0x4638,     0, &m68k_not<byte_size, byte_abs_short>},
       {0x4639,     0, &m68k_not<byte_size, byte_abs_long>},
       {0x4640,     7, &m68k_not<word_size, word_d_register>},
       {0x4650,     7, &m68k_not<word_size, word_indirect>},
       {0x4658,     7, &m68k_not<word_size, word_postinc_indirect>},
       {0x4660,     7, &m68k_not<word_size, word_predec_indirect>},
       {0x4668,     7, &m68k_not<word_size, word_disp_indirect>},
       {0x4670,     7, &m68k_not<word_size, word_index_indirect>},
       {0x4678,     0, &m68k_not<word_size, word_abs_short>},
       {0x4679,     0, &m68k_not<word_size, word_abs_long>},
       {0x4680,     7, &m68k_not<long_word_size, long_word_d_register>},
       {0x4690,     7, &m68k_not<long_word_size, long_word_indirect>},
       {0x4698,     7, &m68k_not<long_word_size, long_word_postinc_indirect>},
       {0x46a0,     7, &m68k_not<long_word_size, long_word_predec_indirect>},
       {0x46a8,     7, &m68k_not<long_word_size, long_word_disp_indirect>},
       {0x46b0,     7, &m68k_not<long_word_size, long_word_index_indirect>},
       {0x46b8,     0, &m68k_not<long_word_size, long_word_abs_short>},
       {0x46b9,     0, &m68k_not<long_word_size, long_word_abs_long>},
       {0x46c0,     7, &m68k_move_to_sr<word_d_register>},
       {0x46d0,     7, &m68k_move_to_sr<word_indirect>},
       {0x46d8,     7, &m68k_move_to_sr<word_postinc_indirect>},
       {0x46e0,     7, &m68k_move_to_sr<word_predec_indirect>},
       {0x46e8,     7, &m68k_move_to_sr<word_disp_indirect>},
       {0x46f0,     7, &m68k_move_to_sr<word_index_indirect>},
       {0x46f8,     0, &m68k_move_to_sr<word_abs_short>},
       {0x46f9,     0, &m68k_move_to_sr<word_abs_long>},
       {0x46fa,     0, &m68k_move_to_sr<word_disp_pc_indirect>},
       {0x46fb,     0, &m68k_move_to_sr<word_index_pc_indirect>},
       {0x46fc,     0, &m68k_move_to_sr<word_immediate>},
       {0x4840,     7, &m68k_swap},
       {0x4850,     7, &m68k_pea<word_indirect>},
       {0x4868,     7, &m68k_pea<word_disp_indirect>},
       {0x4870,     7, &m68k_pea<word_index_indirect>},
       {0x4878,     0, &m68k_pea<word_abs_short>},
       {0x4879,     0, &m68k_pea<word_abs_long>},
       {0x487a,     0, &m68k_pea<word_disp_pc_indirect>},
       {0x487b,     0, &m68k_pea<word_index_pc_indirect>},
       {0x4880,     7, &m68k_ext<byte_size, word_size>},
       {0x4890,     7, &m68k_movem_r_m<word_size, word_indirect>},
       {0x48a0,     7, &m68k_movem_r_predec<word_size>},
       {0x48a8,     7, &m68k_movem_r_m<word_size, word_disp_indirect>},
       {0x48b0,     7, &m68k_movem_r_m<word_size, word_index_indirect>},
       {0x48b8,     0, &m68k_movem_r_m<word_size, word_abs_short>},
       {0x48b9,     0, &m68k_movem_r_m<word_size, word_abs_long>},
       {0x48c0,     7, &m68k_ext<word_size, long_word_size>},
       {0x48d0,     7, &m68k_movem_r_m<long_word_size, long_word_indirect>},
       {0x48e0,     7, &m68k_movem_r_predec<long_word_size>},
       {0x48e8,     7, &m68k_movem_r_m<long_word_size, long_word_disp_indirect>},
       {0x48f0,     7, &m68k_movem_r_m<long_word_size, long_word_index_indirect>},
       {0x48f8,     0, &m68k_movem_r_m<long_word_size, long_word_abs_short>},
       {0x48f9,     0, &m68k_movem_r_m<long_word_size, long_word_abs_long>},
       {0x4a00,     7, &m68k_tst<byte_size, byte_d_register>},
       {0x4a10,     7, &m68k_tst<byte_size, byte_indirect>},
       {0x4a18,     7, &m68k_tst<byte_size, byte_postinc_indirect>},
       {0x4a20,     7, &m68k_tst<byte_size, byte_predec_indirect>},
       {0x4a28,     7, &m68k_tst<byte_size, byte_disp_indirect>},
       {0x4a30,     7, &m68k_tst<byte_size, byte_index_indirect>},
       {0x4a38,     0, &m68k_tst<byte_size, byte_abs_short>},
       {0x4a39,     0, &m68k_tst<byte_size, byte_abs_long>},
       {0x4a40,     7, &m68k_tst<word_size, word_d_register>},
       {0x4a50,     7, &m68k_tst<word_size, word_indirect>},
       {0x4a58,     7, &m68k_tst<word_size, word_postinc_indirect>},
       {0x4a60,     7, &m68k_tst<word_size, word_predec_indirect>},
       {0x4a68,     7, &m68k_tst<word_size, word_disp_indirect>},
       {0x4a70,     7, &m68k_tst<word_size, word_index_indirect>},
       {0x4a78,     0, &m68k_tst<word_size, word_abs_short>},
       {0x4a79,     0, &m68k_tst<word_size, word_abs_long>},
       {0x4a80,     7, &m68k_tst<long_word_size, long_word_d_register>},
       {0x4a90,     7, &m68k_tst<long_word_size, long_word_indirect>},
       {0x4a98,     7, &m68k_tst<long_word_size, long_word_postinc_indirect>},
       {0x4aa0,     7, &m68k_tst<long_word_size, long_word_predec_indirect>},
       {0x4aa8,     7, &m68k_tst<long_word_size, long_word_disp_indirect>},
       {0x4ab0,     7, &m68k_tst<long_word_size, long_word_index_indirect>},
       {0x4ab8,     0, &m68k_tst<long_word_size, long_word_abs_short>},
       {0x4ab9,     0, &m68k_tst<long_word_size, long_word_abs_long>},
       {0x4c90,     7, &m68k_movem_m_r<word_size, word_indirect>},
       {0x4c98,     7, &m68k_movem_postinc_r<word_size>},
       {0x4ca8,     7, &m68k_movem_m_r<word_size, word_disp_indirect>},
       {0x4cb0,     7, &m68k_movem_m_r<word_size, word_index_indirect>},
       {0x4cb8,     0, &m68k_movem_m_r<word_size, word_abs_short>},
       {0x4cb9,     0, &m68k_movem_m_r<word_size, word_abs_long>},
       {0x4cba,     0, &m68k_movem_m_r<word_size, word_disp_pc_indirect>},
       {0x4cbb,     0, &m68k_movem_m_r<word_size, word_index_pc_indirect>},
       {0x4cd0,     7, &m68k_movem_m_r<long_word_size, long_word_indirect>},
       {0x4cd8,     7, &m68k_movem_postinc_r<long_word_size>},
       {0x4ce8,     7, &m68k_movem_m_r<long_word_size, long_word_disp_indirect>},
       {0x4cf0,     7, &m68k_movem_m_r<long_word_size, long_word_index_indirect>},
       {0x4cf8,     0, &m68k_movem_m_r<long_word_size, long_word_abs_short>},
       {0x4cf9,     0, &m68k_movem_m_r<long_word_size, long_word_abs_long>},
       {0x4cfa,     0, &m68k_movem_m_r<long_word_size, long_word_disp_pc_indirect>},
       {0x4cfb,     0, &m68k_movem_m_r<long_word_size, long_word_index_pc_indirect>},
       {0x4e50,     7, &m68k_link},
       {0x4e58,     7, &m68k_unlk},
       {0x4e60,     7, &m68k_move_to_usp},
       {0x4e68,     7, &m68k_move_from_usp},
       {0x4e71,     0, &m68k_nop},
       {0x4e73,     0, &m68k_rte},
       {0x4e75,     0, &m68k_rts},
       {0x4e90,     7, &m68k_jsr<word_indirect>},
       {0x4ea8,     7, &m68k_jsr<word_disp_indirect>},
       {0x4eb0,     7, &m68k_jsr<word_index_indirect>},
       {0x4eb8,     0, &m68k_jsr<word_abs_short>},
       {0x4eb9,     0, &m68k_jsr<word_abs_long>},
       {0x4eba,     0, &m68k_jsr<word_disp_pc_indirect>},
       {0x4ebb,     0, &m68k_jsr<word_index_pc_indirect>},
       {0x4ed0,     7, &m68k_jmp<word_indirect>},
       {0x4ee8,     7, &m68k_jmp<word_disp_indirect>},
       {0x4ef0,     7, &m68k_jmp<word_index_indirect>},
       {0x4ef8,     0, &m68k_jmp<word_abs_short>},
       {0x4ef9,     0, &m68k_jmp<word_abs_long>},
       {0x4efa,     0, &m68k_jmp<word_disp_pc_indirect>},
       {0x4efb,     0, &m68k_jmp<word_index_pc_indirect>}};

  for (const instruction_map *i = inst + 0;
       i != inst + sizeof inst / sizeof inst[0]; ++i)
    eu.set_instruction(i->base, i->mask, make_pair(i->handler, data));
}
