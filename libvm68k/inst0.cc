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
  using vm68k::privilege_violation_exception;

  /* Handles an ADDI instruction.  */
  template <class Size, class Destination> void
  m68k_addi(uint16_type op, context &c, unsigned long data)
  {
    typename Size::svalue_type value2 = c.fetch(Size(), 2);
    Destination ea1(op & 0x7, 2 + Size::aligned_value_size());
#ifdef HAVE_NANA_H
    L("\taddi%s #%#lx,%s\n", Size::suffix(), Size::uvalue(value2) + 0UL,
      ea1.text(c).c_str());
#endif

    typename Size::svalue_type value1 = ea1.get(c);
    typename Size::svalue_type value = Size::svalue(value1 + value2);
    ea1.put(c, value);
    c.regs.ccr.set_cc_as_add(value, value1, value2);
    ea1.finish(c);

    c.regs.pc += 2 + Size::aligned_value_size() + ea1.extension_size();
  }

  /* Handles an ANDI instruction.  */
  template <class Size, class Destination> void
  m68k_andi(uint16_type op, context &c, unsigned long data)
  {
    typename Size::svalue_type value2 = c.fetch(Size(), 2);
    Destination ea1(op & 0x7, 2 + Size::aligned_value_size());
#ifdef HAVE_NANA_H
    L("\tandi%s #%#lx,%s\n", Size::suffix(), Size::uvalue(value2) + 0UL,
      ea1.text(c).c_str());
#endif

    typename Size::svalue_type value1 = ea1.get(c);
    typename Size::svalue_type value
      = Size::svalue(Size::uvalue(value1) & Size::uvalue(value2));
    ea1.put(c, value);
    c.regs.ccr.set_cc(value);
    ea1.finish(c);

    c.regs.pc += 2 + Size::aligned_value_size() + ea1.extension_size();
  }

  /* Handles an ANDI-to-CCR instruction.  */
  void
  m68k_andi_to_ccr(uint16_type op, context &c, unsigned long data)
  {
    byte_size::uvalue_type value2 = c.ufetch(byte_size(), 2);
#ifdef HAVE_NANA_H
    L("\tandi%s #%#x,%%ccr\n", byte_size::suffix(), value2);
#endif

    byte_size::uvalue_type value1 = c.regs.ccr & 0xffu;
    byte_size::uvalue_type value = value1 & value2;
    c.regs.ccr = c.regs.ccr & ~0xffu | value & 0xffu;

    c.regs.pc += 2 + byte_size::aligned_value_size();
  }

  /* Handles an ANDI-to-SR instruction.  */
  void
  m68k_andi_to_sr(uint16_type op, context &c, unsigned long data)
  {
    word_size::uvalue_type value2 = c.ufetch(word_size(), 2);
#ifdef HAVE_NANA_H
    L("\tandi%s #%#x,%%sr\n", word_size::suffix(), value2);
#endif

    // This instruction is privileged.
    if (!c.supervisor_state())
      throw privilege_violation_exception();

    word_size::uvalue_type value1 = c.sr();
    word_size::uvalue_type value = value1 & value2;
    c.set_sr(value);

    c.regs.advance_pc(2 + word_size::aligned_value_size());
  }

  /* Handles a BCLR instruction (register).  */
  template <class Size, class Destination> void
  m68k_bclr_r(uint16_type op, context &c, unsigned long data)
  {
    Destination ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\tbclr%s %%d%u,%s\n", Size::suffix(), reg2, ea1.text(c).c_str());
#endif

    // This instruction affects only the Z bit of the condition codes.
    unsigned int value2 = c.regs.d[reg2] % Size::value_bit();
    typename Size::uvalue_type mask = Size::uvalue(1) << value2;
    typename Size::uvalue_type value1 = ea1.get(c);
    bool value = value1 & mask;
    ea1.put(c, value1 & ~mask);
    c.regs.ccr.set_cc(value);	// FIXME.
    ea1.finish(c);

    c.regs.pc += 2 + ea1.extension_size();
  }

  /* Handles a BCLR instruction (immediate).  */
  template <class Size, class Destination> void
  m68k_bclr_i(uint16_type op, context &c, unsigned long data)
  {
    Destination ea1(op & 0x7, 2 + 2);
    unsigned int value2 = c.ufetch(word_size(), 2) % Size::value_bit();
#ifdef HAVE_NANA_H
    L("\tbclr%s #%u,%s\n", Size::suffix(), value2, ea1.text(c).c_str());
#endif

    // This instruction affects only the Z bit of the condition codes.
    typename Size::uvalue_type mask = Size::uvalue(1) << value2;
    typename Size::uvalue_type value1 = ea1.get(c);
    bool value = value1 & mask;
    ea1.put(c, value1 & ~mask);
    c.regs.ccr.set_cc(value);	// FIXME.

    ea1.finish(c);
    c.regs.pc += 2 + 2 + ea1.extension_size();
  }

  /* Handles a BSET instruction (register).  */
  template <class Size, class Destination> void
  m68k_bset_r(uint16_type op, context &c, unsigned long data)
  {
    Destination ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\tbset%s %%d%u,%s\n", Size::suffix(), reg2, ea1.text(c).c_str());
#endif

    // This instruction affects only the Z bit of the condition codes.
    typename Size::uvalue_type mask = Size::uvalue(1) << c.regs.d[reg2] % Size::value_bit();
    typename Size::uvalue_type value1 = ea1.get(c);
    bool value = value1 & mask;
    ea1.put(c, value1 | mask);
    c.regs.ccr.set_cc(value);	// FIXME.

    c.regs.pc += 2 + ea1.extension_size();
  }

  /* Handles a BSET instruction (immediate).  */
  template <class Size, class Destination> void
  m68k_bset_i(uint16_type op, context &c, unsigned long data)
  {
    unsigned int value2 = c.ufetch(word_size(), 2) % Size::value_bit();
    Destination ea1(op & 0x7, 2 + word_size::aligned_value_size());
#ifdef HAVE_NANA_H
    L("\tbset%s #%u,%s\n", Size::suffix(), value2, ea1.text(c).c_str());
#endif

    // This instruction affects only the Z bit of the condition codes.
    typename Size::uvalue_type mask = Size::uvalue(1) << value2;
    typename Size::uvalue_type value1 = ea1.get(c);
    bool value = value1 & mask;
    ea1.put(c, value1 | mask);
    c.regs.ccr.set_cc(value);	// FIXME.

    ea1.finish(c);
    c.regs.pc += (2 + word_size::aligned_value_size()
		  + Destination::extension_size());
  }

  /* Handles a BTST instruction (register).  */
  template <class Size, class Destination> void
  m68k_btst_r(uint16_type op, context &c, unsigned long data)
  {
    Destination ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\tbtst%s %%d%u,%s\n", Size::suffix(), reg2, ea1.text(c).c_str());
#endif

    // This instruction affects only the Z bit of the condition codes.
    unsigned int value2 = c.regs.d[reg2] % Size::value_bit();
    typename Size::uvalue_type mask = Size::uvalue(1) << value2;
    typename Size::uvalue_type value1 = ea1.get(c);
    bool value = value1 & mask;
    c.regs.ccr.set_cc(value);	// FIXME.
    ea1.finish(c);

    c.regs.pc += 2 + ea1.extension_size();
  }

  /* Handles a BTST instruction (immediate).  */
  template <class Size, class Destination> void
  m68k_btst_i(uint16_type op, context &c, unsigned long data)
  {
    unsigned int value2 = c.ufetch(word_size(), 2) % Size::value_bit();
    Destination ea1(op & 0x7, 2 + word_size::aligned_value_size());
#ifdef HAVE_NANA_H
    L("\tbtst%s #%u,%s\n", Size::suffix(), value2, ea1.text(c).c_str());
#endif

    // This instruction affects only the Z bit of the condition codes.
    typename Size::uvalue_type mask = Size::uvalue(1) << value2;
    typename Size::svalue_type value1 = ea1.get(c);
    bool value = Size::uvalue(value1) & mask;
    c.regs.ccr.set_cc(value);	// FIXME.

    ea1.finish(c);
    long_word_size::put(c.regs.pc,
			c.regs.pc + 2 + word_size::aligned_value_size()
			+ Destination::extension_size());
  }

