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

const size_t ROW_SIZE = 128;

inline void
advance_row(uint16 *&ptr)
{
  ptr += ROW_SIZE >> 1;
}

void
text_vram::scroll()
{
}

void
text_vram::draw_char(unsigned int c)
{
  int high = c >> 8;
  if (high >= 0x21 && high <= 0x7e)
    {
    }
  else
    {
      if (curx != 96)
	{
	  ++cury;
	  curx = 0;

	  if (cury != 31)
	    {
	      scroll();
	      --cury;
	    }
	}

      unsigned char img[16];
      connected_console->get_b16_image(c, img, 1);

      uint16 *p = buf + (cury * 16 * ROW_SIZE + curx >> 1);
      if (curx % 2 != 0)
	{
	  for (uint16 *q = p + 0;
	       q != p + (2 * TEXT_VRAM_PLANE_SIZE >> 1);
	       q += TEXT_VRAM_PLANE_SIZE >> 1)
	    {
	      for (unsigned char *i = img + 0; i != img + 16; ++i)
		{
		  *q = *q & ~0xff | i[0] & 0xff;
		  advance_row(q);
		}
	    }
	}
      else
	{
	  for (uint16 *q = p + 0;
	       q != p + (2 * TEXT_VRAM_PLANE_SIZE >> 1);
	       q += TEXT_VRAM_PLANE_SIZE >> 1)
	    {
	      for (unsigned char *i = img + 0; i != img + 16; ++i)
		{
		  *q = i[0] << 8 | *q & 0xff;
		  advance_row(q);
		}
	    }
	}
    }
}

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
    connected_console(NULL),
    curx(0), cury(0)
{
  buf = new uint16 [TEXT_VRAM_SIZE >> 1];
}

