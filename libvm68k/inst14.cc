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

  /* Handles an ASL instruction with a register count.  */
  template <class Size> void
  m68k_asl_r(uint16_type op, context &c, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\tasl%s ", Size::suffix());
    L("%%d%u,", reg2);
    L("%%d%u\n", reg1);
#endif

    unsigned int value2 = c.regs.d[reg2] % Size::value_bit();
    typename Size::svalue_type value1 = Size::get(c.regs.d[reg1]);
    typename Size::svalue_type value = Size::svalue(value1 << value2);
    Size::put(c.regs.d[reg1], value);
    c.regs.ccr.set_cc_lsl(value, value1, value2 + (32 - Size::value_bit())); // FIXME?

    c.regs.pc += 2;
  }

  /* Handles an ASL instruction with an immediate count.  */
  template <class Size> void
  m68k_asl_i(uint16_type op, context &c, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int value2 = op >> 9 & 0x7;
    if (value2 == 0)
      value2 = 8;
#ifdef HAVE_NANA_H
    L("\tasl%s #%u,%%d%u\n", Size::suffix(), value2, reg1);
#endif

    typename Size::svalue_type value1 = Size::get(c.regs.d[reg1]);
    typename Size::svalue_type value = Size::svalue(value1 << value2);
    Size::put(c.regs.d[reg1], value);
    c.regs.ccr.set_cc_lsl(value, value1, value2 + (32 - Size::value_bit())); // FIXME?

    c.regs.pc += 2;
  }

  /* Handles an ASR instruction (register).  */
  template <class Size> void
  m68k_asr_r(uint16_type op, context &c, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\tasr%s %%d%u,%%d%u\n", Size::suffix(), reg2, reg1);
#endif

    unsigned int value2 = c.regs.d[reg2] % Size::value_bit();
    typename Size::svalue_type value1 = Size::get(c.regs.d[reg1]);
    typename Size::svalue_type value = Size::svalue(value1 >> value2);
    Size::put(c.regs.d[reg1], value);
    c.regs.ccr.set_cc_asr(value, value1, value2);

    c.regs.pc += 2;
  }

#if 0
  void
  asrl_r(uint16_type op, context &ec, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L(" asrl %%d%u", reg2);
    L(",%%d%u\n", reg1);
#endif

    uint16_type count = ec.regs.d[reg2];
    sint32_type value1 = extsl(ec.regs.d[reg1]);
    sint32_type value = value1 >> count;
    ec.regs.d[reg1] = value;
    ec.regs.ccr.set_cc_asr(value, value1, count);

    ec.regs.pc += 2;
  }
#endif

  /* Handles an ASR instruction with an immediate count.  */
  template <class Size> void
  m68k_asr_i(uint16_type op, context &c, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int value2 = op >> 9 & 0x7;
    if (value2 == 0)
      value2 = 8;
#ifdef HAVE_NANA_H
    L("\tasr%s ", Size::suffix());
    L("#%u,", value2);
    L("%%d%u\n", reg1);
#endif

    typename Size::svalue_type value1 = Size::get(c.regs.d[reg1]);
    typename Size::svalue_type value = Size::svalue(value1 >> value2);
    Size::put(c.regs.d[reg1], value);
    c.regs.ccr.set_cc_asr(value, value1, value2);

    c.regs.pc += 2;
  }

  /* Handles a LSL instruction (register).  */
  template <class Size> void
  m68k_lsl_r(uint16_type op, context &c, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\tlsl%s %%d%u,%%d%u\n", Size::suffix(), reg2, reg1);
#endif

    unsigned int value2 = c.regs.d[reg2] % Size::value_bit();
    typename Size::svalue_type value1 = Size::get(c.regs.d[reg1]);
    typename Size::svalue_type value
      = Size::svalue(Size::uvalue(value1) << value2);
    Size::put(c.regs.d[reg1], value);
    c.regs.ccr.set_cc_lsl(value, value1, value2 + (32 - Size::value_bit())); // FIXME?

    c.regs.advance_pc(2);
  }

