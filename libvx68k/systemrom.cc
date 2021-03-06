/* Virtual X68000 - X68000 virtual machine
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
#else
# include <cassert>
# define I assert
#endif

using vx68k::x68k_address_space;
using vx68k::system_rom;
using vm68k::context;
using vm68k::memory;
using namespace vm68k::types;
using namespace std;

#ifdef HAVE_NANA_H
bool nana_iocs_call_trace = false;
#endif

uint16_type
system_rom::get_16(uint32_type address, function_code fc) const
  throw (memory_exception)
{
#ifdef DL
  DL("class system_rom: get_16: fc=%d address=0x%08lx\n",
     fc, (unsigned long) address);
#endif
#if 0
  if (fc == USER_DATA || fc == USER_PROGRAM)
    throw bus_error_exception(true, fc, address);
#endif

  if (address >= 0xfe0400 && address < 0xfe0800)
    return 0xf84f;
  else
    {
      static bool once;
      if (!once++)
	fprintf(stderr, "class system_rom: FIXME: `get_16' not implemented\n");
      return 0x4e73;
    }
}

int
system_rom::get_8(uint32_type address, function_code fc) const
  throw (memory_exception)
{
#ifdef DL
  DL("class system_rom: get_8: fc=%d address=0x%08lx\n",
     fc, (unsigned long) address);
#endif
  // A program access generates a bus error to detect emulation bugs
  // easily.
  if (fc == SUPER_PROGRAM || fc == USER_PROGRAM)
    throw bus_error(address, READ | fc);

  static bool once;
  if (!once++)
    fprintf(stderr, "class system_rom: FIXME: `get_8' not implemented\n");
  return 0;
}

void
system_rom::put_16(uint32_type address, uint16_type value, function_code fc)
  throw (memory_exception)
{
#ifdef DL
  DL("class system_rom: put_16: fc=%d address=0x%08lx value=0x%04x\n",
     fc, (unsigned long) address, value & 0xffffu);
#endif
  if (fc != SUPER_DATA)
    throw bus_error(address, WRITE | fc);

  static bool once;
  if (!once++)
    fprintf(stderr, "class system_rom: FIXME: `put_16' not implemented\n");
}

void
system_rom::put_8(uint32_type address, int value, function_code fc)
  throw (memory_exception)
{
#ifdef DL
  DL("class system_rom: put_8: fc=%d address=0x%08lx value=0x%02x\n",
     fc, (unsigned long) address, value & 0xffu);
#endif
  if (fc != SUPER_DATA)
    throw bus_error(address, WRITE | fc);

  static bool once;
  if (!once++)
    fprintf(stderr, "class system_rom: FIXME: `put_8' not implemented\n");
}

void
system_rom::set_iocs_call(int funcno, const iocs_call_type &f)
{
  if (funcno < 0 || funcno >= iocs_calls.size())
    throw range_error("system_rom");

  iocs_calls[funcno] = f;
}

void
system_rom::call_iocs(int funcno, context &c)
{
  funcno %= 0x100;

  iocs_call_handler handler = iocs_calls[funcno].first;
  I(handler != NULL);

  (*handler)(c, iocs_calls[funcno].second);
}

namespace
{
  using vm68k::byte_size;
  using vm68k::long_word_size;

  /* Handles an IOCS trap.  This function is an instruction handler.  */
  void
  iocs_trap(uint16_type, context &c, unsigned long data)
  {
    sched_yield();
    pthread_testcancel();

    uint32_type vecaddr = (15u + 32u) * 4u;
    uint32_type addr = c.mem->get_32(vecaddr, memory::SUPER_DATA);
    if (addr != vecaddr + 0xfe0000)
      {
#ifdef DL
	DL("iocs_trap: Installed TRAP handler used\n");
#endif
	uint16_type oldsr = c.sr();
	c.set_supervisor_state(true);
	c.regs.a[7] -= 6;
	c.mem->put_32(c.regs.a[7] + 2, c.regs.pc + 2, memory::SUPER_DATA);
	c.mem->put_16(c.regs.a[7] + 0, oldsr, memory::SUPER_DATA);
	c.regs.pc = addr;
      }
    else
      {
	unsigned int callno = byte_size::uvalue(byte_size::get(c.regs.d[0]));

	uint32_type call_address = (callno + 0x100U) * 4U;
	uint32_type call_handler = c.mem->get_32(call_address,
						 memory::SUPER_DATA);
	if (call_handler != call_address + 0xfe0000)
	  {
#ifdef DL
	    DL("iocs_trap: Installed IOCS call handler used\n");
#endif
	    uint16_type oldsr = c.sr();
	    c.set_supervisor_state(true);
	    c.regs.a[7] -= 10;
	    c.mem->put_32(c.regs.a[7] + 6, c.regs.pc + 2, memory::SUPER_DATA);
	    c.mem->put_16(c.regs.a[7] + 4, oldsr, memory::SUPER_DATA);
	    c.mem->put_32(c.regs.a[7] + 0, 0xfe0000, memory::SUPER_DATA);

	    c.regs.pc = call_handler;
	  }
	else
	  {
	    system_rom *rom = reinterpret_cast<system_rom *>(data);
	    I(rom != NULL);

	    rom->call_iocs(callno, c);
	    c.regs.pc += 2;
	  }
      }
  }

  /* Handles a special IOCS invocation.  This instruction calls a
     internal IOCS handler and executes a return.  The IOCS call
     number is derived from the current value of the PC.  */
  void
  x68k_iocs(uint16_type, context &c, unsigned long data)
  {
    system_rom *rom = reinterpret_cast<system_rom *>(data);
    I(rom != NULL);

    int callno = (c.regs.pc - 0xfe0400) / 4;
    rom->call_iocs(callno, c);

    c.regs.pc = long_word_size::get(*c.mem, memory::SUPER_DATA,
				    long_word_size::get(c.regs.a[7]));
    long_word_size::put(c.regs.a[7],
			long_word_size::get(c.regs.a[7]
					    + long_word_size::value_size()));
  }
} // namespace (unnamed)

