/* vx68k - Virtual X68000
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

#include <vm68k/cpu.h>

#ifdef HAVE_NANA_H
# include <nana.h>
# include <cstdio>
#else
# include <cassert>
# define I assert
#endif

using vm68k::context;
using namespace vm68k::types;
using namespace std;

void
context::interrupt(int prio, unsigned int vecno)
{
  if (prio < 1 || prio > 7)
    return;

  vecno &= 0xffu;
  interrupt_queues[7 - prio].push(vecno);
  a_interrupted = true;
}

void
context::handle_interrupts()
{
  vector<queue<unsigned int> >::iterator i = interrupt_queues.begin();
  while (i->empty())
    {
      ++i;
      I(i != interrupt_queues.end());
    }

  int prio = interrupt_queues.end() - i;
  int level = sr() >> 8 & 0x7;
  if (prio == 7 || prio > level)
    {
      unsigned int vecno = i->front();
      i->pop();

      uint_type old_sr = this->sr();
      this->set_sr(old_sr & ~0x700 | prio << 8);
      this->set_supervisor_state(true);
      regs.a[7] -= 6;
      mem->putl(SUPER_DATA, regs.a[7] + 2, regs.pc);
      mem->putw(SUPER_DATA, regs.a[7] + 0, old_sr);

      uint32_type address = vecno * 4u;
      regs.pc = mem->getl(SUPER_DATA, address);

      a_interrupted = false;
      vector<queue<unsigned int> >::iterator j = i;
      while (j != interrupt_queues.end())
	{
	  if (!j->empty())
	    a_interrupted = true;
	  ++j;
	}
    }
}

void
context::set_supervisor_state(bool state)
{
  if (state)
    {
      if (!supervisor_state())
	{
	  regs.usp = regs.a[7];
	  regs.ccr.set_s_bit(true);
	  regs.a[7] = regs.ssp;

	  pfc_cache = SUPER_PROGRAM;
	  dfc_cache = SUPER_DATA;
	}
    }
  else
    {
      if (supervisor_state())
	{
	  regs.ssp = regs.a[7];
	  regs.ccr.set_s_bit(false);
	  regs.a[7] = regs.usp;

	  pfc_cache = USER_PROGRAM;
	  dfc_cache = USER_DATA;
	}
    }
}

uint_type
context::sr() const
{
  return regs.ccr;
}

void
context::set_sr(uint_type value)
{
  set_supervisor_state(value & 0x2000);
  regs.ccr = value;
}

context::context(address_space *m)
  : mem(m),
    pfc_cache(regs.ccr.supervisor_state() ? SUPER_PROGRAM : USER_PROGRAM),
    dfc_cache(regs.ccr.supervisor_state() ? SUPER_DATA : USER_DATA),
    a_interrupted(false),
    interrupt_queues(7)
{
}
