/* Virtual X68000 - Sharp X68000 emulator
   Copyright (C) 1998-2000 Hypercore Software Design, Ltd.

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

#include <vx68k/machine.h>

#include <stdexcept>
#include <cstdio>

#ifdef HAVE_NANA_H
# include <nana.h>
# include <cstdio>
#else
# include <cassert>
# define I assert
#endif

using vx68k::x68k_address_space;
using vx68k::system_rom;
using vm68k::context;
using vm68k::SUPER_DATA;
using namespace vm68k::types;
using namespace std;

uint_type
system_rom::getw(int, uint32_type) const
{
#ifdef HAVE_NANA_H
  L("system_rom: FIXME: `getw' not implemented\n");
#endif
  return 0;
}

uint_type
system_rom::getb(int, uint32_type) const
{
#ifdef HAVE_NANA_H
  L("system_rom: FIXME: `getb' not implemented\n");
#endif
  return 0;
}

size_t
system_rom::read(int, uint32_type, void *, size_t) const
{
#ifdef HAVE_NANA_H
  L("system_rom: FIXME: `read' not implemented\n");
#endif
  return 0;
}

void
system_rom::putw(int, uint32_type, uint_type)
{
#ifdef HAVE_NANA_H
  L("system_rom: FIXME: `putw' not implemented\n");
#endif
}

void
system_rom::putb(int, uint32_type, uint_type)
{
#ifdef HAVE_NANA_H
  L("system_rom: FIXME: `putb' not implemented\n");
#endif
}

size_t
system_rom::write(int, uint32_type, const void *, size_t)
{
#ifdef HAVE_NANA_H
  L("system_rom: FIXME: `write' not implemented\n");
#endif
  return 0;
}

void
system_rom::set_iocs_function(uint_type funcno, const iocs_function_type &f)
{
  if (funcno < 0 || funcno >= iocs_functions.size())
    throw range_error("system_rom");

  iocs_functions[funcno] = f;
}

void
system_rom::dispatch_iocs_function(context &c)
{
  unsigned int funcno = c.regs.d[0] & 0xffu;

  iocs_function_handler handler = iocs_functions[funcno].first;
  I(handler != NULL);

  (*handler)(c, iocs_functions[funcno].second);

  c.regs.pc += 2;
}

namespace
{
  /* Handles an IOCS trap.  This function is an instruction handler.  */
  void
  iocs_trap(uint_type, context &c, unsigned long data)
  {
    system_rom *rom = reinterpret_cast<system_rom *>(data);
    I(rom != NULL);

    rom->dispatch_iocs_function(c);
  }
} // namespace (unnamed)

void
system_rom::attach(exec_unit *eu)
{
  if (attached_eu != NULL)
    throw logic_error("system_rom");

  attached_eu = eu;

  unsigned long data = reinterpret_cast<unsigned long>(this);
  attached_eu->set_instruction(0x4e4f, make_pair(&iocs_trap, data));
}

void
system_rom::detach(exec_unit *eu)
{
  if (eu != attached_eu)
    throw invalid_argument("system_rom");

  attached_eu = NULL;
}

void
system_rom::initialize(address_space &as)
{
#ifdef HAVE_NANA_H
  L("system_rom: FIXME: `initialize' not fully implemented\n");
#endif

  uint32_type f = 0xfc0000;
  for (uint32_type i = 0x400; i != 0x800; i += 4)
    {
      as.putl(SUPER_DATA, i, f);
      f += 4;
    }
}

namespace
{
  using vm68k::byte_size;
  using vm68k::word_size;
  using vm68k::long_word_size;

  /* Handles a _B_COLOR call.  */
  void
  iocs_b_color(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    L("system_rom: _B_COLOR %%d1=%#010x\n", c.regs.d[1]);
#endif
    fprintf(stderr, "iocs_b_color: FIXME: not implemented\n");
    byte_size::put(c.regs.d[0], 3);
  }

  /* Handles a _B_CONSOL call.  */
  void
  iocs_b_consol(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    L("system_rom: _B_CONSOL %%d1=%#010x %%d2=%#010x\n",
      c.regs.d[1], c.regs.d[2]);
#endif
    fprintf(stderr, "iocs_b_consol: FIXME: not implemented\n");
  }

  /* Handles a _B_CUROFF call.  */
  void
  iocs_b_curoff(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    L("system_rom: _B_CUROFF\n");
#endif
    fprintf(stderr, "iocs_b_curoff: FIXME: not implemented\n");
  }

  /* Handles a _B_CURON call.  */
  void
  iocs_b_curon(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    L("system_rom: _B_CURON\n");
#endif
    fprintf(stderr, "iocs_b_curon: FIXME: not implemented\n");
  }