void
system_rom::attach(processor *eu)
{
  if (attached_eu != NULL)
    throw logic_error("system_rom");

  attached_eu = eu;

  unsigned long data = reinterpret_cast<unsigned long>(this);
  attached_eu->set_instruction(0x4e4f, make_pair(&iocs_trap, data));
  attached_eu->set_instruction(0xf84f, make_pair(&x68k_iocs, data));
}

void
system_rom::detach(processor *eu)
{
  if (eu != attached_eu)
    throw invalid_argument("system_rom");

  attached_eu = NULL;
}

void
system_rom::initialize(memory_map &as)
{
#ifdef DL
  DL("system_rom: FIXME: `initialize' not fully implemented\n");
#endif

  uint32_type f = 0xfe0000;
  for (uint32_type i = 0u; i != 0x800u; i += 4)
    {
      as.put_32(i, f, memory::SUPER_DATA);
      f += 4;
    }

  for (uint32_type i = 0x800u; i != 0x1000u; i += 4)
    as.put_32(i, 0, memory::SUPER_DATA);
}

namespace
{
  using vx68k::machine;
  using vm68k::byte_size;
  using vm68k::word_size;
  using vm68k::long_word_size;

#ifdef HAVE_NANA_H
# undef L_DEFAULT_GUARD
# define L_DEFAULT_GUARD nana_iocs_call_trace
#endif

  /* Handles a _B_DRVCHK call.  */
  void
  iocs_b_drvchk(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS _B_DRVCHK; %%d1:w=0x%04x %%d2:w=0x%04x\n",
      word_size::get(c.regs.d[1]), word_size::get(c.regs.d[2]));
#endif

    static bool once;
    if (!once++)
      fprintf(stderr, "iocs_b_drvchk: FIXME: not implemented\n");
    if ((c.regs.d[2] & 0xffff) == 8)
      long_word_size::put(c.regs.d[0], 1);
    else
      long_word_size::put(c.regs.d[0], 0x02);
  }

  /* Handles a _B_EJECT IOCS call.  */
  void
  iocs_b_eject(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS _B_EJECT; %%d1:w=0x%04x\n", word_size::get(c.regs.d[1]));
#endif

    static bool once;
    if (!once++)
      fprintf(stderr, "iocs_b_eject: FIXME: not implemented\n");
    long_word_size::put(c.regs.d[0], 0);
  }

  /* Handles a _B_INTVCS call.  */
  void
  iocs_b_intvcs(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS _B_INTVCS; %%d1:w=0x%04x %%a1=0x%08lx\n",
      word_size::get(c.regs.d[1]),
      (unsigned long) long_word_size::get(c.regs.a[1]));
