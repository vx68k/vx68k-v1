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

namespace
{
  void
  iocs_dispatch(uint_type, context &, instruction_data *)
  {
  }
} // namespace (unnamed)

uint_type
system_rom::getw(int, uint32_type) const
{
#ifdef HAVE_NANA_H
  L("system_rom: `getw' not implemented\n");
#endif
  return 0;
}

uint_type
system_rom::getb(int, uint32_type) const
{
#ifdef HAVE_NANA_H
  L("system_rom: `getb' not implemented\n");
#endif
  return 0;
}

size_t
system_rom::read(int, uint32_type, void *, size_t) const
{
#ifdef HAVE_NANA_H
  L("system_rom: `read' not implemented\n");
#endif
  return 0;
}

void
system_rom::putw(int, uint32_type, uint_type)
{
#ifdef HAVE_NANA_H
  L("system_rom: `putw' not implemented\n");
#endif
}

void
system_rom::putb(int, uint32_type, uint_type)
{
#ifdef HAVE_NANA_H
  L("system_rom: `putb' not implemented\n");
#endif
}

size_t
system_rom::write(int, uint32_type, const void *, size_t)
{
#ifdef HAVE_NANA_H
  L("system_rom: `write' not implemented\n");
#endif
  return 0;
}

void
system_rom::attach(exec_unit *eu)
{
  if (attached_eu != NULL)
    throw logic_error("system_rom");

  attached_eu = eu;
  attached_eu->set_instruction(0x4e4f, 0, &iocs_dispatch, NULL);
}

void
system_rom::detach(exec_unit *eu)
{
  if (attached_eu != NULL)
    {
#ifdef HAVE_NANA_H
      L("system_rom: `detach' not implemented\n");
#endif
      attached_eu = NULL;
    }
}

void
system_rom::initialize(address_space &)
{
#ifdef HAVE_NANA_H
  L("system_rom: `initialize' not implemented\n");
#endif
}

void
system_rom::invalid_iocs_function(context &c, machine &m, unsigned long data)
{
  throw runtime_error("invalid iocs function");	// FIXME
}

system_rom::~system_rom()
{
  detach(attached_eu);
}

system_rom::system_rom()
  : iocs_functions(0x100, iocs_function_type(&invalid_iocs_function, 0)),
    attached_eu(NULL)
{
}
