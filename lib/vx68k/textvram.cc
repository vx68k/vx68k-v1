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
  uint16 *i = buf;
  for (uint16 *j = buf + 16 * (ROW_SIZE >> 1);
       j != buf + 31 * 16 * (ROW_SIZE >> 1);
       ++j)
    {
      *i++ = *j;
    }
  while (i != buf + 31 * 16 * (ROW_SIZE >> 1))
    {
      *i++ = 0;
    }
}

void
text_vram::draw_char(int x, int y, unsigned int c)
{
  if (c > 0xff)
    {
      unsigned int byte1 = c >> 8;
      if (byte1 >= 0x81 && byte1 <= 0x9f
	  || byte1 >= 0xe0 && byte1 <= 0xef)
	{
	  if (byte1 >= 0xe0)
	    byte1 -= 0xe0 - 0xa0;
	  byte1 -= 0x81;

	  unsigned int byte2 = c & 0xff;
	  if (byte2 >= 0x80)
	    --byte2;
	  byte2 -= 0x40;

	  byte1 *= 2;
	  if (byte2 >= 94)
	    {
	      byte2 -= 94;
	      ++byte1;
	    }

	  c = byte1 + 0x21 << 8 | byte2 + 0x21;
	}

      unsigned char img[16 * 2];
      connected_console->get_k16_image(c, img, 2);

      if (x % 2 != 0)
	{
	  for (uint16 *plane = buf;
	       plane != buf + 2 * (TEXT_VRAM_PLANE_SIZE >> 1);
	       plane += TEXT_VRAM_PLANE_SIZE >> 1)
	    {
	      uint16 *p = plane + (y * 16 * ROW_SIZE + x >> 1);
	      for (unsigned char *i = img + 0; i != img + 16 * 2; i += 2)
		{
		  p[0] = p[0] & ~0xff | i[0] & 0xff;
		  p[1] = i[1] << 8 | p[1] & 0xff;
		  advance_row(p);
		}
	    }
	}
      else
	{
	  for (uint16 *plane = buf;
	       plane != buf + 2 * (TEXT_VRAM_PLANE_SIZE >> 1);
	       plane += TEXT_VRAM_PLANE_SIZE >> 1)
	    {
	      uint16 *p = plane + (y * 16 * ROW_SIZE + x >> 1);
	      for (unsigned char *i = img + 0; i != img + 16 * 2; i += 2)
		{
		  p[0] = i[0] << 8 | i[1] & 0xff;
		  advance_row(p);
		}
	    }
	}
    }
  else
    {
      unsigned char img[16];
      connected_console->get_b16_image(c, img, 1);

      if (x % 2 != 0)
	{
	  for (uint16 *plane = buf;
	       plane != buf + 2 * (TEXT_VRAM_PLANE_SIZE >> 1);
	       plane += TEXT_VRAM_PLANE_SIZE >> 1)
	    {
	      uint16 *p = plane + (y * 16 * ROW_SIZE + x >> 1);
	      for (unsigned char *i = img + 0; i != img + 16; ++i)
		{
		  *p = *p & ~0xff | i[0] & 0xff;
		  advance_row(p);
		}
	    }
	}
      else
	{
	  for (uint16 *plane = buf;
	       plane != buf + 2 * (TEXT_VRAM_PLANE_SIZE >> 1);
	       plane += TEXT_VRAM_PLANE_SIZE >> 1)
	    {
	      uint16 *p = plane + (y * 16 * ROW_SIZE + x >> 1);
	      for (unsigned char *i = img + 0; i != img + 16; ++i)
		{
		  *p = i[0] << 8 | *p & 0xff;
		  advance_row(p);
		}
	    }
	}
    }
}

void
text_vram::get_image(int x, int y, int width, int height,
		     unsigned char *rgb_buf, size_t row_size)
{
  // FIXME
  uint16 *p = buf + (y * ROW_SIZE >> 1);
  for (int i = 0; i != height; ++i)
    {
      for (int j = 0; j != width; ++j)
	{
	  uint16 *q = p + (j >> 4);
	  unsigned char *s = rgb_buf + i * row_size + j * 3;
	  if (*q & 0x8000 >> (j & 0xf))
	    {
	      s[0] = 0xff;
	      s[1] = 0xff;
	      s[2] = 0xff;
	    }
	}
      advance_row(p);
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
    connected_console(NULL)
{
  buf = new uint16 [TEXT_VRAM_SIZE >> 1];
}

