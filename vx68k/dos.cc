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

#include "vx68k/human.h"

using namespace vm68k;

namespace vx68k
{
#if 0
};
#endif

namespace human
{
#if 0
};
#endif

uint32
dos::load_executable (const char *)
{
  main_ec.mem->putw (SUPER_DATA, 0x8100, 0xff00);
  return 0x8100;		// FIXME
}

uint16
dos::start (uint32 address)
{
  main_ec.regs.pc = address;
  main_cpu.run (&main_ec);
  return 0;
}

uint16
dos::execute (const char *name)
{
  return start (load_executable (name));
}

dos::dos (address_space *as, size_t)
  : main_ec (as)
{
}

};				// namespace human

};				// namespace vx68k
