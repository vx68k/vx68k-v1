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
#include <vm68k/condition.h>
#include <vm68k/cpu.h>

#include <algorithm>

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
using vm68k::context;
using vm68k::byte_size;
using vm68k::word_size;
using vm68k::long_word_size;
using vm68k::privilege_violation_exception;
using namespace vm68k::types;
using namespace std;

#ifdef HAVE_NANA_H
bool nana_instruction_trace = false;
#endif

void
exec_unit::run(context &c) const
{
  for (;;)
    {
      if (c.interrupted())
	c.handle_interrupts();

#ifdef HAVE_NANA_H
# ifdef DUMP_REGISTERS
      for (unsigned int i = 0; i != 8; ++i)
	{
	  LG(nana_instruction_trace,
	     "| %%d%u = 0x%08lx, %%a%u = 0x%08lx\n",
	     i, (unsigned long) c.regs.d[i],
	     i, (unsigned long) c.regs.a[i]);
	}
# endif
      LG(nana_instruction_trace, "| 0x%08lx (0x%04x)\n",
	 long_word_size::uvalue(c.regs.pc) + 0UL, c.ufetch(word_size(), 0));
#endif
      step(c);
    }
}

void
exec_unit::set_instruction(int code, int mask, const instruction_type &in)
{
  I (code >= 0);
  I (code < 0x10000);
  code &= ~mask;
  for (int i = code; i <= (code | mask); ++i)
    {
      if ((i & ~mask) == code)
	{
	  instruction_type old_value = set_instruction(i, in);
#ifdef L
	  if (old_value.first != &illegal)
	    L("warning: Replacing instruction handler at 0x%04x\n", i);
#endif
	}
    }
}

exec_unit::exec_unit()
  : instructions(0x10000, instruction_type(&illegal, 0))
{
  unsigned long data = reinterpret_cast<unsigned long>(this);
  install_instructions_0(*this, data);
  install_instructions_1(*this, data);
  install_instructions_2(*this, data);
  install_instructions_3(*this, data);
  install_instructions_4(*this, data);
  install_instructions_5(*this, data);
  install_instructions_6(*this, data);
  install_instructions_7(*this, data);
  install_instructions_8(*this, data);
  install_instructions_9(*this, data);
  install_instructions_11(*this, data);
  install_instructions_12(*this, data);
  install_instructions_13(*this, data);
  install_instructions_14(*this, data);
}

/* Executes an illegal instruction.  */
void
exec_unit::illegal(uint_type op, context &c, unsigned long data)
{
  throw illegal_instruction_exception();
}
