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

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <fstream>
#include <cstdlib>
#include <cstdio>

#include "debug.h"

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
  return data_buf[1];
}

/* Reads data from the file with a DOS file descriptor.  */
int32
dos_exec_context::read(int fd, uint32 data, uint32 size)
{
  // FIXME.
  uint8 *data_buf = static_cast<uint8 *>(malloc(size));
  ssize_t done = ::read(fd, data_buf, size);
  if (done == -1)
    return -6;			// FIXME.
  mem->write(SUPER_DATA, data, data_buf, size);
  return done;
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

/* Closes a DOS file descriptor.  */
int
dos_exec_context::close(int fd)
{
  // FIXME.
  if (::close(fd) == -1)
    return -6;			// FIXME.
  return 0;
}

/* Opens a file.  */
int
dos_exec_context::open(const char *name, unsigned int flag)
{
  // FIXME.
  static const int uflag[] = {O_RDONLY, O_WRONLY, O_RDWR};
  if ((flag & 0xf) > 2)
    return -12;			// FIXME.
  VL(("Opening %s\n", name));
  int fd = ::open(name, uflag[flag & 0xf]);
  if (fd == -1)
    return -2;			// FIXME: errno test.
  return fd;
}

uint32
dos_exec_context::load_executable(const char *name)
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
  fprintf(stderr, "Base : 0x%lx\n", (unsigned long) base);
  fprintf(stderr, "Start: 0x%lx\n", (unsigned long) start_offset);
  fprintf(stderr, "Text : %lu\n", (unsigned long) text_size);
  fprintf(stderr, "Data : %lu\n", (unsigned long) data_size);
  fprintf(stderr, "BSS  : %lu\n", (unsigned long) bss_size);
  fprintf(stderr, "Fixup: %lu\n", (unsigned long) reloc_size);

  uint32 load_address = 0x8100;	// FIXME.

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
  regs.a[0] = load_address - 0x100; // FIXME.
  regs.a[1] = load_address + 0x100000; // FIXME.
  regs.a[2] = 0x7000;		// FIXME.
  regs.a[3] = 0x7000;		// FIXME.
  regs.a[4] = load_address + start_offset;

  return load_address + start_offset;
}

namespace
{
  struct quit_loop
  {
    uint16 status;
    quit_loop(uint16 s): status(s) {};
  };
}

uint16
dos_exec_context::start(uint32 address, const char *const *argv)
{
  regs.pc = address;
  regs.a[7] = 0x8000;		// FIXME.
  uint16 status = 0;
 restart:
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

dos_exec_context::dos_exec_context(address_space *m, exec_unit *e)
  : execution_context(m, e)
{
}

uint16
dos::execute (const char *name, const char *const *argv)
{
  dos_exec_context ec(mem, &main_cpu);
  return ec.start(ec.load_executable(name), argv);
}

namespace
{
  void dos_close(int op, execution_context *ec)
    {
      I(ec != NULL);
      VL((" DOS _CLOSE\n"));

      // FIXME.
      int fd = extsw(ec->mem->getw(SUPER_DATA, ec->regs.a[7]));
      ec->regs.d[0] = static_cast<dos_exec_context *>(ec)->close(fd);

      ec->regs.pc += 2;
    }

  void dos_exit2(int op, execution_context *ec)
    {
      I(ec != NULL);
      VL((" DOS _EXIT2\n"));

      throw quit_loop(ec->mem->getw(SUPER_DATA, ec->regs.a[7]));
    }

  void dos_fgetc(int op, execution_context *ec)
    {
      I(ec != NULL);
      VL((" DOS _FGETC\n"));

      // FIXME.
      uint32 sp = ec->regs.a[7];
      int fd = extsw(ec->mem->getw(SUPER_DATA, sp));
      ec->regs.d[0] = static_cast<dos_exec_context *>(ec)->fgetc(fd);

      ec->regs.pc += 2;
    }

  void dos_open(int op, execution_context *ec)
    {
      I(ec != NULL);
      VL((" DOS _OPEN\n"));

      // FIXME.
      uint32 sp = ec->regs.a[7];
      uint32 name_address = ec->mem->getl(SUPER_DATA, sp + 0);
      unsigned int flags = ec->mem->getw(SUPER_DATA, sp + 4);

      char name[256];
      ec->mem->read(SUPER_DATA, name_address, name, 256);
      ec->regs.d[0] = static_cast<dos_exec_context *>(ec)->open(name, flags);

      ec->regs.pc += 2;
    }

  void dos_print(int op, execution_context *ec)
    {
      I(ec != NULL);
      VL((" DOS _PRINT\n"));

      uint32 address = ec->mem->getl(SUPER_DATA, ec->regs.a[7]);

      int value;
      do
	{
	  value = ec->mem->getb(SUPER_DATA, address++);
	  if (value != 0)
	    putchar(value);
	}
      while (value != 0);

      ec->regs.pc += 2;
    }

  void dos_read(int op, execution_context *ec)
    {
      I(ec != NULL);
      VL((" DOS _READ\n"));

      // FIXME.
      uint32 sp = ec->regs.a[7];
      int fd = extsw(ec->mem->getw(SUPER_DATA, sp));
      uint32 data = ec->mem->getl(SUPER_DATA, sp + 2);
      uint32 size = ec->mem->getl(SUPER_DATA, sp + 6);
      ec->regs.d[0] = static_cast<dos_exec_context *>(ec)->read(fd, data, size);

      ec->regs.pc += 2;
    }

  void dos_seek(int op, execution_context *ec)
    {
      I(ec != NULL);
      VL((" DOS _SEEK\n"));

      // FIXME.
      uint32 sp = ec->regs.a[7];
      int fd = extsw(ec->mem->getw(SUPER_DATA, sp));
      int32 offset = extsl(ec->mem->getl(SUPER_DATA, sp + 2));
      unsigned int whence = ec->mem->getw(SUPER_DATA, sp + 6);
      ec->regs.d[0]
	= static_cast<dos_exec_context *>(ec)->seek(fd, offset, whence);

      ec->regs.pc += 2;
    }
} // (unnamed namespace)

dos::dos(address_space *m, size_t)
  : mem(m)
{
  main_cpu.set_instruction(0xff09, 0, &dos_print);
  main_cpu.set_instruction(0xff1b, 0, &dos_fgetc);
  main_cpu.set_instruction(0xff3d, 0, &dos_open);
  main_cpu.set_instruction(0xff3e, 0, &dos_close);
  main_cpu.set_instruction(0xff3f, 0, &dos_read);
  main_cpu.set_instruction(0xff42, 0, &dos_seek);
  main_cpu.set_instruction(0xff4c, 0, &dos_exit2);
}

