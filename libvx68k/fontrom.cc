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
#include <vm68k/iterator.h>
#include <cstdio>

#ifdef HAVE_NANA_H
# include <nana.h>
#else
# include <cassert>
# define I assert
#endif

using vx68k::font_rom;
using vm68k::uint16_iterator;
using namespace vm68k::types;
using namespace std;

#ifdef HAVE_NANA_H
extern bool nana_iocs_call_trace;
#endif

namespace
{
  inline unsigned int
  linear_jisx0208(unsigned int ch1, unsigned int ch2)
  {
    if (ch1 >= 0x30)
      return (ch1 - 0x30 + 8) * 94 + (ch2 - 0x21);
    else
      return (ch1 - 0x21) * 94 + (ch2 - 0x21);
  }
}

uint32_type
font_rom::jisx0201_16_offset(unsigned int ch)
{
  return ch * size_t(16 * 1) + 0x3a800;
}

uint32_type
font_rom::jisx0201_24_offset(unsigned int ch)
{
  return ch * size_t(24 * 2) + 0x3d000;
}

uint32_type
font_rom::jisx0208_16_offset(unsigned int ch1, unsigned int ch2)
{
  return linear_jisx0208(ch1, ch2) * size_t(16 * 2);
}

uint32_type
font_rom::jisx0208_24_offset(unsigned int ch1, unsigned int ch2)
{
  return linear_jisx0208(ch1, ch2) * size_t(24 * 3) + 0x40000;
}

namespace
{
  using vm68k::memory;
  using vm68k::byte_size;
  using vm68k::word_size;
  using vm68k::long_word_size;
  using vm68k::context;

  /* Handles a _DEFCHR IOCS call.  */
  void
  iocs_defchr(context &c, unsigned long data)
  {
#ifdef LG
    LG(nana_iocs_call_trace, "IOCS _DEFCHR; %%d1=0x%08lx %%a1=0x%08lx\n",
       long_word_size::get(c.regs.d[1]) + 0UL,
       long_word_size::get(c.regs.a[1]) + 0UL);
#endif
    static bool once;
    if (!once++)
      fprintf(stderr, "iocs_defchr: FIXME: not implemented\n");
    c.regs.d[0] = 0;
  }

  /* Handles a _FNTADR IOCS call.  */
  void
  iocs_fntadr(context &c, unsigned long data)
  {
    uint16_type ch = word_size::get(c.regs.d[1]);
    uint32_type size = long_word_size::get(c.regs.d[2]);
#ifdef LG
    LG(nana_iocs_call_trace, "IOCS _FNTADR; %%d1:w=0x%04x %%d2=0x%08lx\n",
       ch, size + 0UL);
#endif

    unsigned int ch1 = ch >> 8;
    unsigned int ch2 = ch & 0xff;
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
	switch (size)
	  {
	  case 6:
	    fprintf(stderr, "iocs_fntadr: FIXME: not implemented\n");
	    long_word_size::put(c.regs.d[0],
				0xf00000
				+ font_rom::jisx0208_16_offset(ch1, ch2));
	    word_size::put(c.regs.d[1], 2 - 1);
	    word_size::put(c.regs.d[2], 12 - 1);
	    break;

	  case 12:
	    long_word_size::put(c.regs.d[0],
				0xf00000
				+ font_rom::jisx0208_24_offset(ch1, ch2));
	    word_size::put(c.regs.d[1], 3 - 1);
	    word_size::put(c.regs.d[2], 24 - 1);
	    break;

	  default:
	  case 8:
	    long_word_size::put(c.regs.d[0],
				0xf00000
				+ font_rom::jisx0208_16_offset(ch1, ch2));
	    word_size::put(c.regs.d[1], 2 - 1);
	    word_size::put(c.regs.d[2], 16 - 1);
	    break;
	  }
      }
    else
      {
	switch (size)
	  {
	  case 6:
	    fprintf(stderr, "iocs_fntadr: FIXME: not implemented\n");
	    long_word_size::put(c.regs.d[0],
				0xf00000
				+ font_rom::jisx0201_16_offset(ch2));
	    word_size::put(c.regs.d[1], 1 - 1);
	    word_size::put(c.regs.d[2], 12 - 1);
	    break;

	  case 12:
	    long_word_size::put(c.regs.d[0],
				0xf00000
				+ font_rom::jisx0201_24_offset(ch2));
	    word_size::put(c.regs.d[1], 2 - 1);
	    word_size::put(c.regs.d[2], 24 - 1);
	    break;

	  default:
	  case 8:
	    long_word_size::put(c.regs.d[0],
				0xf00000
				+ font_rom::jisx0201_16_offset(ch2));
	    word_size::put(c.regs.d[1], 1 - 1);
	    word_size::put(c.regs.d[2], 16 - 1);
	    break;
	  }
      }
  }

  /* Handles a _FNTGET IOCS call.  */
  void
  iocs_fntget(context &c, unsigned long data)
  {
    uint32_type size_ch = long_word_size::get(c.regs.d[1]);
    uint32_type i = long_word_size::get(c.regs.a[1]);
#ifdef LG
    LG(nana_iocs_call_trace, "IOCS _FNTGET; %%d1=0x%08lx %%a1=0x%08lx\n",
       size_ch + 0UL, i + 0UL);
#endif

    unsigned int ch1 = size_ch >> 8 & 0xff;
    unsigned int ch2 = size_ch & 0xff;
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

    unsigned char buf[24 * 3];
    if (ch1 >= 0x21 && ch1 <= 0x7e)
      {
	switch (size_ch >> 16)
	  {
	  case 6:
	    fprintf(stderr, "iocs_fntadr: FIXME: not implemented\n");
	    break;

	  case 12:
	    word_size::put(*c.mem, memory::SUPER_DATA, i, 24);
	    word_size::put(*c.mem, memory::SUPER_DATA, i + 2, 24);
	    c.mem->read(0xf00000 + font_rom::jisx0208_24_offset(ch1, ch2),
			buf, 24 * 3, memory::SUPER_DATA);
	    c.mem->write(i + 4, buf, 24 * 3, memory::SUPER_DATA);
	    break;

	  default:
	  case 0:
	  case 8:
	    word_size::put(*c.mem, memory::SUPER_DATA, i, 16);
	    word_size::put(*c.mem, memory::SUPER_DATA, i + 2, 16);
	    c.mem->read(0xf00000 + font_rom::jisx0208_16_offset(ch1, ch2),
			buf, 16 * 2, memory::SUPER_DATA);
	    c.mem->write(i + 4, buf, 16 * 2, memory::SUPER_DATA);
	    break;
	  }
      }
    else
      {
	switch (size_ch >> 16)
	  {
	  case 6:
	    fprintf(stderr, "iocs_fntadr: FIXME: not implemented\n");
	    break;

	  case 12:
	    word_size::put(*c.mem, memory::SUPER_DATA, i, 12);
	    word_size::put(*c.mem, memory::SUPER_DATA, i + 2, 24);
	    c.mem->read(0xf00000 + font_rom::jisx0201_24_offset(ch2),
			buf, 24 * 2, memory::SUPER_DATA);
	    c.mem->write(i + 4, buf, 24 * 2, memory::SUPER_DATA);
	    break;

	  default:
	  case 0:
	  case 8:
	    word_size::put(*c.mem, memory::SUPER_DATA, i, 8);
	    word_size::put(*c.mem, memory::SUPER_DATA, i + 2, 16);
	    c.mem->read(0xf00000 + font_rom::jisx0201_16_offset(ch2),
			buf, 16 * 1, memory::SUPER_DATA);
	    c.mem->write(i + 4, buf, 16 * 1, memory::SUPER_DATA);
	    break;
	  }
      }
  }
}

