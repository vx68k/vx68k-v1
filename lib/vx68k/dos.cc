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

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

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

uint16
dos::execute (const char *name, const char *const *argv)
{
  dos_exec_context ec(mem, &main_cpu);
  return ec.start(ec.load_executable(name), argv);
}

namespace
{
  void
  dos_chmod(unsigned int op, context &ec)
  {
#ifdef L
    L(" DOS _CHMOD");
    L("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc);
#endif

    // FIXME.
    ec.regs.d[0] = -1;

    ec.regs.pc += 2;
  }

  void dos_close(unsigned int op, context &ec)
    {
      VL((" DOS _CLOSE\n"));

      // FIXME.
      int fd = extsw(ec.mem->getw(SUPER_DATA, ec.regs.a[7]));
      ec.regs.d[0] = static_cast<dos_exec_context &>(ec).close(fd);

      ec.regs.pc += 2;
    }

  void
  dos_create(unsigned int op, context &ec)
  {
#ifdef L
    L(" DOS _CREATE");
    L("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc);
#endif

    // FIXME.
    ec.regs.d[0] = open("create.out", O_RDWR | O_CREAT | O_TRUNC, 0666);

    ec.regs.pc += 2;
  }

  void
  dos_delete(unsigned int op, context &ec)
  {
#ifdef L
    L(" DOS _DELETE");
    L("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc);
#endif

    // FIXME.
    ec.regs.d[0] = 0;

    ec.regs.pc += 2;
  }

  void dos_exit2(unsigned int op, context &ec)
    {
      VL((" DOS _EXIT2\n"));

      uint32 sp = ec.regs.a[7];
      unsigned int status = ec.mem->getw(SUPER_DATA, sp + 0);
      static_cast<dos_exec_context &>(ec).exit(status);
    }

  void
  dos_filedate(unsigned int op, context &ec)
  {
#ifdef L
    L(" DOS _FILEDATE");
    L("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc);
#endif

    // FIXME.
    ec.regs.d[0] = 0;

    ec.regs.pc += 2;
  }

  void dos_fgetc(unsigned int op, context &ec)
    {
      VL((" DOS _FGETC\n"));

      // FIXME.
      uint32 sp = ec.regs.a[7];
      int fd = extsw(ec.mem->getw(SUPER_DATA, sp));
      ec.regs.d[0] = static_cast<dos_exec_context &>(ec).fgetc(fd);

      ec.regs.pc += 2;
    }

  void dos_ioctrl(unsigned int op, context &ec)
    {
#ifdef L
      L(" DOS _IOCTRL");
      L("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc);
#endif

      // FIXME.
      ec.regs.d[0] = 0;

      ec.regs.pc += 2;
    }

  void
  dos_malloc(unsigned int op, context &ec)
  {
#ifdef L
    L(" DOS _MALLOC");
    L("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc);
#endif

    // FIXME.
    ec.regs.d[0] = 0x110000;

    ec.regs.pc += 2;
  }

  void dos_open(unsigned int op, context &ec)
    {
      VL((" DOS _OPEN\n"));

      // FIXME.
      uint32 sp = ec.regs.a[7];
      uint32 name_address = ec.mem->getl(SUPER_DATA, sp + 0);
      unsigned int flags = ec.mem->getw(SUPER_DATA, sp + 4);

      char name[256];
      ec.mem->read(SUPER_DATA, name_address, name, 256);
      ec.regs.d[0] = static_cast<dos_exec_context &>(ec).open(name, flags);

      ec.regs.pc += 2;
    }

  void dos_print(unsigned int op, context &ec)
    {
      VL((" DOS _PRINT\n"));

      uint32 address = ec.mem->getl(SUPER_DATA, ec.regs.a[7]);

      // FIXME.
      unsigned char buf[1];
      do
	{
	  buf[0] = ec.mem->getb(SUPER_DATA, address++);
	  if (buf[0] != 0)
	    write(STDOUT_FILENO, buf, 1);
	}
      while (buf[0] != 0);

      ec.regs.pc += 2;
    }

  void
  dos_putchar(unsigned int op, context &ec)
  {
#ifdef L
    L(" DOS _PUTCHAR");
    L("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc);
#endif

    // FIXME.
    uint32 sp = ec.regs.a[7];
    char ch = extsw(ec.mem->getw(SUPER_DATA, sp));
    write(STDOUT_FILENO, &ch, 1);
    ec.regs.d[0] = 0;

    ec.regs.pc += 2;
  }

  void dos_read(unsigned int op, context &ec)
    {
      VL((" DOS _READ\n"));

      // FIXME.
      uint32 sp = ec.regs.a[7];
      int fd = extsw(ec.mem->getw(SUPER_DATA, sp));
      uint32 data = ec.mem->getl(SUPER_DATA, sp + 2);
      uint32 size = ec.mem->getl(SUPER_DATA, sp + 6);
      ec.regs.d[0] = static_cast<dos_exec_context &>(ec).read(fd, data, size);

      ec.regs.pc += 2;
    }

  void dos_seek(unsigned int op, context &ec)
    {
      VL((" DOS _SEEK\n"));

      // FIXME.
      uint32 sp = ec.regs.a[7];
      int fd = extsw(ec.mem->getw(SUPER_DATA, sp));
      int32 offset = extsl(ec.mem->getl(SUPER_DATA, sp + 2));
      unsigned int whence = ec.mem->getw(SUPER_DATA, sp + 6);
      ec.regs.d[0]
	= static_cast<dos_exec_context &>(ec).seek(fd, offset, whence);

      ec.regs.pc += 2;
    }

  void dos_setblock(unsigned int op, context &ec)
    {
#ifdef L
      L(" DOS _SETBLOCK");
      L("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc);
#endif

      // FIXME.
      ec.regs.d[0] = 0;

      ec.regs.pc += 2;
    }

  void
  dos_write(unsigned int op, context &ec)
  {
#ifdef L
    L(" DOS _WRITE");
    L("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc);
#endif

    // FIXME.
    uint32_type sp = ec.regs.a[7];
    int fd = extsw(ec.mem->getw(SUPER_DATA, sp));
    uint32_type data = ec.mem->getl(SUPER_DATA, sp + 2);
    uint32_type size = ec.mem->getl(SUPER_DATA, sp + 6);
    ec.regs.d[0] = static_cast<dos_exec_context &>(ec).write(fd, data, size);

    ec.regs.pc += 2;
  }
} // (unnamed namespace)

dos::dos(address_space *m, size_t)
  : mem(m)
{
  main_cpu.set_instruction(0xff02, 0, &dos_putchar);
  main_cpu.set_instruction(0xff09, 0, &dos_print);
  main_cpu.set_instruction(0xff1b, 0, &dos_fgetc);
  main_cpu.set_instruction(0xff3c, 0, &dos_create);
  main_cpu.set_instruction(0xff3d, 0, &dos_open);
  main_cpu.set_instruction(0xff3e, 0, &dos_close);
  main_cpu.set_instruction(0xff3f, 0, &dos_read);
  main_cpu.set_instruction(0xff40, 0, &dos_write);
  main_cpu.set_instruction(0xff41, 0, &dos_delete);
  main_cpu.set_instruction(0xff42, 0, &dos_seek);
  main_cpu.set_instruction(0xff43, 0, &dos_chmod);
  main_cpu.set_instruction(0xff44, 0, &dos_ioctrl);
  main_cpu.set_instruction(0xff48, 0, &dos_malloc);
  main_cpu.set_instruction(0xff4a, 0, &dos_setblock);
  main_cpu.set_instruction(0xff4c, 0, &dos_exit2);
  main_cpu.set_instruction(0xff57, 0, &dos_filedate);

  main_cpu.set_instruction(0xff87, 0, &dos_filedate);
}

