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

#include <vx68k/human.h>

#include <ctime>
#include <cstring>

#ifdef HAVE_NANA_H
# include <nana.h>
# include <cstdio>
#else
# include <cassert>
# define I assert
#endif

using namespace vx68k::human;
using namespace vm68k;
using namespace std;

namespace
{
  void
  dos_chmod(uint16_type op, context &c, unsigned long data)
  {
    uint32_type sp = c.regs.a[7];
    uint32_type nameptr = c.mem->get_32(memory::SUPER_DATA, sp + 0);
    sint_type atr = word_size::svalue(c.mem->get_16(memory::SUPER_DATA, sp + 4));
#ifdef L
    L(" DOS _CHMOD\n");
#endif

    dos *d = reinterpret_cast<dos *>(data);
    I(d != NULL);
    c.regs.d[0] = d->fs()->chmod(c.mem, nameptr, atr);

    c.regs.pc += 2;
  }

  void
  dos_close(uint16_type op, context &c, unsigned long data)
  {
    uint32_type sp = c.regs.a[7];
    sint_type fd = word_size::svalue(c.mem->get_16(memory::SUPER_DATA, sp));
#ifdef L
    L(" DOS _CLOSE\n");
#endif

    c.regs.d[0] = static_cast<dos_exec_context &>(c).close(fd);

    c.regs.pc += 2;
  }

  void
  dos_create(uint16_type op, context &c, unsigned long data)
  {
    uint32_type sp = c.regs.a[7];
    uint32_type nameptr = c.mem->get_32(memory::SUPER_DATA, sp + 0);
    uint_type atr = c.mem->get_16(memory::SUPER_DATA, sp + 4);
#ifdef L
    L(" DOS _CREATE\n");
#endif

    c.regs.d[0] = static_cast<dos_exec_context &>(c).create(nameptr, atr);

    c.regs.pc += 2;
  }

  void
  dos_delete(uint16_type op, context &c, unsigned long data)
  {
#ifdef L
    L(" DOS _DELETE\n");
#endif

    // FIXME.
    c.regs.d[0] = 0;

    c.regs.pc += 2;
  }

  void
  dos_dup(uint16_type op, context &c, unsigned long data)
  {
    uint32_type sp = c.regs.a[7];
    uint_type filno = c.mem->get_16(memory::SUPER_DATA, sp);
#ifdef L
    L(" DOS _DUP\n");
#endif

    c.regs.d[0] = static_cast<dos_exec_context &>(c).dup(filno);

    c.regs.pc += 2;
  }

  void
  dos_exit2(uint16_type op, context &c, unsigned long data)
  {
    uint32_type sp = c.regs.a[7];
    unsigned int status = c.mem->get_16(memory::SUPER_DATA, sp + 0);
#ifdef L
    L(" DOS _EXIT2\n");
#endif

    static_cast<dos_exec_context &>(c).exit(status);
  }

  void
  dos_fflush(uint16_type op, context &c, unsigned long data)
  {
#ifdef L
    L(" DOS _FFLUSH\n");
#endif

    dos *d = reinterpret_cast<dos *>(data);
    I(d != NULL);
    // FIXME

    c.regs.pc += 2;
  }

  void
  dos_filedate(uint16_type op, context &c, unsigned long data)
  {
#ifdef L
    L(" DOS _FILEDATE\n");
#endif

    // FIXME.
    c.regs.d[0] = 0;

    c.regs.pc += 2;
  }

  void
  dos_fgetc(uint16_type op, context &c, unsigned long data)
  {
    uint32_type sp = c.regs.a[7];
    int fd = word_size::svalue(c.mem->get_16(memory::SUPER_DATA, sp));
#ifdef L
    L(" DOS _FGETC\n");
#endif

    c.regs.d[0] = static_cast<dos_exec_context &>(c).fgetc(fd);

    c.regs.pc += 2;
  }

  void
  dos_fputs(uint16_type op, context &c, unsigned long data)
  {
    uint32_type sp = c.regs.a[7];
    uint32_type mesptr = c.mem->get_32(memory::SUPER_DATA, sp + 0);
    sint_type filno = word_size::svalue(c.mem->get_16(memory::SUPER_DATA, sp + 4));
#ifdef L
    L(" DOS _FPUTS\n");
#endif

    c.regs.d[0] = static_cast<dos_exec_context &>(c).fputs(mesptr, filno);

    c.regs.pc += 2;
  }

  /* Handles DOS _GETC function.  This function is similar to DOS
     _GETCHAR except no echo-back is made.  */
  void
  dos_getc(uint16_type op, context &c, unsigned long data)
  {
#ifdef L
    L(" DOS _GETC\n");
#endif

    // FIXME
    c.regs.d[0] = static_cast<dos_exec_context &>(c).fgetc(0);

    c.regs.pc += 2;
  }