#if 0
  void
  lslw_r(uint16_type op, context &ec, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L(" lslw %%d%u", reg2);
    L(",%%d%u\n", reg1);
#endif

    uint16_type count = ec.regs.d[reg2];
    sint_type value1 = extsw(ec.regs.d[reg1]);
    sint_type value = extsw(uint16_type(value1) << count);
    const uint32_type MASK = ((uint32_type) 1u << 16) - 1;
    ec.regs.d[reg1] = ec.regs.d[reg1] & ~MASK | uint32_type(value) & MASK;
    ec.regs.ccr.set_cc_lsl(value, value1, count + (32 - 16));

    ec.regs.pc += 2;
  }

  void
  lsll_r(uint16_type op, context &ec, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L(" lsll %%d%u", reg2);
    L(",%%d%u\n", reg1);
#endif

    uint16_type count = ec.regs.d[reg2];
    sint32_type value1 = extsl(ec.regs.d[reg1]);
    sint32_type value = extsl(uint16_type(value1) << count);
    ec.regs.d[reg1] = value;
    ec.regs.ccr.set_cc_lsl(value, value1, count);

    ec.regs.pc += 2;
  }
#endif

  /* Handles a LSL instruction (immediate).  */
  template <class Size> void
  m68k_lsl_i(uint16_type op, context &c, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int value2 = op >> 9 & 0x7;
    if (value2 == 0)
      value2 = 8;
#ifdef HAVE_NANA_H
    L("\tlsl%s #%u,%%d%u\n", Size::suffix(), value2, reg1);
#endif

    typename Size::svalue_type value1 = Size::get(c.regs.d[reg1]);
    typename Size::svalue_type value
      = Size::svalue(Size::uvalue(value1) << value2);
    Size::put(c.regs.d[reg1], value);
    c.regs.ccr.set_cc_lsl(value, value1, value2 + (32 - Size::value_bit())); // FIXME?

    c.regs.advance_pc(2);
  }

#if 0
  void
  lslb_i(uint16_type op, context &ec, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int count = op >> 9 & 0x7;
    if (count == 0)
      count = 8;
#ifdef HAVE_NANA_H
    L(" lslb #%u", count);
    L(",%%d%u\n", reg1);
#endif

    sint_type value1 = extsb(ec.regs.d[reg1]);
    sint_type value = extsb(uint16_type(value1) << count);
    const uint32_type MASK = ((uint32_type) 1u << 8) - 1;
    ec.regs.d[reg1] = ec.regs.d[reg1] & ~MASK | uint32_type(value) & MASK;
    ec.regs.ccr.set_cc_lsl(value, value1, count + (32 - 8));

    ec.regs.pc += 2;
  }

  void
  lslw_i(uint16_type op, context &ec, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int count = op >> 9 & 0x7;
    if (count == 0)
      count = 8;
#ifdef HAVE_NANA_H
    L(" lslw #%u", count);
    L(",%%d%u\n", reg1);
#endif

    sint_type value1 = extsw(ec.regs.d[reg1]);
    sint_type value = extsw(uint16_type(value1) << count);
    const uint32_type MASK = ((uint32_type) 1u << 16) - 1;
    ec.regs.d[reg1] = ec.regs.d[reg1] & ~MASK | uint32_type(value) & MASK;
    ec.regs.ccr.set_cc_lsl(value, value1, count + (32 - 16));

    ec.regs.pc += 2;
  }

  void
  lsll_i(uint16_type op, context &ec, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int count = op >> 9 & 0x7;
    if (count == 0)
      count = 8;
#ifdef HAVE_NANA_H
    L(" lsll #%u", count);
    L(",%%d%u\n", reg1);
#endif

    sint32_type value1 = extsl(ec.regs.d[reg1]);
    sint32_type value = extsl(uint32_type(value1) << count);
    ec.regs.d[reg1] = value;
    ec.regs.ccr.set_cc_lsl(value, value1, count);

    ec.regs.pc += 2;
  }
