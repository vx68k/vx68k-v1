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

#include <iostream>
#include <fstream>
#include <cstdlib>

using namespace std;
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
dos::load_executable (const char *name)
{
  ifstream is (name, ios::in | ios::binary);
  if (!is)
    abort ();			// FIXME
  char head[64];
  is.read (head, 64);
  if (!is)
    abort ();			// FIXME
  if (head[0] != 'H' || head[1] != 'U')
    abort ();			// FIXME
  cerr << "Code size: " << getl (head + 12) << "\n";
  cerr << "Data size: " << getl (head + 16) << "\n";

  size_t load_size = getl (head + 12) + getl (head + 16);
  char *buf = static_cast <char *> (malloc (load_size));
  try
    {
      is.read (buf, load_size);
      if (!is)
	abort ();		// FIXME
      main_ec.mem->write (SUPER_DATA, 0x8100, buf, load_size);
    }
  catch (...)
    {
      free (buf);
    }
  free (buf);

  // Fix relocations here.

  return 0x8100;		// FIXME
}

uint16
dos::start (uint32 address)
{
  main_ec.regs.pc = address;
  uint16 status = 0;
 restart:
  try
    {
      main_cpu.run (&main_ec);
      abort ();
    }
  catch (illegal_instruction &e)
    {
      uint16 op = main_ec.mem->getw (SUPER_DATA, main_ec.regs.pc);
      cerr << hex << "vm68k illegal instruction (op = 0x" << op << ")\n" << dec;
      status = 0xff;
    }

  return status;
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