  void
  dos_getdate(uint16_type op, context &c, unsigned long data)
  {
#ifdef L
    L(" DOS _GETDATE\n");
#endif

    time_t t = time(NULL);
#ifdef HAVE_LOCALTIME_R
    struct tm lt0;
    struct tm *lt = localtime_r(&t, &lt0);
#else
    struct tm *lt = localtime(&t);
#endif

    c.regs.d[0] = (lt->tm_wday << 16
		   | lt->tm_year - 80 << 9
		   | lt->tm_mon + 1 << 5
		   | lt->tm_mday);

    c.regs.pc += 2;
  }

  void
  dos_getenv(uint16_type op, context &c, unsigned long data)
  {
    uint32_type sp = c.regs.a[7];
    uint32_type getname = c.mem->get_16(memory::SUPER_DATA, sp + 0);
    uint32_type env = c.mem->get_16(memory::SUPER_DATA, sp + 4);
    uint32_type getbuf = c.mem->get_16(memory::SUPER_DATA, sp + 8);
#ifdef L
    L(" DOS _GETENV\n");
#endif

    c.regs.d[0]
      = static_cast<dos_exec_context &>(c).getenv(getname, env, getbuf);

    c.regs.pc += 2;
  }

  void
  dos_getpdb(uint16_type op, context &c, unsigned long data)
  {
#ifdef L
    L(" DOS _GETPDB\n");
#endif

    c.regs.d[0] = static_cast<dos_exec_context &>(c).getpdb();

    c.regs.pc += 2;
  }

  void
  dos_gettim2(uint16_type op, context &c, unsigned long data)
  {
#ifdef L
    L(" DOS _GETTIM2\n");
#endif

    time_t t = time(NULL);
#ifdef HAVE_LOCALTIME_R
    struct tm lt0;
    struct tm *lt = localtime_r(&t, &lt0);
#else
    struct tm *lt = localtime(&t);
#endif

    c.regs.d[0] = (lt->tm_hour << 16
		   | lt->tm_min << 8
		   | lt->tm_sec);

    c.regs.pc += 2;
  }

  void
  dos_intvcs(uint16_type op, context &c, unsigned long data)
  {
#ifdef L
    L(" DOS _INTVCS\n");
#endif

    // FIXME.
    c.regs.d[0] = 0;

    c.regs.pc += 2;
  }

  void
  dos_ioctrl(uint16_type op, context &c, unsigned long data)
  {
#ifdef L
    L(" DOS _IOCTRL\n");
#endif

    // FIXME.
    c.regs.d[0] = 0;

    c.regs.pc += 2;
  }

  void
  dos_malloc(uint16_type op, context &c, unsigned long data)
  {
    uint32_type sp = c.regs.a[7];
    uint32_type len = c.mem->get_32(memory::SUPER_DATA, sp + 0);
#ifdef L
    L(" DOS _MALLOC\n");
#endif

    c.regs.d[0] = static_cast<dos_exec_context &>(c).malloc(len);

    c.regs.pc += 2;
  }

  void
  dos_nameck(uint16_type op, context &c, unsigned long data)
  {
    uint32_type sp = c.regs.a[7];
    uint32_type file = c.mem->get_32(memory::SUPER_DATA, sp + 0);
    uint32_type buffer = c.mem->get_32(memory::SUPER_DATA, sp + 4);
#ifdef L
    L(" DOS _NAMECK\n");
#endif

    dos *d = reinterpret_cast<dos *>(data);
    I(d != NULL);

    // FIXME
    string buf = c.mem->get_string(memory::SUPER_DATA, file);
    string::size_type p = buf.find_last_of(buf, '/');
    if (p == string::npos)
      {
	c.mem->put_string(memory::SUPER_DATA, buffer + 0, "./");
	c.mem->put_string(memory::SUPER_DATA, buffer + 67, buf);
      }
    else
      {
	++p;
	c.mem->put_string(memory::SUPER_DATA, buffer + 0, buf.substr(0, p));
	c.mem->put_string(memory::SUPER_DATA, buffer + 67, buf.substr(p));
      }
    c.regs.d[0] = 0;

    c.regs.pc += 2;
  }

  void
  dos_open(uint16_type op, context &c, unsigned long data)
  {
    uint32_type sp = c.regs.a[7];
    uint32_type nameptr = c.mem->get_32(memory::SUPER_DATA, sp + 0);
    uint_type mode = c.mem->get_16(memory::SUPER_DATA, sp + 4);
#ifdef L
    L(" DOS _OPEN\n");
#endif

    c.regs.d[0] = static_cast<dos_exec_context &>(c).open(nameptr, mode);

    c.regs.pc += 2;
  }