#endif
    uint16_type vecno = word_size::get(c.regs.d[1]);
    uint32_type addr = long_word_size::get(c.regs.a[1]);

    if (vecno > 0x1ff)
      {
	fprintf(stderr, "IOCS _B_INTVCS: vector number out of range\n");
	return;
      }

    uint32_type vecaddr = vecno * 4u;
    uint32_type oaddr = c.mem->get_32(vecaddr, memory::SUPER_DATA);
    c.mem->put_32(vecaddr, addr, memory::SUPER_DATA);

    long_word_size::put(c.regs.d[0], oaddr);
  }

  /* Handles a _B_KEYINP call.  */
  void
  iocs_b_keyinp(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS _B_KEYINP\n");
#endif
    
    x68k_address_space *as = dynamic_cast<x68k_address_space *>(c.mem);
    I(as != NULL);

    uint16_type key = as->machine()->get_key();
    long_word_size::put(c.regs.d[0], key);
  }

  /* Handles a _B_KEYSNS call.  */
  void
  iocs_b_keysns(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS _B_KEYSNS\n");
#endif
    
    x68k_address_space *as = dynamic_cast<x68k_address_space *>(c.mem);
    I(as != NULL);

    uint16_type key = as->machine()->peek_key();
    if (key == 0)
      long_word_size::put(c.regs.d[0], 0);
    else
      long_word_size::put(c.regs.d[0], 0x10000 | key);
  }

  /* Handles a _B_LPEEK call.  */
  void
  iocs_b_lpeek(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS _B_LPEEK; %%a1=0x%08lx\n",
      (unsigned long) long_word_size::get(c.regs.a[1]));
#endif
    uint32_type address = c.regs.a[1];

    c.regs.d[0] = c.mem->get_32(address, memory::SUPER_DATA);
    c.regs.a[1] = address + 4;
  }

  /* Handles a _B_PRINT call.  */
  void
  iocs_b_print(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS _B_PRINT; %%a1=0x%08lx\n",
      (unsigned long) long_word_size::get(c.regs.a[1]));
#endif
    uint32_type address = c.regs.a[1];

    x68k_address_space *as = dynamic_cast<x68k_address_space *>(c.mem);
    as->machine()->b_print(c.mem, address);
  }

  /* Handles a _B_PUTC call.  */
  void
  iocs_b_putc(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS _B_PUTC; %%d1:w=0x%04x\n", word_size::get(c.regs.d[1]));
#endif
    uint16_type ch = word_size::get(c.regs.d[1]);

    x68k_address_space *as = dynamic_cast<x68k_address_space *>(c.mem);
    as->machine()->b_putc(ch);
  }

  /* Handles a _B_READ call.  */
  void
  iocs_b_read(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS _B_READ; %%d1:w=0x%04x %%d2=0x%08lx %%d3=0x%08lx %%a1=0x%08lx\n",
      word_size::get(c.regs.d[1]),
      (unsigned long) long_word_size::get(c.regs.d[2]),
      (unsigned long) long_word_size::get(c.regs.d[3]),
      (unsigned long) long_word_size::get(c.regs.a[1]));
#endif

    x68k_address_space *as = dynamic_cast<x68k_address_space *>(c.mem);
    c.regs.d[0] = as->machine()->read_disk(*c.mem,
					   c.regs.d[1] & 0xffffu, c.regs.d[2],
					   c.regs.a[1], c.regs.d[3]);
    // FIXME?
  }

  /* Handles a _B_READID call.  */
  void
  iocs_b_readid(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS _B_READID; %%d1:w=0x%04x %%d2=0x%08lx %%d3=0x%08lx %%a1=0x%08lx\n",
      word_size::get(c.regs.d[1]),
      (unsigned long) long_word_size::get(c.regs.d[2]),
      (unsigned long) long_word_size::get(c.regs.d[3]),
      (unsigned long) long_word_size::get(c.regs.a[1]));