#endif

  /* Handles a LSR instruction (register).  */
  template <class Size> void
  m68k_lsr_r(uint16_type op, context &c, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\tlsr%s %%d%u,%%d%u\n", Size::suffix(), reg2, reg1);
#endif

    unsigned int value2 = c.regs.d[reg2] % Size::value_bit();
    typename Size::svalue_type value1 = Size::get(c.regs.d[reg1]);
    typename Size::svalue_type value =
      Size::svalue(Size::uvalue(value1) >> value2);
    Size::put(c.regs.d[reg1], value);
    c.regs.ccr.set_cc_lsr(value, value1, value2);

    c.regs.advance_pc(2);
  }

#if 0
  void
  lsrw_r(uint16_type op, context &ec, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L(" lsrw %%d%u", reg2);
    L(",%%d%u\n", reg1);
#endif

    uint16_type count = ec.regs.d[reg2];
    sint_type value1 = extsw(ec.regs.d[reg1]);
    sint_type value = extsw((uint16_type(value1) & 0xffffu) >> count);
    const uint32_type MASK = (uint32_type(1) << 16) - 1;
    ec.regs.d[reg1] = ec.regs.d[reg1] & ~MASK | uint32_type(value) & MASK;
    ec.regs.ccr.set_cc_lsr(value, value1, count);

    ec.regs.pc += 2;
  }

  void
  lsrl_r(uint16_type op, context &ec, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L(" lsrl %%d%u", reg2);
    L(",%%d%u\n", reg1);
#endif

    uint16_type count = ec.regs.d[reg2];
    sint32_type value1 = extsl(ec.regs.d[reg1]);
    sint32_type value = extsl((uint32_type(value1) & 0xffffffffu) >> count);
    ec.regs.d[reg1] = value;
    ec.regs.ccr.set_cc_lsr(value, value1, count);

    ec.regs.pc += 2;
  }
#endif

  /* Handles a LSR instruction (immediate).  */
  template <class Size> void
  m68k_lsr_i(uint16_type op, context &c, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int value2 = op >> 9 & 0x7;
    if (value2 == 0)
      value2 = 8;
#ifdef HAVE_NANA_H
    L("\tlsr%s #%u,%%d%u\n", Size::suffix(), value2, reg1);
#endif

    typename Size::svalue_type value1 = Size::get(c.regs.d[reg1]);
    typename Size::svalue_type value
      = Size::svalue(Size::uvalue(value1) >> value2);
    Size::put(c.regs.d[reg1], value);
    c.regs.ccr.set_cc_lsr(value, value1, value2);

    c.regs.advance_pc(2);
  }

#if 0
  void
  lsrb_i(uint16_type op, context &c, unsigned long data)
  {
    unsigned int count = op >> 9 & 0x7;
    if (count == 0)
      count = 8;
    data_register ea1(op & 0x7, 2);
#ifdef HAVE_NANA_H
    L(" lsrb #%u", count);
    L(",%s\n", ea1.textb(c));
#endif

    sint_type value1 = ea1.getb(c);
    sint_type value = extsb((uint16_type(value1) & 0xffu) >> count);
    ea1.putb(c, value);
    c.regs.ccr.set_cc_lsr(value, value1, count);

    c.regs.pc += 2 + ea1.isize(2);
  }

  void
  lsrw_i(uint16_type op, context &ec, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    uint16_type count = op >> 9 & 0x7;
    if (count == 0)
      count = 8;
#ifdef HAVE_NANA_H
    L(" lsrw #%u", count);
    L(",%%d%u\n", reg1);
#endif

    sint_type value1 = extsw(ec.regs.d[reg1]);
    sint_type value = extsw((uint16_type(value1) & 0xffffu) >> count);
    const uint32_type MASK = (uint32_type(1) << 16) - 1;
    ec.regs.d[reg1] = ec.regs.d[reg1] & ~MASK | uint32_type(value) & MASK;
    ec.regs.ccr.set_cc_lsr(value, value1, count);

    ec.regs.pc += 2;
  }

  void
  lsrl_i(uint16_type op, context &ec, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    uint16_type count = op >> 9 & 0x7;
    if (count == 0)
      count = 8;
#ifdef HAVE_NANA_H
    L(" lsrl #%u", count);
    L(",%%d%u\n", reg1);
#endif

    sint32_type value1 = extsl(ec.regs.d[reg1]);
    sint32_type value = extsl((uint32_type(value1) & 0xffffffffu) >> count);
    ec.regs.d[reg1] = value;
    ec.regs.ccr.set_cc_lsr(value, value1, count);

    ec.regs.pc += 2;
  }