  void
  dos_print(uint16_type op, context &c, unsigned long data)
  {
    uint32_type sp = c.regs.a[7];
    uint32_type mesptr = c.mem->get_32(memory::SUPER_DATA, sp + 0);
#ifdef L
    L(" DOS _PRINT\n");
#endif

    static_cast<dos_exec_context &>(c).fputs(mesptr, 1);
    c.regs.d[0] = 0;		// FIXME: is it correct?

    c.regs.pc += 2;
  }

  void
  dos_putchar(uint16_type op, context &c, unsigned long data)
  {
    uint32_type sp = c.regs.a[7];
    sint_type code = word_size::svalue(c.mem->get_16(memory::SUPER_DATA, sp + 0));
#ifdef L
    L(" DOS _PUTCHAR\n");
#endif

    static_cast<dos_exec_context &>(c).fputc(code, 1);
    c.regs.d[0] = 0;		// FIXME: is it correct?

    c.regs.pc += 2;
  }

  void
  dos_read(uint16_type op, context &c, unsigned long data)
  {
    uint32_type sp = c.regs.a[7];
    sint_type fd = word_size::svalue(c.mem->get_16(memory::SUPER_DATA, sp));
    uint32_type buf = c.mem->get_32(memory::SUPER_DATA, sp + 2);
    uint32_type size = c.mem->get_32(memory::SUPER_DATA, sp + 6);
#ifdef L
    L(" DOS _READ\n");
#endif

    c.regs.d[0] = static_cast<dos_exec_context &>(c).read(fd, buf, size);

    c.regs.pc += 2;
  }

  void
  dos_seek(uint16_type op, context &c, unsigned long data)
  {
    uint32_type sp = c.regs.a[7];
    int fd = word_size::svalue(c.mem->get_16(memory::SUPER_DATA, sp));
    sint32_type offset = long_word_size::svalue(c.mem->get_32(memory::SUPER_DATA, sp + 2));
    unsigned int whence = c.mem->get_16(memory::SUPER_DATA, sp + 6);
#ifdef L
    L(" DOS _SEEK\n");
#endif

    c.regs.d[0]
      = static_cast<dos_exec_context &>(c).seek(fd, offset, whence);

    c.regs.pc += 2;
  }

  void
  dos_setblock(uint16_type op, context &c, unsigned long data)
  {
    uint32_type sp = c.regs.a[7];
    uint32_type memptr = c.mem->get_32(memory::SUPER_DATA, sp + 0);
    uint32_type newlen = c.mem->get_32(memory::SUPER_DATA, sp + 4);
#ifdef L
    L(" DOS _SETBLOCK\n");
#endif

    c.regs.d[0] = static_cast<dos_exec_context &>(c).setblock(memptr, newlen);

    c.regs.pc += 2;
  }

  /* Handles DOS call _SUPER.  */
  void
  dos_super(uint16_type op, context &c, unsigned long data)
  {
    uint32_type sp = c.regs.a[7];
    uint32_type stack = c.mem->get_32(memory::SUPER_DATA, sp + 0);
#ifdef L
    L(" DOS _SUPER\n");
#endif

    if (stack != 0)
      {
	if (c.supervisor_state())
	  {
	    c.regs.usp = c.regs.a[7];
	    c.regs.a[7] = stack;
	    c.set_supervisor_state(false);
	  }
      }
    else
      {
	if (c.supervisor_state())
	  {
	    c.regs.d[0] = uint32_type(-26);
	  }
	else
	  {
	    c.set_supervisor_state(true);
	    c.regs.d[0] = c.regs.a[7];
	    c.regs.a[7] = c.regs.usp;
	  }
      }

    c.regs.pc += 2;
  }

  /* Handles DOS _SUPER_JSR syscall.  */
  void
  dos_super_jsr(uint16_type op, context &c, unsigned long data)
  {
    uint32_type sp = c.regs.a[7];
    uint32_type empadr = c.mem->get_32(memory::SUPER_DATA, sp + 0);
#ifdef L
    L(" DOS _SUPER_JSR\t| %#lx\n", (unsigned long) empadr);
#endif

    uint32_type last_pc = c.regs.pc;
    bool last_state = c.supervisor_state();
    c.set_supervisor_state(true);
    c.regs.pc = empadr;

    c.mem->put_32(memory::SUPER_DATA, c.regs.a[7] - 4, 0xfef600);
    c.regs.a[7] -= 4;

    try
      {
	vx68k::x68k_address_space *as
	  = dynamic_cast<vx68k::x68k_address_space *>(c.mem);
	I(as != NULL);

	as->machine()->exec_unit()->run(c);

	abort();
      }
    catch (bus_error_exception &e)
      {
	if (e.address != 0xfef600)
	  throw;
      }

    c.set_supervisor_state(last_state);
    c.regs.pc = last_pc;

    c.regs.pc += 2;
  }

