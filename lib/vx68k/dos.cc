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

#include "debug.h"

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
  void dos_close(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      VL((" DOS _CLOSE\n"));

      // FIXME.
      int fd = extsw(ec->mem->getw(SUPER_DATA, ec->regs.a[7]));
      ec->regs.d[0] = static_cast<dos_exec_context *>(ec)->close(fd);

      ec->regs.pc += 2;
    }

  void dos_exit2(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      VL((" DOS _EXIT2\n"));

      uint32 sp = ec->regs.a[7];
      unsigned int status = ec->mem->getw(SUPER_DATA, sp + 0);
      static_cast<dos_exec_context *>(ec)->exit(status);
    }

  void dos_fgetc(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      VL((" DOS _FGETC\n"));

      // FIXME.
      uint32 sp = ec->regs.a[7];
      int fd = extsw(ec->mem->getw(SUPER_DATA, sp));
      ec->regs.d[0] = static_cast<dos_exec_context *>(ec)->fgetc(fd);

      ec->regs.pc += 2;
    }

  void dos_open(unsigned int op, execution_context *ec)
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

  void dos_print(unsigned int op, execution_context *ec)
    {
      I(ec != NULL);
      VL((" DOS _PRINT\n"));

      uint32 address = ec->mem->getl(SUPER_DATA, ec->regs.a[7]);

      // FIXME.
      unsigned char buf[1];
      do
	{
	  buf[0] = ec->mem->getb(SUPER_DATA, address++);
	  if (buf[0] != 0)
	    write(STDOUT_FILENO, buf, 1);
	}
      while (buf[0] != 0);

      ec->regs.pc += 2;
    }

  void dos_read(unsigned int op, execution_context *ec)
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

  void dos_seek(unsigned int op, execution_context *ec)
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