  /* Handles a _B_DRVCHK call.  */
  void
  iocs_b_drvchk(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    L("system_rom: _B_DRVCHK %%d1=%#010x %%d2=%#010x\n",
      c.regs.d[1], c.regs.d[2]);
#endif
    fprintf(stderr, "iocs_b_drvchk: FIXME: not implemented\n");
    if ((c.regs.d[2] & 0xffff) == 8)
      long_word_size::put(c.regs.d[0], 1);
    else
      long_word_size::put(c.regs.d[0], 0x02);
  }

  /* Handles a _B_EJECT call.  */
  void
  iocs_b_eject(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    L("system_rom: _B_EJECT %%d1=%#010x\n", c.regs.d[1]);
#endif
    fprintf(stderr, "iocs_b_eject: FIXME: not implemented\n");
    long_word_size::put(c.regs.d[0], 0);
  }

  /* Handles a _B_LPEEK call.  */
  void
  iocs_b_lpeek(context &c, unsigned long data)
  {
    uint32_type address = c.regs.a[1];
#ifdef HAVE_NANA_H
    L("system_rom: _B_LPEEK %%a1=%#010x\n", address);
#endif

    c.regs.d[0] = c.mem->getl(SUPER_DATA, address);
    c.regs.a[1] = address + 4;
  }

  /* Handles a _B_PRINT call.  */
  void
  iocs_b_print(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    L("system_rom: _B_PRINT %%a1=%#010x\n", c.regs.a[1]);
#endif
    uint32_type address = c.regs.a[1];

    x68k_address_space *as = dynamic_cast<x68k_address_space *>(c.mem);
    as->machine()->b_print(c.mem, address);
  }

  /* Handles a _B_PUTMES call.  */
  void
  iocs_b_putmes(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    L("system_rom: _B_PUTMES %%d1=%#010x %%d2=%#010x %%d3=%#010x %%d4=%#010x %%a1=%#010x\n",
      c.regs.d[1], c.regs.d[2], c.regs.d[3], c.regs.d[4], c.regs.a[1]);
#endif
    fprintf(stderr, "iocs_b_putmes: FIXME: not implemented\n");
  }

  /* Handles a _B_READ call.  */
  void
  iocs_b_read(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    L("system_rom: _B_READ %%d1=%#010x %%d2=%#010x %%d3=%#010x %%a1=%#010x\n",
      c.regs.d[1], c.regs.d[2], c.regs.d[3], c.regs.a[1]);
#endif

    x68k_address_space *as = dynamic_cast<x68k_address_space *>(c.mem);
    c.regs.d[0] = as->machine()->read_disk(*c.mem,
					   c.regs.d[1] & 0xffffu, c.regs.d[2],
					   c.regs.a[1], c.regs.d[3]);
    // FIXME?
  }

  /* Handles a _B_RECALI call.  */
  void
  iocs_b_recali(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    L("system_rom: _B_RECALI %%d1=%#010x\n", c.regs.d[1]);
#endif
    fprintf(stderr, "iocs_b_recali: FIXME: not implemented\n");
    if ((c.regs.d[1] & 0xf000) == 0x9000 &&
	(c.regs.d[1] & 0x0f00) < 0x0200)
      long_word_size::put(c.regs.d[0], 0);
    else
      long_word_size::put(c.regs.d[0], -1);
  }

  /* Handles a _B_SFTSNS call.  */
  void
  iocs_b_sftsns(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    L("system_rom: _BSFTSNS\n");
#endif
    fprintf(stderr, "iocs_b_sftsns: FIXME: not implemented\n");
    c.regs.d[0] = 0;
  }

  /* Handles a _B_WRITE call.  */
  void
  iocs_b_write(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    L("system_rom: _B_WRITE %%d1=%#06x %%d2=%#010x %%d3=%#010x %%a1=%#010x\n",
      c.regs.d[1] & 0xffffu, c.regs.d[2], c.regs.d[3], c.regs.a[1]);
#endif
    fprintf(stderr, "iocs_b_write: FIXME: not implemented\n");
  }

  /* Handles a _BOOTINF call.  */
  void
  iocs_bootinf(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    L("system_rom: _BOOTINF\n");
#endif
    c.regs.d[0] = 0x90;
  }

  /* Handles a _CRTMOD call.  */
  void
  iocs_crtmod(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    L("system_rom: _CRTMOD %%d1=%#010x\n", c.regs.d[1]);
#endif
    fprintf(stderr, "iocs_crtmod: FIXME: not implemented\n");
    c.regs.d[0] = 16;
  }