#if 0
  template <class Destination> void
  btstb_i(uint16_type op, context &c, unsigned long data)
  {
    unsigned int bit = c.fetch(word_size(), 2) & 0x7;
    Destination ea1(op & 0x7, 2 + 2);
#ifdef HAVE_NANA_H
    L(" btstb #%u", bit);
    L(",%s\n", ea1.textb(c));
#endif

    // This instruction affects only the Z bit of condition codes.
    bool value = uint16_type(ea1.getb(c)) & 1u << bit;
    c.regs.ccr.set_cc(value);	// FIXME.

    c.regs.pc += 2 + 2 + ea1.isize(2);
  }

  void
  btstl_i(uint16_type op, context &ec, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int bit = ec.fetch(word_size(), 2) & 0x1f;
#ifdef HAVE_NANA_H
    L(" btstl #%u", bit);
    L(",%%d%u\n", reg1);
#endif

    // This instruction affects only the Z bit of condition codes.
    bool value = ec.regs.d[reg1] & uint32_type(1) << bit;
    ec.regs.ccr.set_cc(value);	// FIXME.

    ec.regs.pc += 2 + 2;
  }
#endif

  template <class Size, class Destination> void
  m68k_cmpi(uint16_type op, context &c, unsigned long data)
  {
    typename Size::svalue_type value2 = c.fetch(Size(), 2);
    Destination ea1(op & 0x7, 2 + Size::aligned_value_size());
#ifdef HAVE_NANA_H
    L("\tcmpi%s #%#lx,%s\n", Size::suffix(), Size::uvalue(value2) + 0UL,
      ea1.text(c).c_str());
#endif

    typename Size::svalue_type value1 = ea1.get(c);
    typename Size::svalue_type value = Size::svalue(value1 - value2);
    c.regs.ccr.set_cc_cmp(value, value1, value2);

    ea1.finish(c);
    c.regs.advance_pc(2 + Size::aligned_value_size()
		      + Destination::extension_size());
  }

  /* Handles an EORI instruction.  */
  template <class Size, class Destination> void
  m68k_eori(uint16_type op, context &c, unsigned long data)
  {
    typename Size::svalue_type value2 = c.fetch(Size(), 2);
    Destination ea1(op & 0x7, 2 + Size::aligned_value_size());
#ifdef HAVE_NANA_H
    L("\teori%s #%#lx,%s\n", Size::suffix(), Size::uvalue(value2) + 0UL,
      ea1.text(c).c_str());
#endif

    typename Size::svalue_type value1 = ea1.get(c);
    typename Size::svalue_type value
      = Size::svalue(Size::uvalue(value1) ^ Size::uvalue(value2));
    ea1.put(c, value);
    c.regs.ccr.set_cc(value);

    ea1.finish(c);
    c.regs.pc += 2 + Size::aligned_value_size() + ea1.extension_size();
  }

  /* Handles an ORI instruction.  */
  template <class Size, class Destination> void
  m68k_ori(uint16_type op, context &c, unsigned long data)
  {
    typename Size::svalue_type value2 = c.fetch(Size(), 2);
    Destination ea1(op & 0x7, 2 + Size::aligned_value_size());
#ifdef HAVE_NANA_H
    L("\tori%s #%#lx,%s", Size::suffix(), Size::uvalue(value2) + 0UL,
      ea1.text(c).c_str());
#endif

    typename Size::svalue_type value1 = ea1.get(c);
    typename Size::svalue_type value
      = Size::svalue(Size::uvalue(value1) | Size::uvalue(value2));
    ea1.put(c, value);
    c.regs.ccr.set_cc(value);

    ea1.finish(c);
    c.regs.advance_pc(2 + Size::aligned_value_size()
		      + Destination::extension_size());
  }

  /* Handles an ORI-to-CCR instruction.  */
  void
  m68k_ori_to_ccr(uint16_type op, context &c, unsigned long data)
  {
    byte_size::uvalue_type value2 = c.ufetch(byte_size(), 2);
#ifdef HAVE_NANA_H
    L("\tori%s #%#x,%%ccr\n", byte_size::suffix(), value2);
#endif

    byte_size::uvalue_type value1 = c.regs.ccr & 0xffu;
    byte_size::uvalue_type value = value1 | value2;
    c.regs.ccr = c.regs.ccr & ~0xffu | value & 0xffu;

    c.regs.pc += 2 + byte_size::aligned_value_size();
  }

  /* Handles an ORI-to-SR instruction.  */
  void
  m68k_ori_to_sr(uint16_type op, context &c, unsigned long data)
  {
    word_size::uvalue_type value2 = c.ufetch(word_size(), 2);
#ifdef HAVE_NANA_H
    L("\tori%s #%#x,%%sr\n", word_size::suffix(), value2);
#endif

    // This instruction is privileged.
    if (!c.supervisor_state())
      throw privilege_violation_exception();

    word_size::uvalue_type value1 = c.sr();
    word_size::uvalue_type value = value1 | value2;
    c.set_sr(value);

    c.regs.pc += 2 + word_size::aligned_value_size();
  }

  /* Handles a SUBI instruction.  */
  template <class Size, class Destination> void
  m68k_subi(uint16_type op, context &c, unsigned long data)
  {
    typename Size::svalue_type value2 = c.fetch(word_size(), 2);
    Destination ea1(op & 0x7, 2 + word_size::aligned_value_size());
#ifdef HAVE_NANA_H
    L("\tsubi%s #%#lx,%s\n", Size::suffix(), Size::uvalue(value2) + 0UL,
      ea1.text(c).c_str());
#endif

    typename Size::svalue_type value1 = ea1.get(c);
    typename Size::svalue_type value = Size::svalue(value1 - value2);
    ea1.put(c, value);
    c.regs.ccr.set_cc_sub(value, value1, value2);

    ea1.finish(c);
    long_word_size::put(c.regs.pc,
			c.regs.pc + 2 + word_size::aligned_value_size()
			+ Destination::extension_size());
  }
}

