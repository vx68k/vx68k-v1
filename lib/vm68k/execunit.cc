/* vx68k - Virtual X68000
   Copyright (C) 1998, 1999 Hypercore Software Design, Ltd.

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

#include <algorithm>

#include "debug.h"

using namespace vm68k;
using namespace std;

/* Dispatches for instructions.  */
void
exec_unit::dispatch(unsigned int op, execution_context *ec) const
{
  I(op < 0x10000);
  instruction[op](op, ec);
}

/* Sets an instruction to operation codes.  */
void
exec_unit::set_instruction(int code, int mask, insn_handler h)
{
  I (code >= 0);
  I (code < 0x10000);
  code &= ~mask;
  for (int i = code; i <= (code | mask); ++i)
    {
      if ((i & ~mask) == code)
	instruction[i] = h;
    }
}

exec_unit::exec_unit()
{
  fill(instruction + 0, instruction + 0x10000, &illegal);
  install_instructions(this);
}

/* Executes an illegal instruction.  */
void
exec_unit::illegal(unsigned int op, execution_context *)
{
  throw illegal_instruction();
}

namespace
{
  using namespace addressing;

  void addw_off_d(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int s_reg = op & 0x7;
      int d_reg = op >> 9 & 0x7;
      int s_off = extsw(ec->fetchw(2));
      uint32 s_addr = ec->regs.a[s_reg] + s_off;
      VL((" addw %%a%d@(%d),%%d%d |0x%lx,*\n",
	  s_reg, s_off, d_reg, (unsigned long) s_addr));

      int fc = ec->data_fc();
      int value1 = extsw(ec->regs.d[d_reg]);
      int value2 = extsw(ec->mem->getw(fc, s_addr));
      int value = extsw(value1 + value2);
      const uint32 MASK = ((uint32) 1u << 16) - 1;
      ec->regs.d[d_reg] = ec->regs.d[d_reg] & ~MASK | (uint32) value & MASK;
      ec->regs.sr.set_cc(value); // FIXME.

      ec->regs.pc += 2 + 2;
    }

  template <class Destination> void addal(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      Destination ea1(op & 0x7, 2);
      int reg2 = op >> 9 & 0x7;
      VL((" addal %s", ea1.textl(ec)));
      VL((",%%a%d\n", reg2));

      int32 value1 = ea1.getl(ec);
      int32 value2 = extsl(ec->regs.a[reg2]);
      int32 value = extsl(value2 + value1);
      ec->regs.a[reg2] = value;
      ea1.finishl(ec);
      // XXX: The condition codes are not affected.

      ec->regs.pc += 2 + ea1.isize(4);
    }

  template <class Destination> void addil(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int32 value2 = extsl(ec->fetchl(2));
      Destination ea1(op & 0x7, 2 + 4);
      VL((" addil #%ld,*\n", (long) value2));

      int32 value1 = ea1.getl(ec);
      int32 value = extsl(value1 + value2);
      ea1.putl(ec, value);
      ea1.finishl(ec);
      ec->regs.sr.set_cc(value); // FIXME.

      ec->regs.pc += 2 + 4;
    }

  template <> void addil<address_register>(unsigned int, execution_context *);
  // XXX: Address register cannot be the destination.

#if 0
  void addil_d(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int d_reg = op & 0x7;
      int32 value2 = extsl(ec->fetchl(2));
      VL((" addil #%ld,%%d%d\n", (long) value2, d_reg));

      int32 value1 = extsl(ec->regs.d[d_reg]);
      int32 value = extsl(value1 + value2);
      ec->regs.d[d_reg] = value;
      ec->regs.sr.set_cc(value); // FIXME.

      ec->regs.pc += 2 + 4;
    }
#endif /* 0 */

  template <class Destination> void addqb(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      Destination ea1(op & 0x7, 2);
      int value2 = op >> 9 & 0x7;
      if (value2 == 0)
	value2 = 8;
      VL((" addqb #%d,*\n", value2));

      int value1 = ea1.getb(ec);
      int value = extsb(value1 + value2);
      ea1.putb(ec, value);
      ea1.finishb(ec);
      ec->regs.sr.set_cc(value); // FIXME.

      ec->regs.pc += 2;
    }

#if 0
  void addqb_d(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int reg1 = op & 0x7;
      int val2 = op >> 9 & 0x7;
      if (val2 == 0)
	val2 = 8;
      VL((" addqb #%d,%%d%d\n", val2, reg1));

      int val1 = extsb(ec->regs.d[reg1]);
      int val = extsb(val1 + val2);
      const uint32 MASK = ((uint32) 1u << 8) - 1;
      ec->regs.d[reg1] = ec->regs.d[reg1] & ~MASK | (uint32) val & MASK;
      ec->regs.sr.set_cc(val); // FIXME.

      ec->regs.pc += 2;
    }
#endif /* 0 */

  template <class Destination> void addqw(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      Destination ea1(op & 0x7, 2);
      int value2 = op >> 9 & 0x7;
      if (value2 == 0)
	value2 = 8;
      VL((" addqw #%d,*\n", value2));

      int value1 = ea1.getw(ec);
      int value = extsw(value1 + value2);
      ea1.putw(ec, value);
      ea1.finishw(ec);
      ec->regs.sr.set_cc(value); // FIXME.

      ec->regs.pc += 2;
    }

  template <> void addqw<address_register>(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      address_register ea1(op & 0x7, 2);
      int value2 = op >> 9 & 0x7;
      if (value2 == 0)
	value2 = 8;
      VL((" addqw #%d,*\n", value2));

      // XXX: The entire register is used.
      int32 value1 = ea1.getl(ec);
      int32 value = extsl(value1 + value2);
      ea1.putl(ec, value);
      ea1.finishl(ec);
      // XXX: The condition codes are not affected.

      ec->regs.pc += 2;
    }

#if 0
  void addqw_a(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int value = op >> 9 & 0x7;
      int reg = op & 0x7;
      if (value == 0)
	value = 8;
      VL((" addqw #%d,%%a%d\n", value, reg));

      // XXX: The condition codes are not affected.
      ec->regs.a[reg] += value;

      ec->regs.pc += 2;
    }
#endif /* 0 */