  void
  dos_vernum(uint16_type op, context &c, unsigned long data)
  {
#ifdef L
    L(" DOS _VERNUM\n");
#endif

    c.regs.d[0] = (uint32_type(0x3638) << 16 | 3u << 8 | 2u);

    c.regs.pc += 2;
  }

  void
  dos_write(uint16_type op, context &c, unsigned long data)
  {
    uint32_type sp = c.regs.a[7];
    int fd = word_size::svalue(c.mem->get_16(memory::SUPER_DATA, sp));
    uint32_type buf = c.mem->get_32(memory::SUPER_DATA, sp + 2);
    uint32_type size = c.mem->get_32(memory::SUPER_DATA, sp + 6);
#ifdef L
    L(" DOS _WRITE\n");
#endif

    c.regs.d[0] = static_cast<dos_exec_context &>(c).write(fd, buf, size);

    c.regs.pc += 2;
  }

  /* Adds DOS-call instructions to exec_unit EU.  */
  void
  add_instructions(exec_unit &eu, dos *d)
  {
    unsigned long data = reinterpret_cast<unsigned long>(d);
    eu.set_instruction(0xff02u, make_pair(&dos_putchar, data));
    eu.set_instruction(0xff08u, make_pair(&dos_getc, data));
    eu.set_instruction(0xff09u, make_pair(&dos_print, data));
    eu.set_instruction(0xff0du, make_pair(&dos_fflush, data));
    eu.set_instruction(0xff1bu, make_pair(&dos_fgetc, data));
    eu.set_instruction(0xff1eu, make_pair(&dos_fputs, data));
    eu.set_instruction(0xff20u, make_pair(&dos_super, data));
    eu.set_instruction(0xff25u, make_pair(&dos_intvcs, data));
    eu.set_instruction(0xff27u, make_pair(&dos_gettim2, data));
    eu.set_instruction(0xff2au, make_pair(&dos_getdate, data));
    eu.set_instruction(0xff30u, make_pair(&dos_vernum, data));
    eu.set_instruction(0xff37u, make_pair(&dos_nameck, data));
    eu.set_instruction(0xff3cu, make_pair(&dos_create, data));
    eu.set_instruction(0xff3du, make_pair(&dos_open, data));
    eu.set_instruction(0xff3eu, make_pair(&dos_close, data));
    eu.set_instruction(0xff3fu, make_pair(&dos_read, data));
    eu.set_instruction(0xff40u, make_pair(&dos_write, data));
    eu.set_instruction(0xff41u, make_pair(&dos_delete, data));
    eu.set_instruction(0xff42u, make_pair(&dos_seek, data));
    eu.set_instruction(0xff43u, make_pair(&dos_chmod, data));
    eu.set_instruction(0xff44u, make_pair(&dos_ioctrl, data));
    eu.set_instruction(0xff45u, make_pair(&dos_dup, data));
    eu.set_instruction(0xff48u, make_pair(&dos_malloc, data));
    eu.set_instruction(0xff4au, make_pair(&dos_setblock, data));
    eu.set_instruction(0xff4cu, make_pair(&dos_exit2, data));
    eu.set_instruction(0xff51u, make_pair(&dos_getpdb, data));
    eu.set_instruction(0xff53u, make_pair(&dos_getenv, data));
    eu.set_instruction(0xff57u, make_pair(&dos_filedate, data));
    eu.set_instruction(0xfff6u, make_pair(&dos_super_jsr, data));

    eu.set_instruction(0xff81u, make_pair(&dos_getpdb, data));
    eu.set_instruction(0xff83u, make_pair(&dos_getenv, data));
    eu.set_instruction(0xff87u, make_pair(&dos_filedate, data));
  }
} // (unnamed namespace)

dos_exec_context *
dos::create_context()
{
  dos_exec_context *c
    = new dos_exec_context(&as, as.machine()->exec_unit(), &allocator, &_fs);
  c->set_debug_level(debug_level);

  return c;
}

dos::dos(class machine *m)
  : as(m),
    allocator(&as, 0x8000u, as.machine()->memory_size()),
    _fs(as.machine()),
    debug_level(0)
{
  add_instructions(*as.machine()->exec_unit(), this);

  // Dummy NUL device.  LHA scans this for TwentyOne?
  as.put_32(memory::SUPER_DATA, 0x6900 +  0, 0x6a00);
  as.put_32(memory::SUPER_DATA, 0x6900 + 14, 0x4e554c20);
  as.put_32(memory::SUPER_DATA, 0x6900 + 18, 0x20202020);

  as.put_32(memory::SUPER_DATA, 0x6a00 +  0, 0xffffffff);
}