#endif

  /* Handles a LSR instruction (memory).  */
  template <class Destination> void
  m68k_lsr_m(uint16_type op, context &c, unsigned long data)
  {
    Destination ea1(op & 7, 2);
#ifdef HAVE_NANA_H
    L("\tlsr%s %s\n", word_size::suffix(), ea1.text(c).c_str());
#endif

    word_size::svalue_type value1 = ea1.get(c);
    word_size::svalue_type value
      = word_size::svalue(word_size::uvalue(value1) >> 1);
    ea1.put(c, value);
    c.regs.ccr.set_cc_lsr(value, value1, 1);

    ea1.finish(c);
    c.regs.advance_pc(2 + Destination::extension_size());
  }

  /* Handles a ROL instruction (register).  */
  template <class Size> void
  m68k_rol_r(uint16_type op, context &c, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\trol%s ", Size::suffix());
    L("%%d%u,", reg2);
    L("%%d%u\n", reg1);
#endif

    unsigned int count = c.regs.d[reg2] % Size::value_bit();
    typename Size::svalue_type value1 = Size::get(c.regs.d[reg1]);
    typename Size::svalue_type value
      = Size::svalue(Size::uvalue(value1) << count
		     | ((Size::uvalue(value1) & Size::value_mask())
			>> Size::value_bit() - count));
    Size::put(c.regs.d[reg1], value);
    c.regs.ccr.set_cc(value);	// FIXME.

    c.regs.pc += 2;
  }

#if 0
  void
  rolb_r(uint16_type op, context &ec, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L(" rolb %%d%u", reg2);
    L(",%%d%u\n", reg1);
#endif

    uint16_type count = ec.regs.d[reg2] & 0x7;
    sint_type value1 = extsb(ec.regs.d[reg1]);
    sint_type value = extsb(uint16_type(value1) << count
			    | (uint16_type(value1) & 0xff) >> 8 - count);
    const uint32_type MASK = 0xff;
    ec.regs.d[reg1] = ec.regs.d[reg1] & ~MASK | uint32_type(value) & MASK;
    ec.regs.ccr.set_cc(value);	// FIXME.

    ec.regs.pc += 2;
  }
#endif

  /* Handles a ROL instruction (immediate).  */
  template <class Size> void
  m68k_rol_i(uint16_type op, context &c, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int value2 = ((op >> 9) - 1 & 0x7) + 1;
#ifdef HAVE_NANA_H
    L("\trol%s #%u,%%d%u\n", Size::suffix(), value2, reg1);
#endif

    typename Size::svalue_type value1 = Size::get(c.regs.d[reg1]);
    typename Size::svalue_type value
      = Size::svalue(Size::uvalue(value1) << value2
		     | Size::uvalue(value1) >> Size::value_bit() - value2);
    Size::put(c.regs.d[reg1], value);
    c.regs.ccr.set_cc(value);	// FIXME.

    c.regs.advance_pc(2);
  }