  template <class Destination> void addql(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      Destination ea1(op & 0x7, 2);
      int value2 = op >> 9 & 0x7;
      if (value2 == 0)
	value2 = 8;
      VL((" addql #%d,*\n", value2));

      int32 value1 = ea1.getl(ec);
      int32 value = extsl(value1 + value2);
      ea1.putl(ec, value);
      ea1.finishl(ec);
      ec->regs.sr.set_cc(value); // FIXME.

      ec->regs.pc += 2;
    }

  template <> void addql<address_register>(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      address_register ea1(op & 0x7, 2);
      int value2 = op >> 9 & 0x7;
      if (value2 == 0)
	value2 = 8;
      VL((" addql #%d,*\n", value2));

      int32 value1 = ea1.getl(ec);
      int32 value = extsl(value1 + value2);
      ea1.putl(ec, value);
      ea1.finishl(ec);
      // XXX: The condition codes are not affected.

      ec->regs.pc += 2;
    }

#if 0
  void addql_d(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int reg1 = op & 0x7;
      int val2 = op >> 9 & 0x7;
      if (val2 == 0)
	val2 = 8;
      VL((" addql #%d,%%d%d\n", val2, reg1));

      int32 val1 = extsl(ec->regs.d[reg1]);
      int32 val = extsl(val1 + val2);
      ec->regs.d[reg1] = val;
      ec->regs.sr.set_cc(val); // FIXME.

      ec->regs.pc += 2;
    }
#endif /* 0 */

  void andl_i_d(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int reg1 = op >> 9 & 0x7;
      uint32 val2 = ec->fetchl(2);
      VL((" andl #0x%lx,%%d%d\n", (unsigned long) val2, reg1));

      uint32 val = ec->regs.d[reg1] & val2;
      ec->regs.d[reg1] = val;
      ec->regs.sr.set_cc(val);

      ec->regs.pc += 2 + 4;
    }

  void bcc(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int len = 2;
      int disp = op & 0xff;
      if (disp == 0)
	{
	  len = 4;
	  disp = extsw(ec->fetchw(2));
	}
      else
	disp = extsb(disp);
      VL((" bcc 0x%lx\n", (unsigned long) (ec->regs.pc + 2 + disp)));

      // XXX: The condition codes are not affected.
      ec->regs.pc += ec->regs.sr.cc() ? 2 + disp : len;
    }

  void beq(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int len = 2;
      int disp = op & 0xff;
      if (disp == 0)
	{
	  len = 4;
	  disp = extsw(ec->fetchw(2));
	}
      else
	disp = extsb(disp);
      VL((" beq 0x%lx\n", (unsigned long) (ec->regs.pc + 2 + disp)));

      // XXX: The condition codes are not affected.
      ec->regs.pc += ec->regs.sr.eq() ? 2 + disp : len;
    }

  void bge(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int len = 2;
      int disp = op & 0xff;
      if (disp == 0)
	{
	  len = 4;
	  disp = extsw(ec->fetchw(2));
	}
      else
	disp = extsb(disp);
      VL((" bge 0x%lx\n", (unsigned long) (ec->regs.pc + 2 + disp)));

      // XXX: The condition codes are not affected.
      ec->regs.pc += ec->regs.sr.ge() ? 2 + disp : len;
    }

  void bne(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int len = 2;
      int disp = op & 0xff;
      if (disp == 0)
	{
	  len = 4;
	  disp = extsw(ec->fetchw(2));
	}
      else
	disp = extsb(disp);
      VL((" bne 0x%lx\n", (unsigned long) (ec->regs.pc + 2 + disp)));

      // XXX: The condition codes are not affected.
      ec->regs.pc += ec->regs.sr.ne() ? 2 + disp : len;
    }

  void bra(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int len = 2;
      int disp = op & 0xff;
      if (disp == 0)
	{
	  len = 4;
	  disp = extsw(ec->fetchw(2));
	}
      else
	disp = extsb(disp);
      VL((" bra 0x%lx\n", (unsigned long) (ec->regs.pc + 2 + disp)));

      // XXX: The condition codes are not affected.
      ec->regs.pc += 2 + disp;
    }

  void bsr(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int len = 2;
      int disp = op & 0xff;
      if (disp == 0)
	{
	  len = 4;
	  disp = extsw(ec->fetchw(2));
	}
      else
	disp = extsb(disp);
      VL((" bsr 0x%lx\n", (unsigned long) (ec->regs.pc + 2 + disp)));

      // XXX: The condition codes are not affected.
      int fc = ec->data_fc();
      ec->mem->putl(fc, ec->regs.a[7] - 4, ec->regs.pc + len);
      ec->regs.a[7] -= 4;
      ec->regs.pc += 2 + disp;
    }

  template <class Destination> void clrw(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      Destination ea1(op & 0x7, 2);
      VL((" clrw *\n"));

      ea1.putw(ec, 0);
      ea1.finishw(ec);
      ec->regs.sr.set_cc(0);

      ec->regs.pc += 2;
    }

  template <> void clrw<address_register>(unsigned int, execution_context *);
  // XXX: Address register cannot be the destination.

#if 0
  void clrw_predec(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int reg = op & 0x7;
      uint32 d_addr = ec->regs.a[reg] - 2;
      VL((" clrw %%a%d@- |0x%lx\n", reg, (unsigned long) d_addr));

      int fc = ec->data_fc();
      ec->mem->putw(fc, d_addr, 0);
      ec->regs.a[reg] = d_addr;
      ec->regs.sr.set_cc(0);

      ec->regs.pc += 2;
    }
#endif /* 0 */