void
font_rom::install_iocs_calls(system_rom &rom)
{
  unsigned long data = reinterpret_cast<unsigned long>(this);
  rom.set_iocs_call(0x0f, make_pair(&iocs_defchr, data));
  rom.set_iocs_call(0x16, make_pair(&iocs_fntadr, data));
  rom.set_iocs_call(0x19, make_pair(&iocs_fntget, data));
}

void
font_rom::copy_data(const console *c)
{
  for (unsigned int i = 0; i != 0x100; ++i)
    c->get_b16_image(i, data + jisx0201_16_offset(i), 1);

  for (unsigned int i = 0x21; i != 0x29; ++i)
    {
      for (unsigned int j = 0x21; j != 0x7f; ++j)
	c->get_k16_image(i << 8 | j, data + jisx0208_16_offset(i, j), 2);
    }
  for (unsigned int i = 0x30; i != 0x75; ++i)
    {
      for (unsigned int j = 0x21; j != 0x7f; ++j)
	c->get_k16_image(i << 8 | j, data + jisx0208_16_offset(i, j), 2);
    }
}

uint16_type
font_rom::get_16(uint32_type address, function_code fc) const
  throw (memory_exception)
{
#ifdef DL
  DL("class font_rom: get_16: fc=%d address=0x%08lx\n", fc, address + 0UL);
#endif
  address &= 0xfffff;
  if (address >= 0xc0000)
    return 0;
  else
    {
      uint32_type i = address / 2;
      unsigned char *ptr = data + 2 * i;
      uint16_type value = *uint16_iterator(ptr);
      return value;
    }
}

int
font_rom::get_8(uint32_type address, function_code fc) const
  throw (memory_exception)
{
#ifdef DL
  DL("class font_rom: get_8: fc=%d address=0x%08lx\n", fc, address + 0UL);
#endif
  address &= 0xfffff;
  if (address >= 0xc0000)
    return 0;
  else
    {
      uint16_type value = data[address];
      return value;
    }
}

void
font_rom::put_16(uint32_type address, uint16_type value, function_code fc)
  throw (memory_exception)
{
#ifdef DL
  DL("class font_rom: put_16: fc=%d address=0x%08lx value=0x%04x\n",
     fc, address + 0UL, value);
#endif
  static bool once;
  if (!once++)
    fprintf(stderr, "class font_rom: FIXME: `put_16' not implemented\n");
}

void
font_rom::put_8(uint32_type address, int value, function_code fc)
  throw (memory_exception)
{
#ifdef DL
  DL("class font_rom: put_8: fc=%d address=0x%08lx value=0x%02x\n",
     fc, address + 0UL, value);
#endif
  static bool once;
  if (!once++)
    fprintf(stderr, "class font_rom: FIXME: `put_8' not implemented\n");
}

font_rom::~font_rom()
{
  delete [] data;
}

font_rom::font_rom()
  : data(NULL)
{
  data = new unsigned char [0xc0000];
}