#if 0
  void
  rolw_i(uint16_type op, context &ec, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int count = op >> 9 & 0x7;
    if (count == 0)
      count = 8;
#ifdef HAVE_NANA_H
    L(" rolw #%u", count);
    L(",%%d%u\n", reg1);
#endif

    sint_type value1 = extsw(ec.regs.d[reg1]);
    sint_type value = extsw(uint16_type(value1) << count
			    | (uint16_type(value1) & 0xffffu) >> 16 - count);
    const uint32_type MASK = 0xffffu;
    ec.regs.d[reg1] = ec.regs.d[reg1] & ~MASK | uint32_type(value) & MASK;
    ec.regs.ccr.set_cc(value);	// FIXME.

    ec.regs.pc += 2;
  }

  void
  roll_i(uint16_type op, context &c, unsigned long data)
  {
    unsigned int count = op >> 9 & 0x7;
    if (count == 0)
      count = 8;
    data_register ea1(op & 0x7, 2);
#ifdef HAVE_NANA_H
    L(" roll #%u", count);
    L(",%s\n", ea1.textl(c));
#endif

    sint32_type value1 = ea1.getl(c);
    sint32_type value
      = extsl(uint32_type(value1) << count
	      | (uint32_type(value1) & 0xffffffffu) >> 32 - count);
    ea1.putl(c, value);
    c.regs.ccr.set_cc(value);	// FIXME.
    ea1.finishl(c);

    c.regs.pc += 2 + ea1.isize(4);
  }
#endif

  /* Handles a ROR instruction (immediate).  */
  template <class Size> void
  m68k_ror_i(uint16_type op, context &c, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int count = op >> 9 & 0x7;
    if (count == 0)
      count = 8;
#ifdef HAVE_NANA_H
    L("\tror%s #%u,%%d%u\n", Size::suffix(), count, reg1);
#endif

    typename Size::svalue_type value1 = Size::get(c.regs.d[reg1]);
    typename Size::svalue_type value
      = Size::svalue(((Size::uvalue(value1) & Size::value_mask()) >> count)
		     | (Size::uvalue(value1) << Size::value_bit() - count));
    Size::put(c.regs.d[reg1], value);
    c.regs.ccr.set_cc(value);	// FIXME.

    c.regs.pc += 2;
  }

  /* Handles a ROXR instruction (immediate).  */
  template <class Size> void
  m68k_roxr_i(uint16_type op, context &c, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int value2 = ((op >> 9) - 1 & 0x7) + 1;
#ifdef HAVE_NANA_H
    L("\troxr%s #%u,%%d%u", Size::suffix(), value2, reg1);
#endif

    typename Size::svalue_type value1 = Size::get(c.regs.d[reg1]);
    typename Size::svalue_type value
      = Size::svalue(Size::uvalue(value1) >> value2
		     | c.regs.ccr.x() << Size::value_bit() - value2
		     | Size::uvalue(value1) << Size::value_bit() + 1 - value2);
    Size::put(c.regs.d[reg1], value);
    c.regs.ccr.set_cc(value);	// FIXME.

    c.regs.advance_pc(2);
  }

#if 0
  void
  roxrw_i(uint16_type op, context &c, unsigned long data)
  {
    unsigned int count = op >> 9 & 0x7;
    if (count == 0)
      count = 8;
    data_register ea1(op & 0x7, 2);
#ifdef HAVE_NANA_H
    L(" roxrw #%u", count);
    L(",%s\n", ea1.textw(c));
#endif

    sint_type value1 = ea1.getw(c);
    sint_type value = extsw((uint16_type(value1) & 0xffffu) >> count
			    | c.regs.ccr.x() << 16 - count
			    | uint16_type(value1) << 17 - count);
    ea1.putw(c, value);
    c.regs.ccr.set_cc(value);	// FIXME.
    ea1.finishw(c);

    c.regs.pc += 2 + ea1.isize(2);
  }

  void
  roxrl_i(uint16_type op, context &c, unsigned long data)
  {
    unsigned int count = op >> 9 & 0x7;
    if (count == 0)
      count = 8;
    data_register ea1(op & 0x7, 2);
#ifdef HAVE_NANA_H
    L(" roxrl #%u", count);
    L(",%s\n", ea1.textl(c));
#endif

    sint32_type value1 = ea1.getl(c);
    sint32_type value = extsl((uint32_type(value1) & 0xffffffffu) >> count
			      | uint32_type(c.regs.ccr.x()) << 32 - count
			      | uint32_type(value1) << 33 - count);
    ea1.putl(c, value);
    c.regs.ccr.set_cc(value);	// FIXME.
    ea1.finishl(c);

    c.regs.pc += 2 + ea1.isize(4);
  }