  /* Handles a _DATEBIN call.  */
  void
  iocs_datebin(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    L("system_rom: _DATEBIN %%d1=%#010x\n", c.regs.d[1]);
#endif
    uint32_type bcd = long_word_size::get(c.regs.d[1]);
    unsigned int mday = bcd & 0xffu;
    unsigned int mon = (bcd >>= 8) & 0xffu;
    unsigned int year = (bcd >>= 8) & 0xffu;
    unsigned int wday = (bcd >>= 8) & 0xffu;

    uint32_type bin = ((((wday << 8)
			 + year - year / 16u * 6u << 8)
			+ mon - mon / 16u * 6u << 8)
		       + mday - mday / 16u * 6u);
    long_word_size::put(c.regs.d[0], bin);
  }

  /* Handles a _DATEGET call.  */
  void
  iocs_dateget(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    L("system_rom: _DATEGET\n");
#endif
    time_t t = time(NULL);
    struct tm *lt = localtime(&t);

    unsigned int mday = lt->tm_mday;
    unsigned int mon = lt->tm_mon + 1;
    unsigned int year = lt->tm_year % 100;
    unsigned int wday = lt->tm_wday;

    uint32_type bcd = ((((wday << 8)
			 + year + year / 10u * 6u << 8)
			+ mon + mon / 10u * 6u << 8)
		       + mday + mday / 10u * 6u);
    long_word_size::put(c.regs.d[0], bcd);
  }

  /* Handles a _DEFCHR call.  */
  void
  iocs_defchr(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    L("system_rom: _DEFCHR %%d1=%#010x %%a1=%#010x\n",
      c.regs.d[1], c.regs.a[1]);
#endif
    fprintf(stderr, "iocs_defchr: FIXME: not implemented\n");
    c.regs.d[0] = 0;
  }

  /* Handles a _INIT_PRN call.  */
  void
  iocs_init_prn(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    L("system_rom: _INIT_PRN %%d1=%#010x\n", c.regs.d[1]);
#endif
    fprintf(stderr, "iocs_init_prn: FIXME: not implemented\n");
    c.regs.d[0] = 0;
  }

  /* Handles a _OS_CUROF call.  */
  void
  iocs_os_curof(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    L("system_rom: _OS_CUROF\n");
#endif
    fprintf(stderr, "iocs_os_curof: FIXME: not implemented\n");
  }

  /* Handles a _SET232C call.  */
  void
  iocs_set232c(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    L("system_rom: _SET232C %%d1=%#010x\n", c.regs.d[1]);
#endif
    fprintf(stderr, "iocs_set232c: FIXME: not implemented\n");
    c.regs.d[0] = 0;
  }

  /* Handles a _TCOLOR call.  */
  void
  iocs_tcolor(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    L("system_rom: _B_TCOLOR %%d1=%#010x\n",
      c.regs.d[1]);
#endif
    fprintf(stderr, "iocs_tcolor: FIXME: not implemented\n");
  }

  /* Handles a _TEXTPUT call.  */
  void
  iocs_textput(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    L("system_rom: _B_TEXTPUT %%d1=%#010x %%d2=%#010x %%a1=%#010x\n",
      c.regs.d[1], c.regs.d[2], c.regs.a[1]);
#endif
    fprintf(stderr, "iocs_textput: FIXME: not implemented\n");
  }

  /* Handles a _TIMEBIN call.  */
  void
  iocs_timebin(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    L("system_rom: _TIMEBIN %%d1=%#010x\n", c.regs.d[1]);
#endif
    uint32_type bcd = long_word_size::get(c.regs.d[1]);
    unsigned int sec = bcd & 0xffu;
    unsigned int min = (bcd >>= 8) & 0xffu;
    unsigned int hour = (bcd >>= 8) & 0xffu;

    uint32_type bin = (((hour - hour / 16u * 6u << 8)
			+ min - min / 16u * 6u << 8)
		       + sec - sec / 16u * 6u);
    long_word_size::put(c.regs.d[0], bin);
  }

  /* Handles a _TIMEGET call.  */
  void
  iocs_timeget(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    L("system_rom: _TIMEGET\n");
#endif
    time_t t = time(NULL);
    struct tm *lt = localtime(&t);

    unsigned int sec = lt->tm_sec;
    unsigned int min = lt->tm_min;
    unsigned int hour = lt->tm_hour;

    uint32_type bcd = (((hour + hour / 10u * 6u << 8)
			+ min + min / 10u * 6u << 8)
		       + sec + sec / 10u * 6u);
    long_word_size::put(c.regs.d[0], bcd);
  }

  /* Handles a _TIMERDST call.  */
  void
  iocs_timerdst(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    L("system_rom: _TIMERDST %%d1=%#010x %%a1=%#010x\n",
      c.regs.d[1], c.regs.a[1]);
#endif
    fprintf(stderr, "iocs_timerdst: FIXME: not implemented\n");
    long_word_size::put(c.regs.d[0], 0);
  }