  template <class Destination> void cmpl(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      Destination ea1(op & 0x7, 2);
      int reg2 = op >> 9 & 0x7;
      VL((" cmpl %s", ea1.textl(ec)));
      VL((",%%d%d\n", reg2));

      int32 value1 = ea1.getl(ec);
      int32 value2 = extsl(ec->regs.d[reg2]);
      int32 value = extsl(value2 - value1);
      ea1.finishl(ec);
      ec->regs.sr.set_cc(value); // FIXME.

      ec->regs.pc += 2 + ea1.isize(4);
    }

  template <class Destination> void cmpib(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int value2 = extsb(ec->fetchw(2));
      Destination ea1(op & 0x7, 2 + 2);
      VL((" cmpib #0x%x", (unsigned int) value2));
      VL((",%s\n", ea1.textb(ec)));

      int value1 = ea1.getb(ec);
      int value = extsb(value1 - value2);
      ea1.finishb(ec);
      ec->regs.sr.set_cc(value); // FIXME.

      ec->regs.pc += 2 + 2;
    }

#if 0
  void cmpib_postinc(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int reg1 = op & 0x7;
      int val2 = extsb(ec->fetchw(2));
      uint32 addr1 = ec->regs.a[reg1];
      VL((" cmpib #%d,%%a%d@+ |*,0x%lx\n",
	  val2, reg1, (unsigned long) addr1));

      int fc = ec->data_fc();
      int val1 = extsb(ec->mem->getb(fc, addr1));
      int val = extsb(val1 - val2);
      ec->regs.a[reg1] = addr1 + 1;
      ec->regs.sr.set_cc(val);	// FIXME.

      ec->regs.pc += 2 + 2;
    }
#endif /* 0 */

  void dbf_d(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int reg = op & 0x7;
      int disp = extsw(ec->fetchw(2));
      VL((" dbf %%d%d,0x%lx\n",
	  reg, (unsigned long) (ec->regs.pc + 2 + disp)));

      // XXX: The condition codes are not affected.
      int32 value = extsl(ec->regs.d[reg]) - 1;
      ec->regs.d[reg] = value;
      ec->regs.pc += value != -1 ? 2 + disp : 2 + 2;
    }

  template <class Source> void lea(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      Source ea1(op & 0x7, 2);
      int reg2 = op >> 9 & 0x7;
      VL((" lea %s", ea1.textw(ec)));
      VL((",%%a%d\n", reg2));

      // XXX: The condition codes are not affected.
      uint32 address = ea1.address(ec);
      ec->regs.a[reg2] = address;

      ec->regs.pc += 2 + ea1.isize(2);
    }

  void lea_offset_a(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int s_reg = op & 0x7;
      int d_reg = op >> 9 & 0x7;
      int offset = extsw(ec->fetchw(2));
      VL((" lea %%a%d@(%ld),%%a%d\n", s_reg, (long) offset, d_reg));

      // XXX: The condition codes are not affected.
      ec->regs.a[d_reg] = ec->regs.a[s_reg] + offset;

      ec->regs.pc += 4;
    }

#if 0
  void lea_absl_a(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int reg = op >> 9 & 0x7;
      uint32 address = ec->fetchl(2);
      VL((" lea 0x%lx:l,%%a%d\n", (unsigned long) address, reg));

      // XXX: The condition codes are not affected.
      ec->regs.a[reg] = address;

      ec->regs.pc += 6;
    }
#endif /* 0 */

  void link_a(unsigned int op, execution_context *ec)
    {
      int reg = op & 0x0007;
      int disp = extsw(ec->fetchw(2));
      VL((" link %%a%d,#%d\n", reg, disp));

      // XXX: The condition codes are not affected.
      int fc = ec->data_fc();
      ec->mem->putl(fc, ec->regs.a[7] - 4, ec->regs.a[reg]);
      ec->regs.a[7] -= 4;
      ec->regs.a[reg] = ec->regs.a[7];
      ec->regs.a[7] += disp;

      ec->regs.pc += 4;
    }

  void lslw_i_d(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int reg1 = op & 0x7;
      int val2 = op >> 9 & 0x7;
      if (val2 == 0)
	val2 = 8;
      VL((" lslw #%d,%%d%d\n", val2, reg1));

      unsigned int val = ec->regs.d[reg1] << val2;
      const uint32 MASK = ((uint32) 1u << 16) - 1;
      ec->regs.d[reg1] = ec->regs.d[reg1] & ~MASK | (uint32) val & MASK;
      ec->regs.sr.set_cc(val);	// FIXME.

      ec->regs.pc += 2;
    }

  void lsll_i_d(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int reg1 = op & 0x7;
      int val2 = op >> 9 & 0x7;
      if (val2 == 0)
	val2 = 8;
      VL((" lsll #%d,%%d%d\n", val2, reg1));

      uint32 val = ec->regs.d[reg1] << val2;
      ec->regs.d[reg1] = val;
      ec->regs.sr.set_cc(val);	// FIXME.

      ec->regs.pc += 2;
    }

  void lsrw_i_d(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int d_reg = op & 0x7;
      int count = op >> 9 & 0x7;
      if (count == 0)
	count = 8;
      VL((" lsrw #%d,%%d%d\n", count, d_reg));

      unsigned int value = ec->regs.d[d_reg] >> count;
      const uint32 MASK = ((uint32) 1u << 16) - 1;
      ec->regs.d[d_reg] = ec->regs.d[d_reg] & ~MASK | (uint32) value & MASK;
      ec->regs.sr.set_cc(value); // FIXME.

      ec->regs.pc += 2;
    }

  void lsrl_i(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int reg1 = op & 0x7;
      int value2 = op >> 9 & 0x7;
      if (value2 == 0)
	value2 = 8;
      VL((" lsrl #%d", value2));
      VL((",%%d%d\n", reg1));

      uint32 value1 = ec->regs.d[reg1];
      uint32 value = value1 >> value2;
      ec->regs.d[reg1] = value;
      ec->regs.sr.set_cc(value); // FIXME.

      ec->regs.pc += 2;
    }