#endif
}

void
vm68k::install_instructions_14(exec_unit &eu, unsigned long data)
{
  static const instruction_map inst[]
    = {{0xe000, 0xe07, &m68k_asr_i<byte_size>},
       {0xe008, 0xe07, &m68k_lsr_i<byte_size>},
       {0xe010, 0xe07, &m68k_roxr_i<byte_size>},
       {0xe018, 0xe07, &m68k_ror_i<byte_size>},
       {0xe020, 0xe07, &m68k_asr_r<byte_size>},
       {0xe028, 0xe07, &m68k_lsr_r<byte_size>},
       {0xe040, 0xe07, &m68k_asr_i<word_size>},
       {0xe048, 0xe07, &m68k_lsr_i<word_size>},
       {0xe050, 0xe07, &m68k_roxr_i<word_size>},
       {0xe058, 0xe07, &m68k_ror_i<word_size>},
       {0xe060, 0xe07, &m68k_asr_r<word_size>},
       {0xe068, 0xe07, &m68k_lsr_r<word_size>},
       {0xe080, 0xe07, &m68k_asr_i<long_word_size>},
       {0xe088, 0xe07, &m68k_lsr_i<long_word_size>},
       {0xe090, 0xe07, &m68k_roxr_i<long_word_size>},
       {0xe098, 0xe07, &m68k_ror_i<long_word_size>},
       {0xe0a0, 0xe07, &m68k_asr_r<long_word_size>},
       {0xe0a8, 0xe07, &m68k_lsr_r<long_word_size>},
       {0xe100, 0xe07, &m68k_asl_i<byte_size>},
       {0xe108, 0xe07, &m68k_lsl_i<byte_size>},
       {0xe118, 0xe07, &m68k_rol_i<byte_size>},
       {0xe120, 0xe07, &m68k_asl_r<byte_size>},
       {0xe128, 0xe07, &m68k_lsl_r<byte_size>},
       {0xe138, 0xe07, &m68k_rol_r<byte_size>},
       {0xe140, 0xe07, &m68k_asl_i<word_size>},
       {0xe148, 0xe07, &m68k_lsl_i<word_size>},
       {0xe158, 0xe07, &m68k_rol_i<word_size>},
       {0xe160, 0xe07, &m68k_asl_r<word_size>},
       {0xe168, 0xe07, &m68k_lsl_r<word_size>},
       {0xe178, 0xe07, &m68k_rol_r<word_size>},
       {0xe180, 0xe07, &m68k_asl_i<long_word_size>},
       {0xe188, 0xe07, &m68k_lsl_i<long_word_size>},
       {0xe198, 0xe07, &m68k_rol_i<long_word_size>},
       {0xe1a0, 0xe07, &m68k_asl_r<long_word_size>},
       {0xe1a8, 0xe07, &m68k_lsl_r<long_word_size>},
       {0xe1b8, 0xe07, &m68k_rol_r<long_word_size>},
       {0xe2d0,     7, &m68k_lsr_m<word_indirect>},
       {0xe2d8,     7, &m68k_lsr_m<word_postinc_indirect>},
       {0xe2e0,     7, &m68k_lsr_m<word_predec_indirect>},
       {0xe2e8,     7, &m68k_lsr_m<word_disp_indirect>},
       {0xe2f0,     7, &m68k_lsr_m<word_index_indirect>},
       {0xe2f8,     0, &m68k_lsr_m<word_abs_short>},
       {0xe2f9,     0, &m68k_lsr_m<word_abs_long>}};

  for (const instruction_map *i = inst + 0;
       i != inst + sizeof inst / sizeof inst[0]; ++i)
    eu.set_instruction(i->base, i->mask, make_pair(i->handler, data));
}