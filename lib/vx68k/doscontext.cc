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

#include <vx68k/human.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <fstream>
#include <cstdlib>
#include <cstdio>

#ifdef HAVE_NANA_H
# include <nana.h>
# include <cstdio>
#else
# include <cassert>
# define I assert
# define VL(EXPR)
#endif

using namespace vx68k::human;
using namespace vm68k;
using namespace std;

int
dos_exec_context::fgetc(int fd)
{
  unsigned char data_buf[1];
  ssize_t done = ::read(fd, data_buf, 1);
  if (done == -1)
    return -6;			// FIXME.
  return data_buf[0];
}

int32
dos_exec_context::seek(int fd, int32 offset, unsigned int whence)
{
  // FIXME.
  int32 pos = ::lseek(fd, offset, whence);
  if (pos == -1)
    return -6;			// FIXME.
  return pos;
}

namespace
{
  struct quit_loop
  {
    uint16 status;
    quit_loop(uint16 s): status(s) {};
  };
}

void
dos_exec_context::exit(unsigned int status)
{
  throw quit_loop(status);
}

uint16
dos_exec_context::start(uint32 address, const char *const *argv)
{
  regs.pc = address;
  uint16 status = 0;
  try
    {
      run();
      abort ();
    }
  catch (quit_loop &x)
    {
      status = x.status;
    }
  catch (illegal_instruction &e)
    {
      uint16 op = mem->getw(SUPER_DATA, regs.pc);
      fprintf(stderr, "vm68k illegal instruction (op = 0x%x)\n", op);
      status = 0xff;
    }

  return status;
}

uint32
dos_exec_context::load_executable(const char *name, uint32_type address)
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
  if (debug_level >= 1)
    {
      fprintf(stderr, "Base : 0x%lx\n", (unsigned long) base);
      fprintf(stderr, "Start: 0x%lx\n", (unsigned long) start_offset);
      fprintf(stderr, "Text : %lu\n", (unsigned long) text_size);
      fprintf(stderr, "Data : %lu\n", (unsigned long) data_size);
      fprintf(stderr, "BSS  : %lu\n", (unsigned long) bss_size);
      fprintf(stderr, "Fixup: %lu\n", (unsigned long) reloc_size);
    }

  uint32 load_address = address + 0xf0;

  char *buf = static_cast <char *> (malloc (text_size + data_size));
  try
    {
      is.read (buf, text_size + data_size);
      if (!is)
	abort ();		// FIXME
      mem->write(SUPER_DATA, load_address,
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
	      fprintf(stderr, "Illegal fixup at an odd address\n");
	      abort();		// FIXME.
	    }
	  address += d;
	  uint32 value = mem->getl(SUPER_DATA, address);
	  mem->putl(SUPER_DATA, address, value + load_address - base);
	  VL(("Fixup at 0x%lx (0x%lx -> 0x%lx)\n",
	      (unsigned long) address, (unsigned long) value,
	      (unsigned long) value + load_address - base));
	}
    }
  catch (...)
    {
      free(fixup_buf);
      throw;
    }
  free(fixup_buf);

  // PSP setup.
  mem->write(SUPER_DATA, load_address - 128,
	     name, strlen(name) + 1);
  regs.a[0] = load_address - 0x100;
  regs.a[1] = mem->getl(SUPER_DATA, regs.a[0] + 8);
  regs.a[2] = 0x7000;		// FIXME.
  regs.a[3] = 0x7000;		// FIXME.
  regs.a[4] = load_address + start_offset;

  return load_address + start_offset;
}

dos_exec_context::dos_exec_context(exec_unit *e, address_space *m, process *p)
  : context(m, e),
    _process(p),
    debug_level(0)
{
}