  template <class Source, class Destination>
    void moveb(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      Source ea1(op & 0x7, 2);
      Destination ea2(op >> 9 & 0x7, 2 + ea1.isize(2));
      VL((" moveb %s", ea1.textb(ec)));
      VL((",%s\n", ea2.textb(ec)));

      int value = ea1.getb(ec);
      ea2.putb(ec, value);
      ea1.finishb(ec);
      ea2.finishb(ec);
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2 + ea1.isize(2) + ea2.isize(2);
    }

  void moveb_indir_d(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int s_reg = op & 0x7;
      int d_reg = op >> 9 & 0x7;
      uint32 s_addr = ec->regs.a[s_reg];
      VL((" moveb %%a%d@,%%d%d |0x%lx,*\n",
	  s_reg, d_reg, (unsigned long) s_addr));

      int fc = ec->data_fc();
      int val = extsb(ec->mem->getb(fc, s_addr));
      const uint32 MASK = ((uint32) 1u << 8) - 1;
      ec->regs.d[d_reg] = ec->regs.d[d_reg] & ~MASK | (uint32) val & MASK;
      ec->regs.sr.set_cc(val);

      ec->regs.pc += 2;
    }

  void moveb_postinc_d(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int s_reg = op & 0x7;
      int d_reg = op >> 9 & 0x7;
      uint32 s_addr = ec->regs.a[s_reg];
      VL((" moveb %%a%d@+,%%d%d |0x%lx,*\n",
	  s_reg, d_reg, (unsigned long) s_addr));

      int fc = ec->data_fc();
      int val = extsb(ec->mem->getb(fc, s_addr));
      const uint32 MASK = ((uint32) 1u << 8) - 1;
      ec->regs.d[d_reg] = ec->regs.d[d_reg] & ~MASK | (uint32) val & MASK;
      ec->regs.a[s_reg] = s_addr + 1;
      ec->regs.sr.set_cc(val);

      ec->regs.pc += 2;
    }

  void moveb_off_d(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int s_reg = op & 0x7;
      int d_reg = op >> 9 & 0x7;
      int s_off = extsw(ec->fetchw(2));
      uint32 s_addr = ec->regs.a[s_reg] + s_off;
      VL((" moveb %%a%d@(%d),%%d%d |0x%lx,*\n",
	  s_reg, s_off, d_reg, (unsigned long) s_addr));

      int fc = ec->data_fc();
      int val = extsb(ec->mem->getb(fc, s_addr));
      const uint32 MASK = ((uint32) 1u << 8) - 1;
      ec->regs.d[d_reg] = ec->regs.d[d_reg] & ~MASK | (uint32) val & MASK;
      ec->regs.sr.set_cc(val);

      ec->regs.pc += 2 + 2;
    }

  void moveb_d_postinc(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int s_reg = op & 0x7;
      int d_reg = op >> 9 & 0x7;
      uint32 d_addr = ec->regs.a[d_reg];
      VL((" moveb %%d%d,%%a%d@+ |*,0x%lx\n",
	  s_reg, d_reg, (unsigned long) d_addr));

      int fc = ec->data_fc();
      int val = extsb(ec->regs.d[s_reg]);
      ec->mem->putb(fc, d_addr, val);
      ec->regs.a[d_reg] = d_addr + 1;
      ec->regs.sr.set_cc(val);

      ec->regs.pc += 2;
    }

  void moveb_postinc_postinc(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int s_reg = op & 0x7;
      uint32 s_addr = ec->regs.a[s_reg];
      int d_reg = op >> 9 & 0x7;
      uint32 d_addr = ec->regs.a[d_reg];
      VL((" moveb %%a%d@+,%%a%d@+ |0x%lx,0x%lx\n",
	  s_reg, d_reg, (unsigned long) s_addr, (unsigned long) d_addr));

      int fc = ec->data_fc();
      int value = extsb(ec->mem->getb(fc, s_addr));
      ec->mem->putb(fc, d_addr, value);
      ec->regs.a[s_reg] = s_addr + 1;
      ec->regs.a[d_reg] = d_addr + 1;
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2;
    }

  template <class Source, class Destination>
    void movew(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      Source ea1(op & 0x7, 2);
      Destination ea2(op >> 9 & 0x7, 2 + ea1.isize(2));
      VL((" movew %s", ea1.textw(ec)));
      VL((",%s\n", ea2.textw(ec)));

      int value = ea1.getw(ec);
      ea2.putw(ec, value);
      ea1.finishw(ec);
      ea2.finishw(ec);
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2 + ea1.isize(2) + ea2.isize(2);
    }

  void movew_off_d(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int s_reg = op & 0x7;
      int d_reg = op >> 9 & 0x7;
      int s_off = extsw(ec->fetchw(2));
      uint32 s_addr = ec->regs.a[s_reg] + s_off;
      VL((" movew %%a%d@(%d),%%d%d |0x%lx,*\n",
	  s_reg, s_off, d_reg, (unsigned long) s_addr));

      int fc = ec->data_fc();
      int value = extsw(ec->mem->getw(fc, s_addr));
      const uint32 MASK = ((uint32) 1u << 16) - 1;
      ec->regs.d[d_reg] = ec->regs.d[d_reg] & ~MASK | (uint32) value & MASK;
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2 + 2;
    }

#if 0
  void movew_absl_d(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int d_reg = op >> 9 & 0x7;
      uint32 address = ec->fetchl(2);
      VL((" movew 0x%lx,%%d%d\n", (unsigned long) address, d_reg));

      int fc = ec->data_fc();
      int value = extsw(ec->mem->getw(fc, address));
      const uint32 MASK = ((uint32) 1u << 16) - 1;
      ec->regs.d[d_reg] = ec->regs.d[d_reg] & ~MASK | (uint32) value & MASK;
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2 + 4;
    }
#endif /* 0 */

  void movew_d_postinc(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int s_reg = op & 0x7;
      int d_reg = op >> 9 & 0x7;
      uint32 d_addr = ec->regs.a[d_reg];
      VL((" movew %%d%d,%%a%d@+ |*,0x%lx\n",
	  s_reg, d_reg, (unsigned long) d_addr));

      int fc = ec->data_fc();
      int val = extsw(ec->regs.d[s_reg]);
      ec->mem->putw(fc, d_addr, val);
      ec->regs.a[d_reg] = d_addr + 2;
      ec->regs.sr.set_cc(val);

      ec->regs.pc += 2;
    }

