/* vx68k - Virtual X68000
   Copyright (C) 1998 Hypercore Software Design, Ltd.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#undef const
#undef inline

#include "vm68k/cpu.h"
#include "vx68k/memory.h"

using namespace vx68k;
using vm68k::execution_context;
using vm68k::cpu;

/* vx68k main.  */
int
main (int argc, char **argv)
{
  address_space mem (4 * 1024 * 1024); // FIXME
  execution_context ec (&mem);
  ec.regs.pc = 0x8100;		// FIXME
  cpu main_cpu;
  main_cpu.run (&ec);
  return 0;
}

