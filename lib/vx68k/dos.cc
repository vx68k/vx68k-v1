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

#include <ctime>
#include <cstring>

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

namespace
{
  void
  dos_chmod(uint_type op, context &c, instruction_data *data)
  {
    uint32_type sp = c.regs.a[7];
    uint32_type nameptr = c.mem->getl(SUPER_DATA, sp + 0);
    sint_type atr = extsw(c.mem->getw(SUPER_DATA, sp + 4));
#ifdef L
    L(" DOS _CHMOD\n");
#endif

    dos *d = static_cast<dos *>(data);
    I(d != NULL);
    c.regs.d[0] = d->fs()->chmod(c.mem, nameptr, atr);

    c.regs.pc += 2;
  }

  void
  dos_close(unsigned int op, context &ec, instruction_data *data)
  {
#ifdef L
    L(" DOS _CLOSE\n");
    L("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc);
#endif

    uint32_type sp = ec.regs.a[7];
    sint_type fd = extsw(ec.mem->getw(SUPER_DATA, sp));

    ec.regs.d[0] = static_cast<dos_exec_context &>(ec).close(fd);

    ec.regs.pc += 2;
  }

  void
  dos_create(unsigned int op, context &ec, instruction_data *data)
  {
    uint32_type sp = ec.regs.a[7];
    uint32_type nameptr = ec.mem->getl(SUPER_DATA, sp + 0);
    uint_type atr = ec.mem->getw(SUPER_DATA, sp + 4);
#ifdef L
    L(" DOS _CREATE\n");
#endif

    ec.regs.d[0] = static_cast<dos_exec_context &>(ec).create(nameptr, atr);

    ec.regs.pc += 2;
  }

  void
  dos_delete(unsigned int op, context &ec, instruction_data *data)
  {
#ifdef L
    L(" DOS _DELETE");
    L("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc);
#endif

    // FIXME.
    ec.regs.d[0] = 0;

    ec.regs.pc += 2;
  }

  void
  dos_dup(unsigned int op, context &ec, instruction_data *data)
  {
    uint32_type sp = ec.regs.a[7];
    uint_type filno = ec.mem->getw(SUPER_DATA, sp);
#ifdef L
    L(" DOS _DUP\n");
#endif

    ec.regs.d[0] = static_cast<dos_exec_context &>(ec).dup(filno);

    ec.regs.pc += 2;
  }

  void
  dos_exit2(unsigned int op, context &ec, instruction_data *data)
    {
      VL((" DOS _EXIT2\n"));

      uint32 sp = ec.regs.a[7];
      unsigned int status = ec.mem->getw(SUPER_DATA, sp + 0);
      static_cast<dos_exec_context &>(ec).exit(status);
    }

  void
  dos_fflush(uint_type op, context &c, instruction_data *data)
  {
#ifdef L
    L(" DOS _FFLUSH\n");
#endif

    dos *d = static_cast<dos *>(data);
    I(d != NULL);
    // FIXME

    c.regs.pc += 2;
  }

  void
  dos_filedate(unsigned int op, context &ec, instruction_data *data)
  {
#ifdef L
    L(" DOS _FILEDATE");
    L("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc);
#endif

    // FIXME.
    ec.regs.d[0] = 0;

    ec.regs.pc += 2;
  }

  void
  dos_fgetc(unsigned int op, context &ec, instruction_data *data)
    {
      VL((" DOS _FGETC\n"));

      uint32 sp = ec.regs.a[7];
      int fd = extsw(ec.mem->getw(SUPER_DATA, sp));
      ec.regs.d[0] = static_cast<dos_exec_context &>(ec).fgetc(fd);

      ec.regs.pc += 2;
    }

  void
  dos_fputs(uint_type op, context &ec, instruction_data *data)
  {
#ifdef L
    L(" DOS _FPUTS\n");
#endif

    uint32_type sp = ec.regs.a[7];
    uint32_type mesptr = ec.mem->getl(SUPER_DATA, sp + 0);
    sint_type filno = extsw(ec.mem->getw(SUPER_DATA, sp + 4));

    ec.regs.d[0] = static_cast<dos_exec_context &>(ec).fputs(mesptr, filno);

    ec.regs.pc += 2;
  }

  void
  dos_getdate(uint_type op, context &ec, instruction_data *data)
  {
#ifdef L
    L(" DOS _GETDATE");
    L("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc);
#endif

    time_t t = time(NULL);
#ifdef HAVE_LOCALTIME_R
    struct tm lt0;
    struct tm *lt = localtime_r(&t, &lt0);
#else
    struct tm *lt = localtime(&t);
#endif

    ec.regs.d[0] = (lt->tm_wday << 16
		    | lt->tm_year - 80 << 9
		    | lt->tm_mon + 1 << 5
		    | lt->tm_mday);

    ec.regs.pc += 2;
  }

  void
  dos_getenv(uint_type op, context &c, instruction_data *data)
  {
    uint32_type sp = c.regs.a[7];
    uint32_type getname = c.mem->getw(SUPER_DATA, sp + 0);
    uint32_type env = c.mem->getw(SUPER_DATA, sp + 4);
    uint32_type getbuf = c.mem->getw(SUPER_DATA, sp + 8);
#ifdef L
    L(" DOS _GETENV\n");
#endif

    c.regs.d[0]
      = static_cast<dos_exec_context &>(c).getenv(getname, env, getbuf);

    c.regs.pc += 2;
  }

  void
  dos_getpdb(unsigned int op, context &ec, instruction_data *data)
  {
#ifdef L
    L(" DOS _GETPDB");
    L("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc);
#endif

    ec.regs.d[0] = static_cast<dos_exec_context &>(ec).getpdb();

    ec.regs.pc += 2;
  }

  void
  dos_gettim2(uint_type op, context &ec, instruction_data *data)
  {
#ifdef L
    L(" DOS _GETTIM2");
    L("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc);
#endif

    time_t t = time(NULL);
#ifdef HAVE_LOCALTIME_R
    struct tm lt0;
    struct tm *lt = localtime_r(&t, &lt0);
#else
    struct tm *lt = localtime(&t);
#endif

    ec.regs.d[0] = (lt->tm_hour << 16
		    | lt->tm_min << 8
		    | lt->tm_sec);

    ec.regs.pc += 2;
  }

  void
  dos_intvcs(unsigned int op, context &ec, instruction_data *data)
  {
#ifdef L
    L(" DOS _INTVCS");
    L("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc);
#endif

    // FIXME.
    ec.regs.d[0] = 0;

    ec.regs.pc += 2;
  }

  void
  dos_ioctrl(unsigned int op, context &ec, instruction_data *data)
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
  dos_malloc(unsigned int op, context &ec, instruction_data *data)
  {
    uint32_type sp = ec.regs.a[7];
    uint32_type len = ec.mem->getl(SUPER_DATA, sp + 0);
#ifdef L
    L(" DOS _MALLOC\n");
#endif

    ec.regs.d[0] = static_cast<dos_exec_context &>(ec).malloc(len);

    ec.regs.pc += 2;
  }

  void
  dos_nameck(uint_type op, context &c, instruction_data *data)
  {
    uint32_type sp = c.regs.a[7];
    uint32_type file = c.mem->getl(SUPER_DATA, sp + 0);
    uint32_type buffer = c.mem->getl(SUPER_DATA, sp + 4);
#ifdef L
    L(" DOS _NAMECK\n");
#endif

    dos *d = static_cast<dos *>(data);
    I(d != NULL);

    // FIXME
    string buf = c.mem->gets(SUPER_DATA, file);
    string::size_type p = buf.find_last_of(buf, '/');
    if (p == string::npos)
      {
	c.mem->puts(SUPER_DATA, buffer + 0, "./");
	c.mem->puts(SUPER_DATA, buffer + 67, buf);
      }
    else
      {
	++p;
	c.mem->puts(SUPER_DATA, buffer + 0, buf.substr(0, p));
	c.mem->puts(SUPER_DATA, buffer + 67, buf.substr(p));
      }
    c.regs.d[0] = 0;

    c.regs.pc += 2;
  }

  void
  dos_open(unsigned int op, context &ec, instruction_data *data)
  {
    uint32_type sp = ec.regs.a[7];
    uint32_type nameptr = ec.mem->getl(SUPER_DATA, sp + 0);
    uint_type mode = ec.mem->getw(SUPER_DATA, sp + 4);
#ifdef L
    L(" DOS _OPEN\n");
#endif

    ec.regs.d[0] = static_cast<dos_exec_context &>(ec).open(nameptr, mode);

    ec.regs.pc += 2;
  }

  void
  dos_print(unsigned int op, context &ec, instruction_data *data)
  {
#ifdef L
    L(" DOS _PRINT\n");
#endif

    uint32_type sp = ec.regs.a[7];
    uint32_type mesptr = ec.mem->getl(SUPER_DATA, sp + 0);

    static_cast<dos_exec_context &>(ec).fputs(mesptr, 1);
    ec.regs.d[0] = 0;		// FIXME: is it correct?

    ec.regs.pc += 2;
  }

  void
  dos_putchar(unsigned int op, context &ec, instruction_data *data)
  {
#ifdef L
    L(" DOS _PUTCHAR\n");
#endif

    uint32_type sp = ec.regs.a[7];
    sint_type code = extsw(ec.mem->getw(SUPER_DATA, sp + 0));

    static_cast<dos_exec_context &>(ec).fputc(code, 1);
    ec.regs.d[0] = 0;		// FIXME: is it correct?

    ec.regs.pc += 2;
  }

  void
  dos_read(unsigned int op, context &ec, instruction_data *data)
  {
#ifdef L
    L(" DOS _READ\n");
    L("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc);
#endif

    uint32 sp = ec.regs.a[7];
    sint_type fd = extsw(ec.mem->getw(SUPER_DATA, sp));
    uint32_type buf = ec.mem->getl(SUPER_DATA, sp + 2);
    uint32_type size = ec.mem->getl(SUPER_DATA, sp + 6);

    ec.regs.d[0] = static_cast<dos_exec_context &>(ec).read(fd, buf, size);

    ec.regs.pc += 2;
  }

  void
  dos_seek(unsigned int op, context &ec, instruction_data *data)
    {
      VL((" DOS _SEEK\n"));

      uint32 sp = ec.regs.a[7];
      int fd = extsw(ec.mem->getw(SUPER_DATA, sp));
      int32 offset = extsl(ec.mem->getl(SUPER_DATA, sp + 2));
      unsigned int whence = ec.mem->getw(SUPER_DATA, sp + 6);
      ec.regs.d[0]
	= static_cast<dos_exec_context &>(ec).seek(fd, offset, whence);

      ec.regs.pc += 2;
    }

  void
  dos_setblock(unsigned int op, context &ec, instruction_data *data)
  {
    uint32_type sp = ec.regs.a[7];
    uint32_type memptr = ec.mem->getl(SUPER_DATA, sp + 0);
    uint32_type newlen = ec.mem->getl(SUPER_DATA, sp + 4);
#ifdef L
    L(" DOS _SETBLOCK\n");
#endif

    ec.regs.d[0] = static_cast<dos_exec_context &>(ec).setblock(memptr, newlen);

    ec.regs.pc += 2;
  }

  void
  dos_vernum(uint_type op, context &ec, instruction_data *data)
  {
#ifdef L
    L(" DOS _VERNUM");
    L("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc);
#endif

    ec.regs.d[0] = (uint32_type(0x3638) << 16 | 3u << 8 | 2u);

    ec.regs.pc += 2;
  }

  void
  dos_write(unsigned int op, context &ec, instruction_data *data)
  {
#ifdef L
    L(" DOS _WRITE");
    L("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc);
#endif

    uint32_type sp = ec.regs.a[7];
    int fd = extsw(ec.mem->getw(SUPER_DATA, sp));
    uint32_type buf = ec.mem->getl(SUPER_DATA, sp + 2);
    uint32_type size = ec.mem->getl(SUPER_DATA, sp + 6);

    ec.regs.d[0] = static_cast<dos_exec_context &>(ec).write(fd, buf, size);

    ec.regs.pc += 2;
  }
} // (unnamed namespace)

dos_exec_context *
dos::create_context()
{
  dos_exec_context *c
    = new dos_exec_context(vm->address_space(), vm->exec_unit(),
			   &allocator, &_fs);
  c->set_debug_level(debug_level);

  return c;
}

dos::dos(machine *m)
  : vm(m),
    allocator(vm->address_space(), 0x8000u, vm->memory_size()),
    debug_level(0)
{
  exec_unit *eu = vm->exec_unit();

  eu->set_instruction(0xff02, 0, &dos_putchar, this);
  eu->set_instruction(0xff09, 0, &dos_print, this);
  eu->set_instruction(0xff0d, 0, &dos_fflush, this);
  eu->set_instruction(0xff1b, 0, &dos_fgetc, this);
  eu->set_instruction(0xff1e, 0, &dos_fputs, this);
  eu->set_instruction(0xff25, 0, &dos_intvcs, this);
  eu->set_instruction(0xff27, 0, &dos_gettim2, this);
  eu->set_instruction(0xff2a, 0, &dos_getdate, this);
  eu->set_instruction(0xff30, 0, &dos_vernum, this);
  eu->set_instruction(0xff37, 0, &dos_nameck, this);
  eu->set_instruction(0xff3c, 0, &dos_create, this);
  eu->set_instruction(0xff3d, 0, &dos_open, this);
  eu->set_instruction(0xff3e, 0, &dos_close, this);
  eu->set_instruction(0xff3f, 0, &dos_read, this);
  eu->set_instruction(0xff40, 0, &dos_write, this);
  eu->set_instruction(0xff41, 0, &dos_delete, this);
  eu->set_instruction(0xff42, 0, &dos_seek, this);
  eu->set_instruction(0xff43, 0, &dos_chmod, this);
  eu->set_instruction(0xff44, 0, &dos_ioctrl, this);
  eu->set_instruction(0xff45, 0, &dos_dup, this);
  eu->set_instruction(0xff48, 0, &dos_malloc, this);
  eu->set_instruction(0xff4a, 0, &dos_setblock, this);
  eu->set_instruction(0xff4c, 0, &dos_exit2, this);
  eu->set_instruction(0xff51, 0, &dos_getpdb, this);
  eu->set_instruction(0xff53, 0, &dos_getenv, this);
  eu->set_instruction(0xff57, 0, &dos_filedate, this);

  eu->set_instruction(0xff81, 0, &dos_getpdb, this);
  eu->set_instruction(0xff83, 0, &dos_getenv, this);
  eu->set_instruction(0xff87, 0, &dos_filedate, this);

  // Dummy NUL device.  LHA scans this for TwentyOne?
  vm->address_space()->putl(SUPER_DATA, 0x6900 +  0, 0x6a00);
  vm->address_space()->putl(SUPER_DATA, 0x6900 + 14, 0x4e554c20);
  vm->address_space()->putl(SUPER_DATA, 0x6900 + 18, 0x20202020);

  vm->address_space()->putl(SUPER_DATA, 0x6a00 +  0, 0xffffffff);
}