  void movew_d_predec(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int s_reg = op & 0x7;
      int d_reg = op >> 9 & 0x7;
      uint32 d_addr = ec->regs.a[d_reg] - 2;
      VL((" movew %%d%d,%%a%x@- |*,0x%lx\n",
	  s_reg, d_reg, (unsigned long) d_addr));

      int fc = ec->data_fc();
      int value = extsw(ec->regs.d[s_reg]);
      ec->mem->putw(fc, d_addr, value);
      ec->regs.a[d_reg] = d_addr;
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2;
    }

  void movew_absl_predec(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int d_reg = op >> 9 & 0x7;
      uint32 d_addr = ec->regs.a[d_reg] - 2;
      uint32 s_addr = ec->fetchl(2);
      VL((" movew 0x%lx,%%a%x@- |*,0x%lx\n",
	  (unsigned long) s_addr, d_reg, (unsigned long) d_addr));

      int fc = ec->data_fc();
      int value = extsw(ec->mem->getw(fc, s_addr));
      ec->mem->putw(fc, d_addr, value);
      ec->regs.a[d_reg] = d_addr;
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2 + 4;
    }

  void movew_d_absl(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int reg = op & 0x7;
      uint32 address = ec->fetchl(2);
      VL((" movew %%d%d,0x%x\n", reg, address));

      int fc = ec->data_fc();
      int value = extsw(ec->regs.d[reg]);
      ec->mem->putw(fc, address, value);
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2 + 4;
    }

  template <class Source, class Destination>
    void movel(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      Source ea1(op & 0x7, 2);
      Destination ea2(op >> 9 & 0x7, 2 + ea1.isize(4));
      VL((" movel %s", ea1.textl(ec)));
      VL((",%s\n", ea2.textl(ec)));

      int32 value = ea1.getl(ec);
      ea2.putl(ec, value);
      ea1.finishl(ec);
      ea2.finishl(ec);
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2 + ea1.isize(4) + ea2.isize(4);
    }

#if 0
  // This did not work with egcs 1.1.2.
  template <class Source>
    void movel<Source, address_register>(unsigned int op, execution_context *ec);
#endif

#if 0
  void movel_d_d(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int s_reg = op & 0x7;
      int d_reg = op >> 9 & 0x7;
      VL((" movel %%d%d,%%d%d\n", s_reg, d_reg));

      int32 value = extsl(ec->regs.d[s_reg]);
      ec->regs.d[d_reg] = value;
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2;
    }

  void movel_a_d(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int s_reg = op & 0x7;
      int d_reg = op >> 9 & 0x7;
      VL((" movel %%a%d,%%d%d\n", s_reg, d_reg));

      int32 value = extsl(ec->regs.a[s_reg]);
      ec->regs.d[d_reg] = value;
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2;
    }

  void movel_postinc_d(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int reg1 = op & 0x7;
      int reg2 = op >> 9 & 0x7;
      uint32 addr1 = ec->regs.a[reg1];
      VL((" movel %%a%d@+,%%d%d |0x%lx,*\n",
	  reg1, reg2, (unsigned long) addr1));

      int fc = ec->data_fc();
      int32 val = extsl(ec->mem->getl(fc, addr1));
      ec->regs.d[reg2] = val;
      ec->regs.a[reg1] = addr1 + 4;
      ec->regs.sr.set_cc(val);

      ec->regs.pc += 2;
    }
#endif /* 0 */

  void movel_off_d(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int s_reg = op & 0x7;
      int d_reg = op >> 9 & 0x7;
      int s_off = extsw(ec->fetchw(2));
      uint32 s_addr = ec->regs.a[s_reg] + s_off;
      VL((" movel %%a%d@(%d),%%d%d |0x%lx,*\n",
	  s_reg, s_off, d_reg, (unsigned long) s_addr));

      int fc = ec->data_fc();
      int32 value = extsl(ec->mem->getl(fc, s_addr));
      ec->regs.d[d_reg] = value;
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2 + 2;
    }

  void movel_d_postinc(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int s_reg = op & 0x7;
      int d_reg = op >> 9 & 0x7;
      uint32 d_addr = ec->regs.a[d_reg];
      VL((" movel %%d%d,%%a%d@+ |*,0x%lx\n",
	  s_reg, d_reg, (unsigned long) d_addr));

      int fc = ec->data_fc();
      int32 value = extsl(ec->regs.d[s_reg]);
      ec->mem->putl(fc, d_addr, value);
      ec->regs.a[d_reg] = d_addr + 4;
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2;
    }

  void movel_d_predec(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int s_reg = op & 0x7;
      int d_reg = op >> 9 & 0x7;
      uint32 d_addr = ec->regs.a[d_reg] - 4;
      VL((" movel %%d%d,%%a%d@- |*,0x%lx\n",
	  s_reg, d_reg, (unsigned long) d_addr));

      int fc = ec->data_fc();
      int32 value = extsl(ec->regs.d[s_reg]);
      ec->mem->putl(fc, d_addr, value);
      ec->regs.a[d_reg] = d_addr;
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2;
    }

  void movel_a_predec(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int s_reg = op & 0x7;
      int d_reg = op >> 9 & 0x7;
      uint32 d_addr = ec->regs.a[d_reg] - 4;
      VL((" movel %%a%d,%%a%d@- |*,0x%lx\n",
	  s_reg, d_reg, (unsigned long) d_addr));

      int fc = ec->data_fc();
      int32 value = extsl(ec->regs.a[s_reg]);
      ec->mem->putl(fc, d_addr, value);
      ec->regs.a[d_reg] = d_addr;
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2;
    }