#endif
    uint16_type pda_mode = word_size::get(c.regs.d[1]);

    fprintf(stderr, "iocs_b_readid: FIXME: not implemented\n");
    if ((pda_mode & 0xf000) == 0x9000 && (pda_mode & 0x0f00) <= 0x0100)
      long_word_size::put(c.regs.d[0], 0);
    else
      long_word_size::put(c.regs.d[0], -1);
  }

  /* Handles a _B_RECALI call.  */
  void
  iocs_b_recali(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS _B_RECALI; %%d1:w=0x%04x\n", word_size::get(c.regs.d[1]));
#endif
    static bool once;
    if (!once++)
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
#ifdef L
    L("IOCS _B_SFTSNS\n");
#endif
    
    x68k_address_space *as = dynamic_cast<x68k_address_space *>(c.mem);
    I(as != NULL);

    long_word_size::put(c.regs.d[0], as->machine()->key_modifiers());
  }

  /* Handles a _B_SUPER call.  */
  void
  iocs_b_super(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS _B_SUPER; %%a1=0x%08lx\n",
      (unsigned long) long_word_size::get(c.regs.a[1]));
#endif
    uint32_type ssp = long_word_size::get(c.regs.a[1]);

    if (ssp != 0)
      {
	if (c.supervisor_state())
	  {
	    long_word_size::put(c.regs.usp, long_word_size::get(c.regs.a[7]));
	    long_word_size::put(c.regs.a[7], ssp);
	    c.set_supervisor_state(false);
	  }

	long_word_size::put(c.regs.d[0], 0);
      }
    else
      {
	if (c.supervisor_state())
	  long_word_size::put(c.regs.d[0], 1);
	else
	  {
	    c.set_supervisor_state(true);
	    long_word_size::put(c.regs.d[0], c.regs.a[7]);
	    long_word_size::put(c.regs.a[7], long_word_size::get(c.regs.usp));
	  }
      }
  }

  /* Handles a _B_WRITE call.  */
  void
  iocs_b_write(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS _B_WRITE; %%d1:w=0x%04x %%d2=0x%08lx %%d3=0x%08lx %%a1=0x%08lx\n",
      word_size::get(c.regs.d[1]),
      (unsigned long) long_word_size::get(c.regs.d[2]),
      (unsigned long) long_word_size::get(c.regs.d[3]),
      (unsigned long) long_word_size::get(c.regs.a[1]));
#endif
    fprintf(stderr, "iocs_b_write: FIXME: not implemented\n");
  }

  /* Handles a _BITSNS IOCS call.  */
  void
  iocs_bitsns(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS _BITSNS; %%d1:w=0x%04x\n", word_size::get(c.regs.d[1]));
#endif

    static bool once;
    if (!once++)
      fprintf(stderr, "iocs_bitsns: FIXME: not implemented\n");
    long_word_size::put(c.regs.d[0], 0);
  }

  /* Handles a _BOOTINF call.  */
  void
  iocs_bootinf(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS _BOOTINF\n");
#endif
    c.regs.d[0] = 0x90;
  }

  /* Handles a _CONTRAST IOCS call.  */
  void
  iocs_contrast(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS _CONTRAST; %%d1:b=0x%02x\n", byte_size::uget(c.regs.d[1]));
#endif
    static bool once;
    if (!once++)
      fprintf(stderr, "iocs_contrast: FIXME: not implemented\n");

    long_word_size::put(c.regs.d[0], 14);
  }

  /* Handles a _CRTMOD call.  */
  void
  iocs_crtmod(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS _CRTMOD; %%d1:w=0x%04x\n", word_size::get(c.regs.d[1]));
#endif
    fprintf(stderr, "iocs_crtmod: FIXME: not implemented\n");
    c.regs.d[0] = 16;
  }

  /* Handles a _DATEASC call.  */
  void
  iocs_dateasc(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS _DATEASC; %%d1=0x%08lx %%a1=0x%08lx\n",
      (unsigned long) long_word_size::get(c.regs.d[1]),
      (unsigned long) long_word_size::get(c.regs.a[1]));
