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

#ifdef HAVE_NANA_H
# include <nana.h>
# include <cstdio>
#else
# include <cassert>
# define I assert
#endif

using namespace vx68k;
using namespace std;

const size_t ROW_SIZE = 128;

inline void
advance_row(unsigned short *&ptr)
{
  ptr += ROW_SIZE >> 1;
}

void
text_vram::scroll()
{
  unsigned short *i = buf;
  for (unsigned short *j = buf + 16 * (ROW_SIZE >> 1);
       j != buf + 31 * 16 * (ROW_SIZE >> 1);
       ++j)
    {
      *i++ = *j;
    }
  while (i != buf + 31 * 16 * (ROW_SIZE >> 1))
    {
      *i++ = 0;
    }

  connected_console->update_area(0, 0, ROW_SIZE * 8, 31 * 16);
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
	  for (unsigned short *plane = buf;
	       plane != buf + 2 * (TEXT_VRAM_PLANE_SIZE >> 1);
	       plane += TEXT_VRAM_PLANE_SIZE >> 1)
	    {
	      unsigned short *p = plane + (y * 16 * ROW_SIZE + x >> 1);
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
	  for (unsigned short *plane = buf;
	       plane != buf + 2 * (TEXT_VRAM_PLANE_SIZE >> 1);
	       plane += TEXT_VRAM_PLANE_SIZE >> 1)
	    {
	      unsigned short *p = plane + (y * 16 * ROW_SIZE + x >> 1);
	      for (unsigned char *i = img + 0; i != img + 16 * 2; i += 2)
		{
		  p[0] = i[0] << 8 | i[1] & 0xff;
		  advance_row(p);
		}
	    }
	}

      connected_console->update_area(x * 8, y * 16, 16, 16);
    }
  else
    {
      unsigned char img[16];
      connected_console->get_b16_image(c, img, 1);

      if (x % 2 != 0)
	{
	  for (unsigned short *plane = buf;
	       plane != buf + 2 * (TEXT_VRAM_PLANE_SIZE >> 1);
	       plane += TEXT_VRAM_PLANE_SIZE >> 1)
	    {
	      unsigned short *p = plane + (y * 16 * ROW_SIZE + x >> 1);
	      for (unsigned char *i = img + 0; i != img + 16; ++i)
		{
		  *p = *p & ~0xff | i[0] & 0xff;
		  advance_row(p);
		}
	    }
	}
      else
	{
	  for (unsigned short *plane = buf;
	       plane != buf + 2 * (TEXT_VRAM_PLANE_SIZE >> 1);
	       plane += TEXT_VRAM_PLANE_SIZE >> 1)
	    {
	      unsigned short *p = plane + (y * 16 * ROW_SIZE + x >> 1);
	      for (unsigned char *i = img + 0; i != img + 16; ++i)
		{
		  *p = i[0] << 8 | *p & 0xff;
		  advance_row(p);
		}
	    }
	}

      connected_console->update_area(x * 8, y * 16, 8, 16);
    }
}

void
text_vram::get_image(int x, int y, int width, int height,
		     unsigned char *rgb_buf, size_t row_size)
{
  // FIXME
  unsigned short *p = buf + (y * ROW_SIZE >> 1);
  for (int i = 0; i != height; ++i)
    {
      for (int j = 0; j != width; ++j)
	{
	  unsigned short *q = p + (j >> 4);
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

uint_type
text_vram::get_8(int fc, uint32_type address) const
{
  if (fc != SUPER_DATA)
    generate_bus_error(true, fc, address);

  abort();			// FIXME
}

uint_type
text_vram::get_16(int fc, uint32_type address) const
{
  if (fc != SUPER_DATA)
    generate_bus_error(true, fc, address);

  return buf[(address & TEXT_VRAM_SIZE - 1) >> 1];
}

void
text_vram::put_8(int fc, uint32_type address, uint_type value)
{
  if (fc != SUPER_DATA)
    generate_bus_error(false, fc, address);

  abort();			// FIXME
}

void
text_vram::put_16(int fc, uint32_type address, uint_type value)
{
#ifdef HAVE_NANA_H
  L("class text_vram: put_16 fc=%d address=%#010lx\n",
    fc, (unsigned long) address);
#endif
  if (fc != SUPER_DATA)
    generate_bus_error(false, fc, address);

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
  buf = new unsigned short [TEXT_VRAM_SIZE >> 1];
}

