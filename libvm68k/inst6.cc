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
#include <vm68k/conditional.h>
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
using namespace vm68k::conditional;
using namespace vm68k::addressing;

#ifdef HAVE_NANA_H
extern bool nana_instruction_trace;
# undef L_DEFAULT_GUARD
# define L_DEFAULT_GUARD nana_instruction_trace
#endif

namespace
{
  using vm68k::memory;
  using vm68k::context;

  /* Handles a Bcc instruction.  */
  template <class Condition> void 
  m68k_b(uint16_type op, context &c, unsigned long data)
  {
    word_size::svalue_type disp = op & 0xff;
    size_t extsize;
    if (disp == 0)
      {
	disp = c.fetch(word_size(), 2);
	extsize = 2;
      }
    else
      {
	disp = byte_size::svalue(disp);
	extsize = 0;
      }
#ifdef HAVE_NANA_H
    L("\tb%s %#lx\n", Condition::text(),
      long_word_size::uvalue(c.regs.pc + 2 + disp) + 0UL);
#endif

    // This instruction does not affect the condition codes.
    Condition cond;
    c.regs.pc += 2 + (cond(c) ? disp : extsize);
  }

  /* Handles a BRA instruction.  */
  void
  m68k_bra(uint16_type op, context &c, unsigned long data)
  {
    word_size::svalue_type disp = op & 0xff;
    size_t len = 0;
    if (disp == 0)
      {
	disp = c.fetch(word_size(), 2);
	len = word_size::aligned_value_size();
      }
    else
      disp = byte_size::svalue(disp);
#ifdef HAVE_NANA_H
    L("\tbra %#lx\n", long_word_size::uvalue(c.regs.pc + 2 + disp) + 0UL);
#endif

    // XXX: The condition codes are not affected.
    c.regs.pc += 2 + disp;
  }

  /* Handles a BSR instruction.  */
  void
  m68k_bsr(uint16_type op, context &c, unsigned long data)
  {
    word_size::svalue_type disp = op & 0xff;
    size_t len = 0;
    if (disp == 0)
      {
	disp = c.fetch(word_size(), 2);
	len = word_size::aligned_value_size();
      }
    else
      disp = byte_size::svalue(disp);
#ifdef HAVE_NANA_H
    L("\tbsr %#lx\n", long_word_size::uvalue(c.regs.pc + 2 + disp) + 0UL);
#endif

    // XXX: The condition codes are not affected.
    memory::function_code fc = c.data_fc();
    long_word_size::put(*c.mem, fc,
			c.regs.a[7] - long_word_size::aligned_value_size(),
			c.regs.pc + 2 + len);
    c.regs.a[7] -= long_word_size::aligned_value_size();

    c.regs.pc += 2 + disp;
  }
}

void
vm68k::install_instructions_6(exec_unit &eu, unsigned long data)
{
  static const instruction_map inst[]
    = {{0x6000,  0xff, &m68k_bra},
       {0x6100,  0xff, &m68k_bsr},
       {0x6200,  0xff, &m68k_b<hi>},
       {0x6300,  0xff, &m68k_b<ls>},
       {0x6400,  0xff, &m68k_b<cc>},
       {0x6500,  0xff, &m68k_b<cs>},
       {0x6600,  0xff, &m68k_b<ne>},
       {0x6700,  0xff, &m68k_b<eq>},
       {0x6a00,  0xff, &m68k_b<pl>},
       {0x6b00,  0xff, &m68k_b<mi>},
       {0x6c00,  0xff, &m68k_b<ge>},
       {0x6d00,  0xff, &m68k_b<lt>},
       {0x6e00,  0xff, &m68k_b<gt>},
       {0x6f00,  0xff, &m68k_b<le>}};

  for (const instruction_map *i = inst + 0;
       i != inst + sizeof inst / sizeof inst[0]; ++i)
    eu.set_instruction(i->base, i->mask, make_pair(i->handler, data));
}