#endif
    long_word_size::uvalue_type value = long_word_size::get(c.regs.d[1]);
    long_word_size::uvalue_type address = long_word_size::get(c.regs.a[1]);

    unsigned int mday = value & 0xffu;
    unsigned int mon = (value >>= 8) & 0xffu;
    unsigned int year = (value >>= 8) & 0xfffu;
    unsigned int format = (value >>= 12) & 0xfu;

    char str[11];
    switch (format % 4)
      {
      case 0:
	sprintf(str, "%04u/%02u/%02u", year, mon, mday);
	break;

      case 1:
	sprintf(str, "%04u-%02u-%02u", year, mon, mday);
	break;

      case 2:
	sprintf(str, "%02u/%02u/%02u", year % 100, mon, mday);
	break;

      case 3:
	sprintf(str, "%02u-%02u-%02u", year % 100, mon, mday);
	break;
      }

    char *p = str + 0;
    while (*p != '\0')
      {
	c.mem->put_8(address, *p++, memory::SUPER_DATA);
	address = long_word_size::get(address + 1);
      }

    c.mem->put_8(address, *p, memory::SUPER_DATA);

    long_word_size::put(c.regs.a[1], address);
  }

  /* Handles a _DATEBIN call.  */
  void
  iocs_datebin(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS _DATEBIN; %%d1=0x%08lx\n",
      (unsigned long) long_word_size::get(c.regs.d[1]));
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
#ifdef L
    L("IOCS _DATEGET\n");
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

  /* Handles a _G_CLR_ON call.  */
  void
  iocs_g_clr_on(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS _G_CLR_ON\n");
#endif
    fprintf(stderr, "iocs_g_clr_on: FIXME: not implemented\n");
  }

  /* Handles a _INIT_PRN call.  */
  void
  iocs_init_prn(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS _INIT_PRN; %%d1:w=0x%04x\n", word_size::get(c.regs.d[1]));
#endif
    fprintf(stderr, "iocs_init_prn: FIXME: not implemented\n");
    c.regs.d[0] = 0;
  }

  /* Handles a _JOYGET IOCS call.  */
  void
  iocs_joyget(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS _JOYGET; %%d1:w=0x%04x\n", word_size::get(c.regs.d[1]));
#endif

    static bool once;
    if (!once++)
      fprintf(stderr, "iocs_joyget: FIXME: not implemented\n");
    byte_size::put(c.regs.d[0], 0xff);
  }

  /* Handles a _LEDMOD call.  */
  void
  iocs_ledmod(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS _LEDMOD; %%d1=0x%08lx %%d2:b=0x%02x\n",
      (unsigned long) long_word_size::get(c.regs.d[1]),
      byte_size::get(c.regs.d[2]));
#endif
    fprintf(stderr, "iocs_ledmod: FIXME: not implemented\n");
  }

  /* Handles a _ONTIME call.  */
  void
  iocs_ontime(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS _ONTIME\n");
#endif
    fprintf(stderr, "iocs_ontime: FIXME: not implemented\n");
    long_word_size::put(c.regs.d[0], 0);
    long_word_size::put(c.regs.d[1], 0);
  }

  /* Handles a _OPMINTST call.  */
  void
  iocs_opmintst(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS _OPMINTST; %%a1=0x%08lx\n",
      (unsigned long) long_word_size::get(c.regs.a[1]));
#endif
    uint32_type address = long_word_size::get(c.regs.a[1]);

    x68k_address_space *as = dynamic_cast<x68k_address_space *>(c.mem);

    if (address == 0)
      {
	as->machine()->set_opm_interrupt_enabled(false);
	long_word_size::put(c.regs.d[0], 0);
      }
    else
      {
	if (as->machine()->opm_interrupt_enabled())
	    long_word_size::put(c.regs.d[0], 1);
	else
	  {
	    as->put_32(0x43 * 4, address, memory::SUPER_DATA);
	    as->machine()->set_opm_interrupt_enabled(true);
	    long_word_size::put(c.regs.d[0], 0);
	  }
      }
  }

  /* Handles a _OPMSET call.  */
  void
  iocs_opmset(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS _OPMSET; %%d1:b=0x%02x %%d2:b=0x%02x\n",
      byte_size::get(c.regs.d[1]), byte_size::get(c.regs.d[2]));