  void movel_postinc_a(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int s_reg = op & 0x7;
      uint32 s_addr = ec->regs.a[s_reg];
      int d_reg = op >> 9 & 0x7;
      VL((" movel %%a%d@+,%%a%d |0x%lx,*\n",
	  s_reg, d_reg, (unsigned long) s_addr));

      // XXX: The condition codes are not affected.
      int fc = ec->data_fc();
      ec->regs.a[d_reg] = ec->mem->getl(fc, s_addr);
      ec->regs.a[s_reg] = s_addr + 4;

      ec->regs.pc += 2;
    }

  void movel_i_predec(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int d_reg = op >> 9 & 0x7;
      uint32 d_addr = ec->regs.a[d_reg] - 4;
      int32 value = extsl(ec->fetchl(2));
      VL((" movel #%ld,%%a%d@- |*,0x%lx\n",
	  (long) value, d_reg, (unsigned long) d_addr));

      int fc = ec->data_fc();
      ec->mem->putl(fc, d_addr, value);
      ec->regs.a[d_reg] = d_addr;
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2 + 4;
    }

  void movel_d_absl(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int s_reg = op & 0x7;
      uint32 d_addr = ec->fetchl(2);
      VL((" movel %%d%d,0x%x\n", s_reg, d_addr));

      int fc = ec->data_fc();
      int32 value = extsl(ec->regs.d[s_reg]);
      ec->mem->putw(fc, d_addr, value);
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2 + 4;
    }

  void movel_a_absl(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int s_reg = op & 0x7;
      uint32 d_addr = ec->fetchl(2);
      VL((" movel %%a%d,0x%x\n", s_reg, d_addr));

      int fc = ec->data_fc();
      int32 value = extsl(ec->regs.a[s_reg]);
      ec->mem->putw(fc, d_addr, value);
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2 + 4;
    }

  /* movem regs to EA (postdec).  */
  void moveml_r_predec(unsigned int op, execution_context *ec)
    {
      int reg = op & 0x0007;
      unsigned int bitmap = ec->fetchw(2);
      VL((" moveml #0x%x,%%a%d@-\n", bitmap, reg));

      // XXX: The condition codes are not affected.
      uint32 address = ec->regs.a[reg];
      int fc = ec->data_fc();
      for (uint32 *i = ec->regs.a + 8; i != ec->regs.a + 0; --i)
	{
	  if (bitmap & 1 != 0)
	    {
	      ec->mem->putl(fc, address - 4, *(i - 1));
	      address -= 4;
	    }
	  bitmap >>= 1;
	}
      for (uint32 *i = ec->regs.d + 8; i != ec->regs.d + 0; --i)
	{
	  if (bitmap & 1 != 0)
	    {
	      ec->mem->putl(fc, address - 4, *(i - 1));
	      address -= 4;
	    }
	  bitmap >>= 1;
	}
      ec->regs.a[reg] = address;

      ec->regs.pc += 2 + 2;
    }

  void moveml_postinc_r(unsigned int op, execution_context *ec)
    {
      int reg = op & 0x0007;
      unsigned int bitmap = ec->fetchw(2);
      VL((" moveml %%a%d@+,#0x%x\n", reg, bitmap));

      // XXX: The condition codes are not affected.
      uint32 address = ec->regs.a[reg];
      int fc = ec->data_fc();
      for (uint32 *i = ec->regs.d + 0; i != ec->regs.d + 8; ++i)
	{
	  if (bitmap & 1 != 0)
	    {
	      *i = ec->mem->getl(fc, address);
	      address += 4;
	    }
	  bitmap >>= 1;
	}
      for (uint32 *i = ec->regs.a + 0; i != ec->regs.a + 8; ++i)
	{
	  if (bitmap & 1 != 0)
	    {
	      *i = ec->mem->getl(fc, address);
	      address += 4;
	    }
	  bitmap >>= 1;
	}
      ec->regs.a[reg] = address;

      ec->regs.pc += 2 + 2;
    }

  void moveql_d(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int value = extsb(op & 0xff);
      int reg = op >> 9 & 0x7;
      VL((" moveql #%d,%%d%d\n", value, reg));
      
      ec->regs.d[reg] = value;
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2;
    }

  void pea_absl(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      uint32 address = ec->fetchl(2);
      VL((" pea 0x%lx:l\n", (unsigned long) address));

      // XXX: The condition codes are not affected.
      int fc = ec->data_fc();
      ec->mem->putl(fc, ec->regs.a[7] - 4, address);
      ec->regs.a[7] -= 4;

      ec->regs.pc += 2 + 4;
    }

  void rts(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      VL((" rts\n"));

      // XXX: The condition codes are not affected.
      int fc = ec->data_fc();
      uint32 value = ec->mem->getl(fc, ec->regs.a[7]);
      ec->regs.a[7] += 4;
      ec->regs.pc = value;
    }

  void subb_postinc_d(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int reg1 = op & 0x7;
      int reg2 = op >> 9 & 0x7;
      uint32 addr1 = ec->regs.a[reg1];
      VL((" subb %%a%d@+,%%d%d |0x%lx,*\n",
	  reg1, reg2, (unsigned long) addr1));

      int fc = ec->data_fc();
      int val1 = extsb(ec->mem->getb(fc, addr1));
      int val2 = extsb(ec->regs.d[reg2]);
      int val = extsb(val2 - val1);
      const uint32 MASK = ((uint32) 1u << 8) - 1;
      ec->regs.d[reg2] = ec->regs.d[reg2] & ~MASK | (uint32) val & MASK;
      ec->regs.a[reg1] = addr1 + 1;
      ec->regs.sr.set_cc(val);	// FIXME.

      ec->regs.pc += 2;
    }

  template <class Destination> void subl(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      Destination ea1(op & 0x7, 2);
      int reg2 = op >> 9 & 0x7;
      VL((" subl %s", ea1.textl(ec)));
      VL((",%%d%d\n", reg2));

      int32 value1 = ea1.getl(ec);
      int32 value2 = extsl(ec->regs.d[reg2]);
      int32 value = extsl(value2 - value1);
      ec->regs.d[reg2] = value;
      ea1.finishl(ec);
      ec->regs.sr.set_cc(value); // FIXME.

      ec->regs.pc += 2 + ea1.isize(4);
    }

