/* Virtual X68000 - Sharp X68000 emulator
   Copyright (C) 1998, 2000 Hypercore Software Design, Ltd.

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

using namespace vx68k;
using namespace std;

#ifdef HAVE_NANA_H
# include <nana.h>
# include <cstdio>
#else
# include <cassert>
# define I assert
#endif

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
  iocs_trap(uint_type, context &c, instruction_data *data)
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
  attached_eu->set_instruction(0x4e4f, 0, &iocs_trap,
			       reinterpret_cast<instruction_data *>(this));
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
  /* Handles a _B_LPEEK call.  */
  void
  iocs_b_lpeek(context &c, unsigned long data)
  {
    uint32_type address = c.regs.a[1];
#ifdef HAVE_NANA_H
    L("| _B_LPEEK %%a1=%#010x\n", address);
#endif

    c.regs.d[0] = c.mem->getl(SUPER_DATA, address);
    c.regs.a[1] = address + 4;
  }

  /* Initializes the IOCS functions.  */
  void
  initialize_iocs_functions(system_rom &rom)
  {
    typedef system_rom::iocs_function_type iocs_function_type;

    rom.set_iocs_function(0x84, iocs_function_type(&iocs_b_lpeek, NULL));
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
  initialize_iocs_functions(*this);
}

void
system_rom::invalid_iocs_function(context &c, unsigned long data)
{
#ifdef HAVE_NANA_H
  L("| IOCS (%#04x)\n", c.regs.d[0] & 0xffu);
#endif
  throw runtime_error("invalid iocs function");	// FIXME
}