#endif
    unsigned int regno = byte_size::get(c.regs.d[1]);
    unsigned int value = byte_size::get(c.regs.d[2]);

    x68k_address_space *as = dynamic_cast<x68k_address_space *>(c.mem);

    as->machine()->set_opm_reg(regno, value);
  }

  /* Handles a _OS_CUROF call.  */
  void
  iocs_os_curof(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS _OS_CUROF\n");
#endif
    fprintf(stderr, "iocs_os_curof: FIXME: not implemented\n");
  }

  /* Handles a _OS_CURON call.  */
  void
  iocs_os_curon(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS _OS_CURON\n");
#endif
    fprintf(stderr, "iocs_os_curon: FIXME: not implemented\n");
  }

  /* Handles a _ROMVER call.  */
  void
  iocs_romver(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS _ROMVER\n");
#endif
    uint32_type romver = ((0x13 << 8 | 0x00) << 8 | 0x01) << 8 | 0x01;

    long_word_size::put(c.regs.d[0], romver);
  }

  /* Handles a _SCROLL call.  */
  void
  iocs_scroll(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS _SCROLL; %%d1:w=0x%04x %%d2:w=0x%04x %%d3:w=0x%04x\n",
      word_size::get(c.regs.d[1]), word_size::get(c.regs.d[2]),
      word_size::get(c.regs.d[3]));
#endif
    fprintf(stderr, "iocs_scroll: FIXME: not implemented\n");
  }

  /* Handles a _SKEY_MOD call.  */
  void
  iocs_skey_mod(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS _SKEY_MOD; %%d1=0x%08lx %%d2=0x%08lx\n",
      (unsigned long) long_word_size::get(c.regs.d[1]),
      (unsigned long) long_word_size::get(c.regs.d[2]));
#endif
    fprintf(stderr, "iocs_skey_mod: FIXME: not implemented\n");
    long_word_size::put(c.regs.d[0], 0);
  }

  /* Handles a _SYS_STAT call.  */
  void
  iocs_sys_stat(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS _SYS_STAT; %%d1=0x%08lx %%d2=0x%08lx\n",
      (unsigned long) long_word_size::get(c.regs.d[1]),
      (unsigned long) long_word_size::get(c.regs.d[2]));
#endif
    uint32_type mode = long_word_size::get(c.regs.d[1]);
    switch (mode)
      {
      case 0:
	{
	  uint32_type mpu = 100 << 16 | 0x0000;
	  long_word_size::put(c.regs.d[0], mpu);
	}
	break;

      default:
	long_word_size::put(c.regs.d[0], 0);
	break;
      }
  }

  /* Handles a _TCOLOR call.  */
  void
  iocs_tcolor(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS _TCOLOR; %%d1:b=0x%02x\n", byte_size::get(c.regs.d[1]));
#endif
    fprintf(stderr, "iocs_tcolor: FIXME: not implemented\n");
  }

  /* Handles a _TGUSEMD call.  */
  void
  iocs_tgusemd(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS _TGUSEMD; %%d1:b=0x%02x %%d2:b=0x%02x\n",
      byte_size::get(c.regs.d[1]), byte_size::get(c.regs.d[2]));
#endif
    fprintf(stderr, "iocs_tgusemd: FIXME: not implemented\n");
    long_word_size::put(c.regs.d[0], 0);
  }

  /* Handles a _TIMEASC call.  */
  void
  iocs_timeasc(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS _TIMEASC; %%d1=0x%08lx %%a1=0x%08lx\n",
      (unsigned long) long_word_size::get(c.regs.d[1]),
      (unsigned long) long_word_size::get(c.regs.a[1]));
#endif
    long_word_size::uvalue_type value = long_word_size::get(c.regs.d[1]);
    long_word_size::uvalue_type address = long_word_size::get(c.regs.a[1]);

    unsigned int sec = value & 0xffu;
    unsigned int min = (value >>= 8) & 0xffu;
    unsigned int hour = (value >>= 8) & 0xffu;

    char str[9];
    sprintf(str, "%02u:%02u:%02u", hour, min, sec);

    char *p = str + 0;
    while (*p != '\0')
      {
	c.mem->put_8(address, *p++, memory::SUPER_DATA);
	address = long_word_size::get(address + 1);
      }

    c.mem->put_8(address, *p, memory::SUPER_DATA);

    long_word_size::put(c.regs.a[1], address);
  }

  /* Handles a _TIMEBIN call.  */
  void
  iocs_timebin(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS _TIMEBIN; %%d1=0x%08lx\n",
      (unsigned long) long_word_size::get(c.regs.d[1]));
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
#ifdef L
    L("IOCS _TIMEGET\n");
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
#ifdef L
    L("IOCS _TIMERDST; %%d1:w=0x%04x %%a1=0x%08lx\n",
      word_size::get(c.regs.d[1]),
      (unsigned long) long_word_size::get(c.regs.a[1]));