  void subql_d(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int reg1 = op & 0x7;
      int val2 = op >> 9 & 0x7;
      if (val2 == 0)
	val2 = 8;
      VL((" subql #%d,%%d%d\n", val2, reg1));

      int32 val1 = extsl(ec->regs.d[reg1]);
      int32 val = extsl(val1 - val2);
      ec->regs.d[reg1] = val;
      ec->regs.sr.set_cc(val); // FIXME.

      ec->regs.pc += 2;
    }

  void subql_a(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int value = op >> 9 & 0x7;
      int reg = op & 0x7;
      if (value == 0)
	value = 8;
      VL((" subql #%d,%%a%d\n", value, reg));

      // XXX: The condition codes are not affected.
      ec->regs.a[reg] -= value;

      ec->regs.pc += 2;
    }

  void tstb_d(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int reg = op & 0x7;
      VL((" tstb %%d%d\n", reg));

      int val = extsb(ec->regs.d[reg]);
      ec->regs.sr.set_cc(val);

      ec->regs.pc += 2;
    }

  void tstb_predec(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int reg = op & 0x7;
      uint32 addr = ec->regs.a[reg] - 1;
      VL((" tstb %%a%d@- |0x%lx\n", reg, (unsigned long) addr));

      int fc = ec->data_fc();
      int val = extsb(ec->mem->getb(fc, addr));
      ec->regs.a[reg] = addr;
      ec->regs.sr.set_cc(val);

      ec->regs.pc += 2;
    }

  void tstw_d(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int reg = op & 0x7;
      VL((" tstw %%d%d\n", reg));

      int value = extsw(ec->regs.d[reg]);
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2;
    }

  void tstl_d(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      int s_reg = op & 0x7;
      VL((" tstl %%d%d\n", s_reg));

      int value = extsl(ec->regs.d[s_reg]);
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2;
    }

  void unlk_a(unsigned int op, execution_context *ec)
    {
      int reg = op & 0x0007;
      VL((" unlk %%a%d\n", reg));

      // XXX: The condition codes are not affected.
      int fc = ec->data_fc();
      uint32 address = ec->mem->getl(fc, ec->regs.a[reg]);
      ec->regs.a[7] = ec->regs.a[reg] + 4;
      ec->regs.a[reg] = address;

      ec->regs.pc += 2;
    }
} // (unnamed namespace)

