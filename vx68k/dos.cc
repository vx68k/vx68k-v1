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

  uint32 base = getl (head + 4);
  uint32 start_offset = getl (head + 8);
  size_t text_size = getl (head + 12);
  size_t data_size = getl (head + 16);
  size_t bss_size = getl (head + 20);
  size_t reloc_size = getl (head + 24);
  cerr << hex << "Base : 0x" << base << "\n";
  cerr << "Start: 0x" << start_offset << "\n" << dec;
  cerr << "Text : " << text_size << "\n";
  cerr << "Data : " << data_size << "\n";
  cerr << "BSS  : " << bss_size << "\n";
  cerr << "Reloc: " << reloc_size << "\n";

  uint32 load_address = 0x8100;	// FIXME.

  char *buf = static_cast <char *> (malloc (text_size + data_size));
  try
    {
      is.read (buf, text_size + data_size);
      if (!is)
	abort ();		// FIXME
      main_ec.mem->write (SUPER_DATA, load_address,
			  buf, text_size + data_size);
    }
  catch (...)
    {
      free (buf);
      throw;
    }
  free (buf);

  /* Fix-up.  */
  char *fixup_buf = static_cast<char *>(malloc(reloc_size));
  try
    {
      is.read(fixup_buf, reloc_size);
      if (!is)
	abort();		// FIXME.

      const char *p = fixup_buf;
      uint32 address = load_address;
      while (p != fixup_buf + reloc_size)
	{
	  uint32 d = getw(p);
	  p += 2;
	  if (d == 1)		// Prefix for long offset.
	    {
	      d = getl(p);
	      p += 4;
	    }
	  if (d % 2 != 0)
	    {
	      cerr << "Illegal fixup at an odd address\n";
	      abort();		// FIXME.
	    }
	  address += d;
	  uint32 value = main_ec.mem->getl(SUPER_DATA, address);
	  main_ec.mem->putl(SUPER_DATA, address, value + load_address - base);
	  cerr << "Fixup at 0x" << hex << address
	       << " (0x" << value << " -> 0x" << value + load_address - base
	       << dec << ")\n";
	}
    }
  catch (...)
    {
      free(fixup_buf);
      throw;
    }
  free(fixup_buf);

  // Fix relocations here.

  return load_address + start_offset;
}

uint16
dos::start (uint32 address, const char *const *argv)
{
  main_ec.regs.pc = address;
  main_ec.regs.a[7] = 0x8000;	// FIXME.
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
dos::execute (const char *name, const char *const *argv)
{
  return start (load_executable (name), argv);
}

dos::dos (address_space *as, size_t)
  : main_ec (as)
{
}

};				// namespace human

};				// namespace vx68k