#endif
    fprintf(stderr, "iocs_timerdst: FIXME: not implemented\n");
    long_word_size::put(c.regs.d[0], 0);
  }

  /* Handles a _TPALET call.  */
  void
  iocs_tpalet(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS _TPALET; %%d1:b=0x%02x %%d2=0x%08lx\n",
      byte_size::get(c.regs.d[1]),
      (unsigned long) long_word_size::get(c.regs.d[2]));
#endif
    fprintf(stderr, "iocs_tpalet: FIXME: not implemented\n");
  }

  /* Handles a _VDISPST call.  */
  void
  iocs_vdispst(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS _VDISPST; %%d1:w=0x%04x %%a1=0x%08lx\n",
      word_size::get(c.regs.d[1]),
      (unsigned long) long_word_size::get(c.regs.a[1]));
#endif
    uint32_type address = long_word_size::get(c.regs.a[1]);

    x68k_address_space *as = dynamic_cast<x68k_address_space *>(c.mem);

    if (address == 0)
      {
	as->machine()->set_vdisp_counter_data(0);
	long_word_size::put(c.regs.d[0], 0);
      }
    else
      {
	if (as->machine()->vdisp_interrupt_enabled())
	    long_word_size::put(c.regs.d[0], 1);
	else
	  {
	    unsigned int count = byte_size::get(c.regs.d[1]);
	    if (count == 0)
	      count = 0x100;

	    as->put_32(0x4d * 4, address, memory::SUPER_DATA);
	    as->machine()->set_vdisp_counter_data(count);
	    long_word_size::put(c.regs.d[0], 0);
	  }
      }
  }

  /* Handles a 0x37 call.  */
  void
  iocs_x37(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS 0x37; %%d1=0x%08lx\n",
      (unsigned long) long_word_size::get(c.regs.d[1]));
#endif
    fprintf(stderr, "iocs_x37: FIXME: not implemented\n");
  }

  /* Handles a 0x38 call.  */
  void
  iocs_x38(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS 0x38; %%d1=0x%08lx %%d2=0x%08lx\n",
      (unsigned long) long_word_size::get(c.regs.d[1]),
      (unsigned long) long_word_size::get(c.regs.d[2]));
#endif
    fprintf(stderr, "iocs_x38: FIXME: not implemented\n");
  }

  /* Handles a 0x39 call.  */
  void
  iocs_x39(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS 0x39; %%d1=0x%08lx %%d2:w=0x%04x\n",
      (unsigned long) long_word_size::get(c.regs.d[1]),
      word_size::get(c.regs.d[2]));
#endif
    fprintf(stderr, "iocs_x39: FIXME: not implemented\n");
    long_word_size::put(c.regs.d[1], 0);
    word_size::put(c.regs.d[2], 0);
  }

  /* Handles a 0x3a call.  */
  void
  iocs_x3a(context &c, unsigned long data)
  {
#ifdef L
    L("IOCS 0x3a; %%a1=0x%08lx\n",
      (unsigned long) long_word_size::get(c.regs.a[1]));
#endif
    fprintf(stderr, "iocs_x3a: FIXME: not implemented\n");
  }