  /* Handles a _TPALET call.  */
  void
  iocs_tpalet(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    L("system_rom: _B_TPALET %%d1=%#010x %%d2=%#010x\n",
      c.regs.d[1], c.regs.d[2]);
#endif
    fprintf(stderr, "iocs_tpalet: FIXME: not implemented\n");
  }

  /* Handles a 0x37 call.  */
  void
  iocs_x37(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    L("system_rom: 0x37 %%d1=%#010x\n", c.regs.d[1]);
#endif
    fprintf(stderr, "iocs_x37: FIXME: not implemented\n");
  }

  /* Handles a 0x38 call.  */
  void
  iocs_x38(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    L("system_rom: 0x38 %%d1=%#010x %%d2=%#010x\n",
      c.regs.d[1], c.regs.d[2]);
#endif
    fprintf(stderr, "iocs_x38: FIXME: not implemented\n");
  }

  /* Handles a 0x39 call.  */
  void
  iocs_x39(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    L("system_rom: 0x39 %%d1=%#010x %%d2=%#010x\n",
      c.regs.d[1], c.regs.d[2]);
#endif
    fprintf(stderr, "iocs_x39: FIXME: not implemented\n");
    long_word_size::put(c.regs.d[1], 0);
    word_size::put(c.regs.d[2], 0);
  }

  /* Initializes the IOCS functions.  */
  void
  initialize_iocs_functions(system_rom *rom)
  {
    typedef system_rom::iocs_function_type iocs_function_type;

    rom->set_iocs_function(0x02, iocs_function_type(&iocs_b_sftsns, 0));
    rom->set_iocs_function(0x0f, iocs_function_type(&iocs_defchr, 0));
    rom->set_iocs_function(0x10, iocs_function_type(&iocs_crtmod, 0));
    rom->set_iocs_function(0x13, iocs_function_type(&iocs_tpalet, 0));
    rom->set_iocs_function(0x15, iocs_function_type(&iocs_tcolor, 0));
    rom->set_iocs_function(0x1b, iocs_function_type(&iocs_textput, 0));
    rom->set_iocs_function(0x1e, iocs_function_type(&iocs_b_curon, 0));
    rom->set_iocs_function(0x1f, iocs_function_type(&iocs_b_curoff, 0));
    rom->set_iocs_function(0x21, iocs_function_type(&iocs_b_print, 0));
    rom->set_iocs_function(0x22, iocs_function_type(&iocs_b_color, 0));
    rom->set_iocs_function(0x2e, iocs_function_type(&iocs_b_consol, 0));
    rom->set_iocs_function(0x2f, iocs_function_type(&iocs_b_putmes, 0));
    rom->set_iocs_function(0x30, iocs_function_type(&iocs_set232c, 0));
    rom->set_iocs_function(0x37, iocs_function_type(&iocs_x37, 0));
    rom->set_iocs_function(0x38, iocs_function_type(&iocs_x38, 0));
    rom->set_iocs_function(0x39, iocs_function_type(&iocs_x39, 0));
    rom->set_iocs_function(0x3c, iocs_function_type(&iocs_init_prn, 0));
    rom->set_iocs_function(0x45, iocs_function_type(&iocs_b_write, 0));
    rom->set_iocs_function(0x46, iocs_function_type(&iocs_b_read, 0));
    rom->set_iocs_function(0x47, iocs_function_type(&iocs_b_recali, 0));
    rom->set_iocs_function(0x4e, iocs_function_type(&iocs_b_drvchk, 0));
    rom->set_iocs_function(0x4f, iocs_function_type(&iocs_b_eject, 0));
    rom->set_iocs_function(0x54, iocs_function_type(&iocs_dateget, 0));
    rom->set_iocs_function(0x55, iocs_function_type(&iocs_datebin, 0));
    rom->set_iocs_function(0x56, iocs_function_type(&iocs_timeget, 0));
    rom->set_iocs_function(0x57, iocs_function_type(&iocs_timebin, 0));
    rom->set_iocs_function(0x6b, iocs_function_type(&iocs_timerdst, 0));
    rom->set_iocs_function(0x84, iocs_function_type(&iocs_b_lpeek, 0));
    rom->set_iocs_function(0x8e, iocs_function_type(&iocs_bootinf, 0));
    rom->set_iocs_function(0xaf, iocs_function_type(&iocs_os_curof, 0));
  }
} // namespace (unnamed)

system_rom::~system_rom()
{
  detach(attached_eu);
}

system_rom::system_rom()
  : iocs_functions(0x100, iocs_function_type(&invalid_iocs_function, 0)),
    attached_eu(NULL)
{
  initialize_iocs_functions(this);
}

void
system_rom::invalid_iocs_function(context &c, unsigned long data)
{
#ifdef HAVE_NANA_H
  L("system_rom: IOCS %#04x\n", c.regs.d[0] & 0xffu);
#endif
  throw runtime_error("invalid iocs function");	// FIXME
}