void
vm68k::install_instructions_0(exec_unit &eu, unsigned long data)
{
  static const instruction_map inst[]
    = {{     0,     7, &m68k_ori<byte_size, byte_d_register>},
       {  0x10,     7, &m68k_ori<byte_size, byte_indirect>},
       {  0x18,     7, &m68k_ori<byte_size, byte_postinc_indirect>},
       {  0x20,     7, &m68k_ori<byte_size, byte_predec_indirect>},
       {  0x28,     7, &m68k_ori<byte_size, byte_disp_indirect>},
       {  0x30,     7, &m68k_ori<byte_size, byte_index_indirect>},
       {  0x38,     0, &m68k_ori<byte_size, byte_abs_short>},
       {  0x39,     0, &m68k_ori<byte_size, byte_abs_long>},
       {  0x3c,     0, &m68k_ori_to_ccr},
       {  0x40,     7, &m68k_ori<word_size, word_d_register>},
       {  0x50,     7, &m68k_ori<word_size, word_indirect>},
       {  0x58,     7, &m68k_ori<word_size, word_postinc_indirect>},
       {  0x60,     7, &m68k_ori<word_size, word_predec_indirect>},
       {  0x68,     7, &m68k_ori<word_size, word_disp_indirect>},
       {  0x70,     7, &m68k_ori<word_size, word_index_indirect>},
       {  0x78,     0, &m68k_ori<word_size, word_abs_short>},
       {  0x79,     0, &m68k_ori<word_size, word_abs_long>},
       {  0x7c,     0, &m68k_ori_to_sr},
       {  0x80,     7, &m68k_ori<long_word_size, long_word_d_register>},
       {  0x90,     7, &m68k_ori<long_word_size, long_word_indirect>},
       {  0x98,     7, &m68k_ori<long_word_size, long_word_postinc_indirect>},
       {  0xa0,     7, &m68k_ori<long_word_size, long_word_predec_indirect>},
       {  0xa8,     7, &m68k_ori<long_word_size, long_word_disp_indirect>},
       {  0xb0,     7, &m68k_ori<long_word_size, long_word_index_indirect>},
       {  0xb8,     0, &m68k_ori<long_word_size, long_word_abs_short>},
       {  0xb9,     0, &m68k_ori<long_word_size, long_word_abs_long>},
       { 0x100, 0xe07, &m68k_btst_r<long_word_size, long_word_d_register>},
       { 0x110, 0xe07, &m68k_btst_r<byte_size, byte_indirect>},
       { 0x118, 0xe07, &m68k_btst_r<byte_size, byte_postinc_indirect>},
       { 0x120, 0xe07, &m68k_btst_r<byte_size, byte_predec_indirect>},
       { 0x128, 0xe07, &m68k_btst_r<byte_size, byte_disp_indirect>},
       { 0x130, 0xe07, &m68k_btst_r<byte_size, byte_index_indirect>},
       { 0x138, 0xe00, &m68k_btst_r<byte_size, byte_abs_short>},
       { 0x139, 0xe00, &m68k_btst_r<byte_size, byte_abs_long>},
       { 0x180, 0xe07, &m68k_bclr_r<long_word_size, long_word_d_register>},
       { 0x190, 0xe07, &m68k_bclr_r<byte_size, byte_indirect>},
       { 0x198, 0xe07, &m68k_bclr_r<byte_size, byte_postinc_indirect>},
       { 0x1a0, 0xe07, &m68k_bclr_r<byte_size, byte_predec_indirect>},
       { 0x1a8, 0xe07, &m68k_bclr_r<byte_size, byte_disp_indirect>},
       { 0x1b0, 0xe07, &m68k_bclr_r<byte_size, byte_index_indirect>},
       { 0x1b8, 0xe00, &m68k_bclr_r<byte_size, byte_abs_short>},
       { 0x1b9, 0xe00, &m68k_bclr_r<byte_size, byte_abs_long>},
       { 0x1c0, 0xe07, &m68k_bset_r<long_word_size, long_word_d_register>},
       { 0x1d0, 0xe07, &m68k_bset_r<byte_size, byte_indirect>},
       { 0x1d8, 0xe07, &m68k_bset_r<byte_size, byte_postinc_indirect>},
       { 0x1e0, 0xe07, &m68k_bset_r<byte_size, byte_predec_indirect>},
       { 0x1e8, 0xe07, &m68k_bset_r<byte_size, byte_disp_indirect>},
       { 0x1f0, 0xe07, &m68k_bset_r<byte_size, byte_index_indirect>},
       { 0x1f8, 0xe00, &m68k_bset_r<byte_size, byte_abs_short>},
       { 0x1f9, 0xe00, &m68k_bset_r<byte_size, byte_abs_long>},
       { 0x200,     7, &m68k_andi<byte_size, byte_d_register>},
       { 0x210,     7, &m68k_andi<byte_size, byte_indirect>},
       { 0x218,     7, &m68k_andi<byte_size, byte_postinc_indirect>},
       { 0x220,     7, &m68k_andi<byte_size, byte_predec_indirect>},
       { 0x228,     7, &m68k_andi<byte_size, byte_disp_indirect>},
       { 0x230,     7, &m68k_andi<byte_size, byte_index_indirect>},
       { 0x238,     0, &m68k_andi<byte_size, byte_abs_short>},
       { 0x239,     0, &m68k_andi<byte_size, byte_abs_long>},
       { 0x23c,     0, &m68k_andi_to_ccr},
       { 0x240,     7, &m68k_andi<word_size, word_d_register>},
       { 0x250,     7, &m68k_andi<word_size, word_indirect>},
       { 0x258,     7, &m68k_andi<word_size, word_postinc_indirect>},
       { 0x260,     7, &m68k_andi<word_size, word_predec_indirect>},
       { 0x268,     7, &m68k_andi<word_size, word_disp_indirect>},
       { 0x270,     7, &m68k_andi<word_size, word_index_indirect>},
       { 0x278,     0, &m68k_andi<word_size, word_abs_short>},
       { 0x279,     0, &m68k_andi<word_size, word_abs_long>},
       { 0x27c,     0, &m68k_andi_to_sr},
       { 0x280,     7, &m68k_andi<long_word_size, long_word_d_register>},
       { 0x290,     7, &m68k_andi<long_word_size, long_word_indirect>},
       { 0x298,     7, &m68k_andi<long_word_size, long_word_postinc_indirect>},
       { 0x2a0,     7, &m68k_andi<long_word_size, long_word_predec_indirect>},
       { 0x2a8,     7, &m68k_andi<long_word_size, long_word_disp_indirect>},
       { 0x2b0,     7, &m68k_andi<long_word_size, long_word_index_indirect>},
       { 0x2b8,     0, &m68k_andi<long_word_size, long_word_abs_short>},
       { 0x2b9,     0, &m68k_andi<long_word_size, long_word_abs_long>},
       { 0x400,     7, &m68k_subi<byte_size, byte_d_register>},
       { 0x410,     7, &m68k_subi<byte_size, byte_indirect>},
       { 0x418,     7, &m68k_subi<byte_size, byte_postinc_indirect>},
       { 0x420,     7, &m68k_subi<byte_size, byte_predec_indirect>},
       { 0x428,     7, &m68k_subi<byte_size, byte_disp_indirect>},
       { 0x430,     7, &m68k_subi<byte_size, byte_index_indirect>},
       { 0x438,     0, &m68k_subi<byte_size, byte_abs_short>},
       { 0x439,     0, &m68k_subi<byte_size, byte_abs_long>},
       { 0x440,     7, &m68k_subi<word_size, word_d_register>},
       { 0x450,     7, &m68k_subi<word_size, word_indirect>},
       { 0x458,     7, &m68k_subi<word_size, word_postinc_indirect>},
       { 0x460,     7, &m68k_subi<word_size, word_predec_indirect>},
       { 0x468,     7, &m68k_subi<word_size, word_disp_indirect>},
       { 0x470,     7, &m68k_subi<word_size, word_index_indirect>},
       { 0x478,     0, &m68k_subi<word_size, word_abs_short>},
       { 0x479,     0, &m68k_subi<word_size, word_abs_long>},
       { 0x480,     7, &m68k_subi<long_word_size, long_word_d_register>},
       { 0x490,     7, &m68k_subi<long_word_size, long_word_indirect>},
       { 0x498,     7, &m68k_subi<long_word_size, long_word_postinc_indirect>},
       { 0x4a0,     7, &m68k_subi<long_word_size, long_word_predec_indirect>},
       { 0x4a8,     7, &m68k_subi<long_word_size, long_word_disp_indirect>},
       { 0x4b0,     7, &m68k_subi<long_word_size, long_word_index_indirect>},
       { 0x4b8,     0, &m68k_subi<long_word_size, long_word_abs_short>},
       { 0x4b9,     0, &m68k_subi<long_word_size, long_word_abs_long>},
       { 0x600,     7, &m68k_addi<byte_size, byte_d_register>},
       { 0x610,     7, &m68k_addi<byte_size, byte_indirect>},
       { 0x618,     7, &m68k_addi<byte_size, byte_postinc_indirect>},
       { 0x620,     7, &m68k_addi<byte_size, byte_predec_indirect>},
       { 0x628,     7, &m68k_addi<byte_size, byte_disp_indirect>},
       { 0x630,     7, &m68k_addi<byte_size, byte_index_indirect>},
       { 0x638,     0, &m68k_addi<byte_size, byte_abs_short>},
       { 0x639,     0, &m68k_addi<byte_size, byte_abs_long>},
       { 0x640,     7, &m68k_addi<word_size, word_d_register>},
       { 0x650,     7, &m68k_addi<word_size, word_indirect>},
       { 0x658,     7, &m68k_addi<word_size, word_postinc_indirect>},
       { 0x660,     7, &m68k_addi<word_size, word_predec_indirect>},
       { 0x668,     7, &m68k_addi<word_size, word_disp_indirect>},
       { 0x670,     7, &m68k_addi<word_size, word_index_indirect>},
       { 0x678,     0, &m68k_addi<word_size, word_abs_short>},
       { 0x679,     0, &m68k_addi<word_size, word_abs_long>},
       { 0x680,     7, &m68k_addi<long_word_size, long_word_d_register>},
       { 0x690,     7, &m68k_addi<long_word_size, long_word_indirect>},
       { 0x698,     7, &m68k_addi<long_word_size, long_word_postinc_indirect>},
       { 0x6a0,     7, &m68k_addi<long_word_size, long_word_predec_indirect>},
       { 0x6a8,     7, &m68k_addi<long_word_size, long_word_disp_indirect>},
       { 0x6b0,     7, &m68k_addi<long_word_size, long_word_index_indirect>},
       { 0x6b8,     0, &m68k_addi<long_word_size, long_word_abs_short>},
       { 0x6b9,     0, &m68k_addi<long_word_size, long_word_abs_long>},
       { 0x800,     7, &m68k_btst_i<long_word_size, long_word_d_register>},
       { 0x810,     7, &m68k_btst_i<byte_size, byte_indirect>},
       { 0x818,     7, &m68k_btst_i<byte_size, byte_postinc_indirect>},
       { 0x820,     7, &m68k_btst_i<byte_size, byte_predec_indirect>},
       { 0x828,     7, &m68k_btst_i<byte_size, byte_disp_indirect>},
       { 0x830,     7, &m68k_btst_i<byte_size, byte_index_indirect>},
       { 0x838,     0, &m68k_btst_i<byte_size, byte_abs_short>},
       { 0x839,     0, &m68k_btst_i<byte_size, byte_abs_long>},
       { 0x880,     7, &m68k_bclr_i<long_word_size, long_word_d_register>},
       { 0x890,     7, &m68k_bclr_i<byte_size, byte_indirect>},
       { 0x898,     7, &m68k_bclr_i<byte_size, byte_postinc_indirect>},
       { 0x8a0,     7, &m68k_bclr_i<byte_size, byte_predec_indirect>},
       { 0x8a8,     7, &m68k_bclr_i<byte_size, byte_disp_indirect>},
       { 0x8b0,     7, &m68k_bclr_i<byte_size, byte_index_indirect>},
       { 0x8b8,     0, &m68k_bclr_i<byte_size, byte_abs_short>},
       { 0x8b9,     0, &m68k_bclr_i<byte_size, byte_abs_long>},
       { 0x8c0,     7, &m68k_bset_i<long_word_size, long_word_d_register>},
       { 0x8d0,     7, &m68k_bset_i<byte_size, byte_indirect>},
       { 0x8d8,     7, &m68k_bset_i<byte_size, byte_postinc_indirect>},
       { 0x8e0,     7, &m68k_bset_i<byte_size, byte_predec_indirect>},
       { 0x8e8,     7, &m68k_bset_i<byte_size, byte_disp_indirect>},
       { 0x8f0,     7, &m68k_bset_i<byte_size, byte_index_indirect>},
       { 0x8f8,     0, &m68k_bset_i<byte_size, byte_abs_short>},
       { 0x8f9,     0, &m68k_bset_i<byte_size, byte_abs_long>},
       { 0xa00,     7, &m68k_eori<byte_size, byte_d_register>},
       { 0xa10,     7, &m68k_eori<byte_size, byte_indirect>},
       { 0xa18,     7, &m68k_eori<byte_size, byte_postinc_indirect>},
       { 0xa20,     7, &m68k_eori<byte_size, byte_predec_indirect>},
       { 0xa28,     7, &m68k_eori<byte_size, byte_disp_indirect>},
       { 0xa30,     7, &m68k_eori<byte_size, byte_index_indirect>},
       { 0xa38,     0, &m68k_eori<byte_size, byte_abs_short>},
       { 0xa39,     0, &m68k_eori<byte_size, byte_abs_long>},
       { 0xa40,     7, &m68k_eori<word_size, word_d_register>},
       { 0xa50,     7, &m68k_eori<word_size, word_indirect>},
       { 0xa58,     7, &m68k_eori<word_size, word_postinc_indirect>},
       { 0xa60,     7, &m68k_eori<word_size, word_predec_indirect>},
       { 0xa68,     7, &m68k_eori<word_size, word_disp_indirect>},
       { 0xa70,     7, &m68k_eori<word_size, word_index_indirect>},
       { 0xa78,     0, &m68k_eori<word_size, word_abs_short>},
       { 0xa79,     0, &m68k_eori<word_size, word_abs_long>},
       { 0xa80,     7, &m68k_eori<long_word_size, long_word_d_register>},
       { 0xa90,     7, &m68k_eori<long_word_size, long_word_indirect>},
       { 0xa98,     7, &m68k_eori<long_word_size, long_word_postinc_indirect>},
       { 0xaa0,     7, &m68k_eori<long_word_size, long_word_predec_indirect>},
       { 0xaa8,     7, &m68k_eori<long_word_size, long_word_disp_indirect>},
       { 0xab0,     7, &m68k_eori<long_word_size, long_word_index_indirect>},
       { 0xab8,     0, &m68k_eori<long_word_size, long_word_abs_short>},
       { 0xab9,     0, &m68k_eori<long_word_size, long_word_abs_long>},
       { 0xc00,     7, &m68k_cmpi<byte_size, byte_d_register>},
       { 0xc10,     7, &m68k_cmpi<byte_size, byte_indirect>},
       { 0xc18,     7, &m68k_cmpi<byte_size, byte_postinc_indirect>},
       { 0xc20,     7, &m68k_cmpi<byte_size, byte_predec_indirect>},
       { 0xc28,     7, &m68k_cmpi<byte_size, byte_disp_indirect>},
       { 0xc30,     7, &m68k_cmpi<byte_size, byte_index_indirect>},
       { 0xc38,     0, &m68k_cmpi<byte_size, byte_abs_short>},
       { 0xc39,     0, &m68k_cmpi<byte_size, byte_abs_long>},
       { 0xc40,     7, &m68k_cmpi<word_size, word_d_register>},
       { 0xc50,     7, &m68k_cmpi<word_size, word_indirect>},
       { 0xc58,     7, &m68k_cmpi<word_size, word_postinc_indirect>},
       { 0xc60,     7, &m68k_cmpi<word_size, word_predec_indirect>},
       { 0xc68,     7, &m68k_cmpi<word_size, word_disp_indirect>},
       { 0xc70,     7, &m68k_cmpi<word_size, word_index_indirect>},
       { 0xc78,     0, &m68k_cmpi<word_size, word_abs_short>},
       { 0xc79,     0, &m68k_cmpi<word_size, word_abs_long>},
       { 0xc80,     7, &m68k_cmpi<long_word_size, long_word_d_register>},
       { 0xc90,     7, &m68k_cmpi<long_word_size, long_word_indirect>},
       { 0xc98,     7, &m68k_cmpi<long_word_size, long_word_postinc_indirect>},
       { 0xca0,     7, &m68k_cmpi<long_word_size, long_word_predec_indirect>},
       { 0xca8,     7, &m68k_cmpi<long_word_size, long_word_disp_indirect>},
       { 0xcb0,     7, &m68k_cmpi<long_word_size, long_word_index_indirect>},
       { 0xcb8,     0, &m68k_cmpi<long_word_size, long_word_abs_short>},
       { 0xcb9,     0, &m68k_cmpi<long_word_size, long_word_abs_long>}};

  for (const instruction_map *i = inst + 0;
       i != inst + sizeof inst / sizeof inst[0]; ++i)
    eu.set_instruction(i->base, i->mask, make_pair(i->handler, data));
}