#ifdef HAVE_NANA_H
# undef L_DEFAULT_GUARD
# define L_DEFAULT_GUARD true
#endif

  /* Initializes the IOCS functions.  */
  void
  initialize_iocs_calls(system_rom *rom, unsigned long data)
  {
    rom->set_iocs_call(0x00, make_pair(&iocs_b_keyinp, data));
    rom->set_iocs_call(0x01, make_pair(&iocs_b_keysns, data));
    rom->set_iocs_call(0x02, make_pair(&iocs_b_sftsns, data));
    rom->set_iocs_call(0x04, make_pair(&iocs_bitsns, data));
    rom->set_iocs_call(0x0d, make_pair(&iocs_ledmod, data));
    rom->set_iocs_call(0x0e, make_pair(&iocs_tgusemd, data));
    rom->set_iocs_call(0x10, make_pair(&iocs_crtmod, data));
    rom->set_iocs_call(0x11, make_pair(&iocs_contrast, data));
    rom->set_iocs_call(0x13, make_pair(&iocs_tpalet, data));
    rom->set_iocs_call(0x15, make_pair(&iocs_tcolor, data));
    rom->set_iocs_call(0x1d, make_pair(&iocs_scroll, data));
    rom->set_iocs_call(0x20, make_pair(&iocs_b_putc, data));
    rom->set_iocs_call(0x21, make_pair(&iocs_b_print, data));
    rom->set_iocs_call(0x37, make_pair(&iocs_x37, data));
    rom->set_iocs_call(0x38, make_pair(&iocs_x38, data));
    rom->set_iocs_call(0x39, make_pair(&iocs_x39, data));
    rom->set_iocs_call(0x3a, make_pair(&iocs_x3a, data));
    rom->set_iocs_call(0x3b, make_pair(&iocs_joyget, data));
    rom->set_iocs_call(0x3c, make_pair(&iocs_init_prn, data));
    rom->set_iocs_call(0x45, make_pair(&iocs_b_write, data));
    rom->set_iocs_call(0x46, make_pair(&iocs_b_read, data));
    rom->set_iocs_call(0x47, make_pair(&iocs_b_recali, data));
    rom->set_iocs_call(0x4a, make_pair(&iocs_b_readid, data));
    rom->set_iocs_call(0x4e, make_pair(&iocs_b_drvchk, data));
    rom->set_iocs_call(0x4f, make_pair(&iocs_b_eject, data));
    rom->set_iocs_call(0x54, make_pair(&iocs_dateget, data));
    rom->set_iocs_call(0x55, make_pair(&iocs_datebin, data));
    rom->set_iocs_call(0x56, make_pair(&iocs_timeget, data));
    rom->set_iocs_call(0x57, make_pair(&iocs_timebin, data));
    rom->set_iocs_call(0x5a, make_pair(&iocs_dateasc, data));
    rom->set_iocs_call(0x5b, make_pair(&iocs_timeasc, data));
    rom->set_iocs_call(0x68, make_pair(&iocs_opmset, data));
    rom->set_iocs_call(0x6a, make_pair(&iocs_opmintst, data));
    rom->set_iocs_call(0x6b, make_pair(&iocs_timerdst, data));
    rom->set_iocs_call(0x6c, make_pair(&iocs_vdispst, data));
    rom->set_iocs_call(0x7d, make_pair(&iocs_skey_mod, data));
    rom->set_iocs_call(0x7f, make_pair(&iocs_ontime, data));
    rom->set_iocs_call(0x80, make_pair(&iocs_b_intvcs, data));
    rom->set_iocs_call(0x81, make_pair(&iocs_b_super, data));
    rom->set_iocs_call(0x84, make_pair(&iocs_b_lpeek, data));
    rom->set_iocs_call(0x8e, make_pair(&iocs_bootinf, data));
    rom->set_iocs_call(0x8f, make_pair(&iocs_romver, data));
    rom->set_iocs_call(0x90, make_pair(&iocs_g_clr_on, data));
    rom->set_iocs_call(0xac, make_pair(&iocs_sys_stat, data));
    rom->set_iocs_call(0xae, make_pair(&iocs_os_curon, data));
    rom->set_iocs_call(0xaf, make_pair(&iocs_os_curof, data));
  }
} // namespace (unnamed)

system_rom::~system_rom()
{
  detach(attached_eu);
}

system_rom::system_rom()
  : iocs_calls(0x100, make_pair(&invalid_iocs_call, 0)),
    attached_eu(NULL)
{
  initialize_iocs_calls(this, 0);
}

void
system_rom::invalid_iocs_call(context &c, unsigned long data)
{
  fprintf(stderr, "system_rom: IOCS 0x%02x\n",
	  byte_size::get(c.regs.d[0]) & 0xff);
  throw runtime_error("invalid iocs function");	// FIXME
}
