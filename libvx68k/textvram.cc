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

#include <vx68k/memory.h>
#include <vm68k/mutex.h>

#include <algorithm>

#ifdef HAVE_NANA_H
# include <nana.h>
# include <cstdio>
#else
# include <cassert>
# define I assert
#endif

using vx68k::text_video_raster_iterator;
using vx68k::text_video_memory;
using vm68k::bus_error_exception;
using vm68k::SUPER_DATA;
using vm68k::auto_lock;
using namespace vm68k::types;
using namespace std;

const size_t ROW_SIZE = 1024 / 8;
const size_t PLANE_SIZE = 1024 * ROW_SIZE;
const size_t PLANE_MAX = 4;

inline void
advance_row(unsigned char *&ptr)
{
  ptr += ROW_SIZE;
}

uint_type
text_video_raster_iterator::operator*() const
{
  unsigned char mask = 0x80 >> (pos % 8u);
  uint_type bit = 1u;
  uint_type value = 0;
  for (const unsigned char *i = packs + 0; i != packs + 4; ++i)
    {
      if (*i & mask)
	value |= bit;

      bit <<= 1;
    }

  return value;
}

text_video_raster_iterator &
text_video_raster_iterator::operator++()
{
  unsigned int tmp = pos;
  ++pos;
  if (pos / 8u != tmp / 8u)
    {
      unsigned char *p = buf + (pos / 8u) % ROW_SIZE;
      packs[0] = p[0 * PLANE_SIZE];
      packs[1] = p[1 * PLANE_SIZE];
      packs[2] = p[2 * PLANE_SIZE];
      packs[3] = p[3 * PLANE_SIZE];
    }

  return *this;
}

text_video_raster_iterator::text_video_raster_iterator(unsigned char *base,
						       unsigned int x)
  : buf(base), pos(x)
{
  unsigned char *p = buf + (pos / 8u) % ROW_SIZE;
  packs[0] = p[0 * PLANE_SIZE];
  packs[1] = p[1 * PLANE_SIZE];
  packs[2] = p[2 * PLANE_SIZE];
  packs[3] = p[3 * PLANE_SIZE];
}

void
text_video_memory::mark_update_area(unsigned int left_x,
				    unsigned int top_y,
				    unsigned int right_x,
				    unsigned int bottom_y)
{
  auto_lock<pthread_mutex_t> lock(&mutex);

  fill(raster_update_marks.begin() + top_y,
       raster_update_marks.begin() + bottom_y, true);
}

vector<bool>
text_video_memory::poll_update()
{
  auto_lock<pthread_mutex_t> lock(&mutex);

  vector<bool> tmp(1024, false);
  tmp.swap(raster_update_marks);
  return tmp;
}

void
text_video_memory::scroll()
{
  fill(copy(buf + 1 * 16 * ROW_SIZE, buf + 31 * 16 * ROW_SIZE, buf),
       buf + 31 * 16 * ROW_SIZE, 0);

  fill(copy(buf + PLANE_SIZE + 1 * 16 * ROW_SIZE, buf + PLANE_SIZE + 31 * 16 * ROW_SIZE, buf + PLANE_SIZE),
       buf + PLANE_SIZE + 31 * 16 * ROW_SIZE, 0);

  mark_update_area(0, 0, ROW_SIZE * 8, 31 * 16);
}

void
text_video_memory::draw_char(int x, int y, unsigned int c)
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

      for (unsigned char *plane = buf;
	   plane != buf + 2 * PLANE_SIZE;
	   plane += PLANE_SIZE)
	{
	  unsigned char *p = plane + y * 16 * ROW_SIZE + x;
	  for (unsigned char *i = img + 0; i != img + 16 * 2; i += 2)
	    {
	      p[0] = i[0] & 0xffu;
	      p[1] = i[1] & 0xffu;
	      advance_row(p);
	    }
	}

      mark_update_area(x * 8, y * 16, x * 8 + 16, y * 16 + 16);
    }
  else
    {
      unsigned char img[16];
      connected_console->get_b16_image(c, img, 1);

      for (unsigned char *plane = buf;
	   plane != buf + 2 * PLANE_SIZE;
	   plane += PLANE_SIZE)
	{
	  unsigned char *p = plane + y * 16 * ROW_SIZE + x;
	  for (unsigned char *i = img + 0; i != img + 16; ++i)
	    {
	      *p = i[0] & 0xffu;
	      advance_row(p);
	    }
	}

      mark_update_area(x * 8, y * 16, x * 8 + 8, y * 16 + 16);
    }
}

void
text_video_memory::get_image(int x, int y, int width, int height,
			     unsigned char *rgb_buf, size_t row_size)
{
  // FIXME
  unsigned char *p = buf + y * ROW_SIZE;
  for (int i = 0; i != height; ++i)
    {
      for (int j = 0; j != width; ++j)
	{
	  unsigned char *q = p + (j / 8u);
	  unsigned char *s = rgb_buf + i * row_size + j * 3;
	  if (*q & 0x80 >> (j % 8u))
	    {
	      s[0] = 0xff;
	      s[1] = 0xff;
	      s[2] = 0xff;
	    }
	}
      advance_row(p);
    }
}

text_video_memory::raster_iterator
text_video_memory::raster(unsigned int x, unsigned int y)
{
  return raster_iterator(buf + y * ROW_SIZE, x);
}

uint_type
text_video_memory::get_16(int fc, uint32_type address) const
{
  if (fc != SUPER_DATA)
    throw bus_error_exception(true, fc, address);

  address &= PLANE_MAX * PLANE_SIZE - 1u;
  uint_type value = vm68k::getw(buf + address);
  return value;
}

uint_type
text_video_memory::get_8(int fc, uint32_type address) const
{
  if (fc != SUPER_DATA)
    throw bus_error_exception(true, fc, address);

  address &= PLANE_MAX * PLANE_SIZE - 1u;
  uint_type value = *(buf + address);
  return value;
}

void
text_video_memory::put_16(int fc, uint32_type address, uint_type value)
{
  if (fc != SUPER_DATA)
    throw bus_error_exception(false, fc, address);

  address &= PLANE_MAX * PLANE_SIZE - 1u;
  vm68k::putw(buf + address, value & 0xffffu);

  unsigned int x = address % ROW_SIZE * 8;
  unsigned int y = address / ROW_SIZE % 1024u;
  mark_update_area(x, y, x + 16, y + 1);
}

void
text_video_memory::put_8(int fc, uint32_type address, uint_type value)
{
  if (fc != SUPER_DATA)
    throw bus_error_exception(false, fc, address);

  address &= PLANE_MAX * PLANE_SIZE - 1u;
  *(buf + address) = value & 0xffu;

  unsigned int x = address % ROW_SIZE * 8;
  unsigned int y = address / ROW_SIZE % 1024u;
  mark_update_area(x, y, x + 8, y + 1);
}

void
text_video_memory::connect(console *con)
{
  connected_console = con;
  mark_update_area(0, 0, ROW_SIZE * 8, 512);
}

text_video_memory::~text_video_memory()
{
  pthread_mutex_destroy(&mutex);

  delete [] buf;
}

text_video_memory::text_video_memory()
  : buf(NULL),
    connected_console(NULL),
    raster_update_marks(1024, false)
{
  buf = new unsigned char [PLANE_MAX * PLANE_SIZE];
  fill(buf + 0, buf + PLANE_MAX * PLANE_SIZE, 0);

  pthread_mutex_init(&mutex, NULL);
}