/* Installs instructions into the execution unit.  */
void
exec_unit::install_instructions(exec_unit *eu)
{
  I(eu != NULL);
  eu->set_instruction(0x0680, 0x0007, &addil<data_register>);
  eu->set_instruction(0x0690, 0x0007, &addil<indirect>);
  eu->set_instruction(0x0698, 0x0007, &addil<postincrement_indirect>);
  eu->set_instruction(0x06a0, 0x0007, &addil<predecrement_indirect>);
  eu->set_instruction(0x0c00, 0x0007, &cmpib<data_register>);
  eu->set_instruction(0x0c10, 0x0007, &cmpib<indirect>);
  eu->set_instruction(0x0c18, 0x0007, &cmpib<postincrement_indirect>);
  eu->set_instruction(0x0c20, 0x0007, &cmpib<predecrement_indirect>);
  eu->set_instruction(0x1010, 0x0e07, &moveb_indir_d);
  eu->set_instruction(0x1018, 0x0e07, &moveb_postinc_d);
  eu->set_instruction(0x1028, 0x0e07, &moveb_off_d);
  eu->set_instruction(0x10c0, 0x0e07, &moveb_d_postinc);
  eu->set_instruction(0x10d8, 0x0e07, &moveb_postinc_postinc);
  eu->set_instruction(0x13c0, 0x0007, &moveb<data_register, absolute_long>);
  eu->set_instruction(0x2000, 0x0e07, &movel<data_register, data_register>);
  eu->set_instruction(0x2008, 0x0e07, &movel<address_register, data_register>);
  eu->set_instruction(0x2010, 0x0e07, &movel<indirect, data_register>);
  eu->set_instruction(0x2018, 0x0e07, &movel<postincrement_indirect, data_register>);
  eu->set_instruction(0x2020, 0x0e07, &movel<predecrement_indirect, data_register>);
  eu->set_instruction(0x2028, 0x0e07, &movel_off_d);
  eu->set_instruction(0x2039, 0x0e00, &movel<absolute_long, data_register>);
  eu->set_instruction(0x203c, 0x0e00, &movel<immediate, data_register>);
  eu->set_instruction(0x2058, 0x0e07, &movel_postinc_a);
  eu->set_instruction(0x20c0, 0x0e07, &movel_d_postinc);
  eu->set_instruction(0x2100, 0x0e07, &movel_d_predec);
  eu->set_instruction(0x2108, 0x0e07, &movel_a_predec);
  eu->set_instruction(0x213c, 0x0e00, &movel_i_predec);
  eu->set_instruction(0x23c0, 0x0007, &movel_d_absl);
  eu->set_instruction(0x23c8, 0x0007, &movel_a_absl);
  eu->set_instruction(0x23fc, 0x0000, &movel<immediate, absolute_long>);
  eu->set_instruction(0x3000, 0x0e07, &movew<data_register, data_register>);
  eu->set_instruction(0x3008, 0x0e07, &movew<address_register, data_register>);
  eu->set_instruction(0x3010, 0x0e07, &movew<indirect, data_register>);
  eu->set_instruction(0x3018, 0x0e07, &movew<postincrement_indirect, data_register>);
  eu->set_instruction(0x3020, 0x0e07, &movew<predecrement_indirect, data_register>);
  eu->set_instruction(0x3028, 0x0e07, &movew_off_d);
  eu->set_instruction(0x3039, 0x0e00, &movew<absolute_long, data_register>);
  eu->set_instruction(0x303c, 0x0e00, &movew<immediate, data_register>);
  eu->set_instruction(0x30c0, 0x0e07, &movew_d_postinc);
  eu->set_instruction(0x3100, 0x0e07, &movew_d_predec);
  eu->set_instruction(0x3139, 0x0e00, &movew_absl_predec);
  eu->set_instruction(0x33c0, 0x0007, &movew_d_absl);
  eu->set_instruction(0x4190, 0x0e07, &lea<indirect>);
  eu->set_instruction(0x41e8, 0x0e07, &lea_offset_a);
  eu->set_instruction(0x41f9, 0x0e00, &lea<absolute_long>);
  eu->set_instruction(0x4240, 0x0007, &clrw<data_register>);
  eu->set_instruction(0x4250, 0x0007, &clrw<indirect>);
  eu->set_instruction(0x4258, 0x0007, &clrw<postincrement_indirect>);
  eu->set_instruction(0x4260, 0x0007, &clrw<predecrement_indirect>);
  eu->set_instruction(0x4879, 0x0000, &pea_absl);
  eu->set_instruction(0x48e0, 0x0007, &moveml_r_predec);
  eu->set_instruction(0x4cd8, 0x0007, &moveml_postinc_r);
  eu->set_instruction(0x4a00, 0x0007, &tstb_d);
  eu->set_instruction(0x4a20, 0x0007, &tstb_predec);
  eu->set_instruction(0x4a40, 0x0007, &tstw_d);
  eu->set_instruction(0x4a80, 0x0007, &tstl_d);
  eu->set_instruction(0x4e50, 0x0007, &link_a);
  eu->set_instruction(0x4e58, 0x0007, &unlk_a);
  eu->set_instruction(0x4e75, 0x0000, &rts);
  eu->set_instruction(0x5000, 0x0e07, &addqb<data_register>);
  eu->set_instruction(0x5010, 0x0e07, &addqb<indirect>);
  eu->set_instruction(0x5018, 0x0e07, &addqb<postincrement_indirect>);
  eu->set_instruction(0x5020, 0x0e07, &addqb<predecrement_indirect>);
  eu->set_instruction(0x5040, 0x0e07, &addqw<data_register>);
  eu->set_instruction(0x5048, 0x0e07, &addqw<address_register>);
  eu->set_instruction(0x5050, 0x0e07, &addqw<indirect>);
  eu->set_instruction(0x5058, 0x0e07, &addqw<postincrement_indirect>);
  eu->set_instruction(0x5060, 0x0e07, &addqw<predecrement_indirect>);
  eu->set_instruction(0x5080, 0x0e07, &addql<data_register>);
  eu->set_instruction(0x5088, 0x0e07, &addql<address_register>);
  eu->set_instruction(0x5090, 0x0e07, &addql<indirect>);
  eu->set_instruction(0x5098, 0x0e07, &addql<postincrement_indirect>);
  eu->set_instruction(0x50a0, 0x0e07, &addql<predecrement_indirect>);
  eu->set_instruction(0x5180, 0x0e07, &subql_d);
  eu->set_instruction(0x5188, 0x0e07, &subql_a);
  eu->set_instruction(0x51c8, 0x0007, &dbf_d);
  eu->set_instruction(0x6000, 0x00ff, &bra);
  eu->set_instruction(0x6100, 0x00ff, &bsr);
  eu->set_instruction(0x6400, 0x00ff, &bcc);
  eu->set_instruction(0x6600, 0x00ff, &bne);
  eu->set_instruction(0x6700, 0x00ff, &beq);
  eu->set_instruction(0x6c00, 0x00ff, &bge);
  eu->set_instruction(0x7000, 0x0eff, &moveql_d);
  eu->set_instruction(0x9018, 0x0e07, &subb_postinc_d);
  eu->set_instruction(0x9080, 0x0e07, &subl<data_register>);
  eu->set_instruction(0x9088, 0x0e07, &subl<address_register>);
  eu->set_instruction(0x9090, 0x0e07, &subl<indirect>);
  eu->set_instruction(0x9098, 0x0e07, &subl<postincrement_indirect>);
  eu->set_instruction(0x90a0, 0x0e07, &subl<predecrement_indirect>);
  eu->set_instruction(0x90b9, 0x0e00, &subl<absolute_long>);
  eu->set_instruction(0x90bc, 0x0e00, &subl<immediate>);
  eu->set_instruction(0xb080, 0x0e07, &cmpl<data_register>);
  eu->set_instruction(0xb088, 0x0e07, &cmpl<address_register>);
  eu->set_instruction(0xb090, 0x0e07, &cmpl<indirect>);
  eu->set_instruction(0xb098, 0x0e07, &cmpl<postincrement_indirect>);
  eu->set_instruction(0xb0a0, 0x0e07, &cmpl<predecrement_indirect>);
  eu->set_instruction(0xb0b9, 0x0e00, &cmpl<absolute_long>);
  eu->set_instruction(0xc0bc, 0x0e00, &andl_i_d);
  eu->set_instruction(0xd068, 0x0e07, &addw_off_d);
  eu->set_instruction(0xd1c0, 0x0e07, &addal<data_register>);
  eu->set_instruction(0xd1c8, 0x0e07, &addal<address_register>);
  eu->set_instruction(0xd1d0, 0x0e07, &addal<indirect>);
  eu->set_instruction(0xd1d8, 0x0e07, &addal<postincrement_indirect>);
  eu->set_instruction(0xd1e0, 0x0e07, &addal<predecrement_indirect>);
  eu->set_instruction(0xd1f9, 0x0e07, &addal<absolute_long>);
  eu->set_instruction(0xd1fc, 0x0e07, &addal<immediate>);
  eu->set_instruction(0xe048, 0x0e07, &lsrw_i_d);
  eu->set_instruction(0xe088, 0x0e07, &lsrl_i);
  eu->set_instruction(0xe148, 0x0e07, &lslw_i_d);
  eu->set_instruction(0xe188, 0x0e07, &lsll_i_d);
}

