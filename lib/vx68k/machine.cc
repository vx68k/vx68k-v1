/* vx68k - Virtual X68000
   Copyright (C) 1998, 1999 Hypercore Software Design, Ltd.

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

#include <algorithm>
#include <stdexcept>

using namespace vx68k;
using namespace vm68k;
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
  iocs_b_lpeek(context &c, machine &m, iocs_function_data *data)
  {
    uint32_type address = c.regs.a[1];
#ifdef L
    L("| address = %#10x\n", address);
#endif

    c.regs.d[0] = m.address_space()->getl(SUPER_DATA, address);
    c.regs.a[1] = address + 4;
  }

  void
  set_iocs_functions(machine &m)
  {
    m.set_iocs_function(0x84, &iocs_b_lpeek, NULL);
  }
} // (unnamed namespace)

void
machine::invalid_iocs_function(context &c, machine &m,
			       iocs_function_data *data)
{
  throw runtime_error("invalid iocs function");	// FIXME
}

void
machine::iocs(uint_type op, context &c, instruction_data *data)
{
  unsigned int funcno = c.regs.d[0] & 0xffu;
#ifdef L
  L(" trap #15\t| IOCS %#4x\n", funcno);
#endif

  machine *m = static_cast<machine *>(data);
  I(m != NULL);
  (*m->iocs_functions[funcno].first)(c, *m, m->iocs_functions[funcno].second);

  c.regs.pc += 2;
}

void
machine::set_iocs_function(unsigned int i, iocs_function_handler handler,
			   iocs_function_data *data)
{
  if (i > 0xffu)
    throw range_error("iocs function must be between 0 and 0xff");

  iocs_functions[i] = make_pair(handler, data);
}

machine::machine(size_t memory_size)
  : _memory_size(memory_size),
    main_mem(memory_size)
{
  fill(iocs_functions + 0, iocs_functions + 0x100,
       make_pair(&invalid_iocs_function, (iocs_function_data *) 0));
  
  as.set_pages(0 >> PAGE_SHIFT, _memory_size >> PAGE_SHIFT, &main_mem);
#if 0
  as.set_pages(0xc00000 >> PAGE_SHIFT, 0xe00000 >> PAGE_SHIFT, &graphic_vram);
  as.set_pages(0xe00000 >> PAGE_SHIFT, 0xe80000 >> PAGE_SHIFT, &text_vram);
#endif

  eu.set_instruction(0x4e4f, 0, &iocs, this);

  set_iocs_functions(*this);
}
