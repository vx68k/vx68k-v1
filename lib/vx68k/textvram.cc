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

using namespace vx68k;
using namespace std;

size_t
text_vram::read(int fc, uint32_type address, void *, size_t) const
{
  if (fc != SUPER_DATA)
    generate_bus_error(fc, address);

  abort();			// FIXME
}

uint_type
text_vram::getb(int fc, uint32_type address) const
{
  if (fc != SUPER_DATA)
    generate_bus_error(fc, address);

  abort();			// FIXME
}

uint_type
text_vram::getw(int fc, uint32_type address) const
{
  if (fc != SUPER_DATA)
    generate_bus_error(fc, address);

  return buf[(address & TEXT_VRAM_SIZE - 1) >> 1];
}

size_t
text_vram::write(int fc, uint32_type address, const void *, size_t)
{
  if (fc != SUPER_DATA)
    generate_bus_error(fc, address);

  abort();			// FIXME
}

void
text_vram::putb(int fc, uint32_type address, uint_type value)
{
  if (fc != SUPER_DATA)
    generate_bus_error(fc, address);

  abort();			// FIXME
}

void
text_vram::putw(int fc, uint32_type address, uint_type value)
{
  if (fc != SUPER_DATA)
    generate_bus_error(fc, address);

  buf[(address & TEXT_VRAM_SIZE - 1) >> 1] = value & 0xffffu;
}

void
text_vram::connect(console *con)
{
  connected_console = con;
}

text_vram::~text_vram()
{
  delete [] buf;
}

text_vram::text_vram()
  : buf(NULL),
    connected_console(NULL)
{
  buf = new uint16 [TEXT_VRAM_SIZE >> 1];
}

