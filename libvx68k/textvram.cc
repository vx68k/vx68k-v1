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
#include <cstdio>

#ifdef HAVE_NANA_H
# include <nana.h>
#else
# include <cassert>
# define I assert
#endif

using vx68k::text_video_raster_iterator;
using vx68k::text_video_memory;
using vm68k::bus_error_exception;
using vm68k::mutex_lock;
using namespace vm68k::types;
using namespace std;

const size_t ROW_SIZE = 1024 / 8;
const size_t PLANE_SIZE = 1024 * ROW_SIZE;
const size_t PLANE_MAX = 4;

#ifdef HAVE_NANA_H
extern bool nana_iocs_call_trace;
#endif

inline void
advance_row(unsigned char *&ptr)
{
  ptr += ROW_SIZE;
}

int
text_video_raster_iterator::operator*() const
{
  unsigned char mask = 0x80 >> (pos % 8u);
  int bit = 1u;
  int value = 0;
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
  mutex_lock lock(&mutex);

  fill(raster_update_marks.begin() + top_y,
       raster_update_marks.begin() + bottom_y, true);
}

bool
text_video_memory::row_changed(unsigned int index)
{
  bool value = raster_update_marks[index];
  if (value)
    raster_update_marks[index] = false;
  return value;
}

vector<bool>
text_video_memory::poll_update()
{
  mutex_lock lock(&mutex);

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
  int ch1 = c >> 8 & 0xff;
  int ch2 = c & 0xff;
  if (ch1 >= 0x81 && ch1 <= 0x9f || ch1 >= 0xe0 && ch1 <= 0xef)
    {
      if (ch1 >= 0xe0)
	ch1 -= 0x81 + (0xe0 - 0xa0);
      else
	ch1 -= 0x81;

      if (ch2 >= 0x80)
	ch2 -= 0x40 + 1;
      else
	ch2 -= 0x40;

      ch1 *= 2;
      if (ch2 >= 94)
	{
	  ch2 -= 94;
	  ++ch1;
	}

      ch1 += 0x21;
      ch2 += 0x21;
    }

  if (ch1 >= 0x21 && ch1 <= 0x7e)
    {
      unsigned char img[16 * 2];
      connected_console->get_k16_image(uint_type(ch1) << 8 | ch2, img, 2);

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
      connected_console->get_b16_image(ch2, img, 1);

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

namespace
{
  using vm68k::byte_size;
  using vm68k::word_size;
  using vm68k::long_word_size;
  using vm68k::context;

  /* Handles a _B_CLR_ST IOCS call.  */
  void
  iocs_b_clr_st(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    LG(nana_iocs_call_trace, "IOCS _B_CLR_ST; %%d1:b=0x%02x\n",
       byte_size::get(c.regs.d[1]));
#endif
    static bool once;
    if (!once++)
      fprintf(stderr, "iocs_b_clr_st: FIXME: not implemented\n");
  }

  /* Handles a _B_COLOR IOCS call.  */
  void
  iocs_b_color(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    LG(nana_iocs_call_trace, "IOCS _B_COLOR; %%d1:w=0x%04x\n",
       word_size::get(c.regs.d[1]));
#endif
    static bool once;
    if (!once++)
      fprintf(stderr, "iocs_b_color: FIXME: not implemented\n");

    byte_size::put(c.regs.d[0], 3);
  }

  /* Handles a _B_CONSOL IOCS call.  */
  void
  iocs_b_consol(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    LG(nana_iocs_call_trace, "IOCS _B_CONSOL; %%d1=0x%08lx %%d2=0x%08lx\n",
       long_word_size::get(c.regs.d[1]) + 0UL,
       long_word_size::get(c.regs.d[2]) + 0UL);
#endif
    static bool once;
    if (!once++)
      fprintf(stderr, "iocs_b_consol: FIXME: not implemented\n");
  }

  /* Handles a _B_CUROFF IOCS call.  */
  void
  iocs_b_curoff(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    LG(nana_iocs_call_trace, "IOCS _B_CUROFF\n");
#endif
    static bool once;
    if (!once++)
      fprintf(stderr, "iocs_b_curoff: FIXME: not implemented\n");
  }

  /* Handles a _B_CURON IOCS call.  */
  void
  iocs_b_curon(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    LG(nana_iocs_call_trace, "IOCS _B_CURON\n");
#endif
    static bool once;
    if (!once++)
      fprintf(stderr, "iocs_b_curon: FIXME: not implemented\n");
  }

  /* Handles a _B_LOCATE IOCS call.  */
  void
  iocs_b_locate(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    LG(nana_iocs_call_trace, "IOCS _B_LOCATE; %%d1:w=0x%04x %%d2:w=0x%04x\n",
      word_size::get(c.regs.d[1]), word_size::get(c.regs.d[2]));
#endif
    static bool once;
    if (!once++)
      fprintf(stderr, "iocs_b_locate: FIXME: not implemented\n");
  }

  /* Handles a _B_PUTMES IOCS call.  */
  void
  iocs_b_putmes(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    LG(nana_iocs_call_trace,
       "IOCS _B_PUTMES; %%d1:b=0x%02x %%d2:w=0x%04x %%d3:w=0x%04x "
       "%%d4:w=0x%04x %%a1=0x%08lx\n",
       byte_size::get(c.regs.d[1]), word_size::get(c.regs.d[2]),
       word_size::get(c.regs.d[3]), word_size::get(c.regs.d[4]),
       long_word_size::get(c.regs.a[1]) + 0UL);
#endif
    static bool once;
    if (!once++)
      fprintf(stderr, "iocs_b_putmes: FIXME: not implemented\n");
  }

  /* Handles a _TEXTPUT IOCS call.  */
  void
  iocs_textput(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    LG(nana_iocs_call_trace,
       "IOCS _TEXTPUT; %%d1:w=0x%04x %%d2:w=0x%04x %%a1=0x%08lx\n",
       word_size::get(c.regs.d[1]), word_size::get(c.regs.d[2]),
       long_word_size::get(c.regs.a[1]) + 0UL);
#endif
    static bool once;
    if (!once++)
      fprintf(stderr, "iocs_textput: FIXME: not implemented\n");
  }

  /* Handles a _TXFILL IOCS call.  */
  void
  iocs_txfill(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    LG(nana_iocs_call_trace, "IOCS _TXFILL; %%a1=0x%08lx\n",
       long_word_size::get(c.regs.a[1]) + 0UL);
#endif
    static bool once;
    if (!once++)
      fprintf(stderr, "iocs_txfill: FIXME: not implemented\n");
  }
}

void
text_video_memory::install_iocs_calls(system_rom &rom)
{
  unsigned long data = reinterpret_cast<unsigned long>(this);
  // 0x1a: _TEXTGET
  rom.set_iocs_function(0x1b, make_pair(&iocs_textput, data));
  // 0x1c: _CLIPPUT
  rom.set_iocs_function(0x1e, make_pair(&iocs_b_curon, data));
  rom.set_iocs_function(0x1f, make_pair(&iocs_b_curoff, data));
  // 0x20: _B_PUTC
  // 0x21: _B_PRINT
  rom.set_iocs_function(0x22, make_pair(&iocs_b_color, data));
  rom.set_iocs_function(0x23, make_pair(&iocs_b_locate, data));
  // 0x24: _B_DOWN_S
  // 0x25: _B_UP_S
  // 0x26: _B_UP
  // 0x27: _B_DOWN
  // 0x28: _B_RIGHT
  // 0x29: _B_LEFT
  rom.set_iocs_function(0x2a, make_pair(&iocs_b_clr_st, data));
  // 0x2b: _B_ERA_ST
  // 0x2c: _B_INS
  // 0x2d: _B_DEL
  rom.set_iocs_function(0x2e, make_pair(&iocs_b_consol, data));
  rom.set_iocs_function(0x2f, make_pair(&iocs_b_putmes, data));
  rom.set_iocs_function(0xd7, make_pair(&iocs_txfill, data));
}

uint16_type
text_video_memory::get_16(function_code fc, uint32_type address) const
{
  if (fc != SUPER_DATA)
    throw bus_error_exception(true, fc, address);

  address &= PLANE_MAX * PLANE_SIZE - 1u;
  uint_type value = vm68k::getw(buf + address);
  return value;
}

int
text_video_memory::get_8(function_code fc, uint32_type address) const
{
  if (fc != SUPER_DATA)
    throw bus_error_exception(true, fc, address);

  address &= PLANE_MAX * PLANE_SIZE - 1u;
  uint_type value = *(buf + address);
  return value;
}

void
text_video_memory::put_16(function_code fc, uint32_type address,
			  uint16_type value)
{
  if (fc != SUPER_DATA)
    throw bus_error_exception(false, fc, address);

  address &= PLANE_MAX * PLANE_SIZE - 1u;
  if ((value & 0xffff) != vm68k::getw(buf + address))
    {
      vm68k::putw(buf + address, value & 0xffffu);

      unsigned int x = address % ROW_SIZE * 8;
      unsigned int y = address / ROW_SIZE % 1024u;
      mark_update_area(x, y, x + 16, y + 1);
    }
}

void
text_video_memory::put_8(function_code fc, uint32_type address,
			 int value)
{
  if (fc != SUPER_DATA)
    throw bus_error_exception(false, fc, address);

  address &= PLANE_MAX * PLANE_SIZE - 1u;
  if ((value & 0xff) != *(buf + address))
    {
      *(buf + address) = value & 0xffu;

      unsigned int x = address % ROW_SIZE * 8;
      unsigned int y = address / ROW_SIZE % 1024u;
      mark_update_area(x, y, x + 8, y + 1);
    }
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
