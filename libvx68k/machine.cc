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

/* NOTE

   The IOCS implementation is being moved to class system_rom.  */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#undef const
#undef inline

#include <vx68k/machine.h>
#include <vx68k/utility.h>

#include <algorithm>
#include <memory>
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

void
machine::b_putc(uint_type code)
{
  /* FIXME.  This function must handle double-byte character.  */
  if (code <= 0x1f || code == 0x7f)
    {
      saved_byte1 = 0;

      switch (code)
	{
	case 0x09:		// HT
	  curx = (curx + 8) & ~7;
	  break;
	  
	case 0x0a:		// LF
	  ++cury;
	  if (cury == 31)
	    {
	      --cury;
	      tvram.scroll();
	    }
	  break;

	case 0x0d:		// CR
	  curx = 0;
	  break;
	}
    }
  else
    {
      if (saved_byte1 != 0)
	{
	  if (code <= 0xff)
	    code |= saved_byte1 << 8;

	  saved_byte1 = 0;
	}

      if (code >= 0x100 && curx + 1 == 96)
	++curx;

      if (curx == 96)
	{
	  curx = 0;
	  ++cury;

	  if (cury == 31)
	    {
	      --cury;
	      tvram.scroll();
	    }
	}

      if ((code >= 0x80 && code <= 0x9f)
	  || (code >= 0xe0 && code <= 0xff))
	saved_byte1 = code;
      else
	{
	  tvram.draw_char(curx, cury, code);
	  ++curx;
	  if (code >= 0x100)
	    ++curx;
	}
    }
}

void
machine::b_print(const address_space *as, uint32_type strptr)
{
  const string str = as->gets(SUPER_DATA, strptr);

  for (string::const_iterator i = str.begin();
       i != str.end();
       ++i)
    {
      b_putc((unsigned char) *i);
    }
}

sint32_type
machine::read_disk(address_space &as, uint_type mode, uint32_type pos,
		   uint32_type buf, uint32_type nbytes)
{
  uint_type u = mode >> 8 & 0xf;

#ifdef HAVE_NANA_H
  L("machine: reading disk %#x %#x %#x %#x\n", mode, pos, buf, nbytes);
#endif
  switch (mode >> 12)
    {
    case 0x9:
      if (u >= NFDS)
	throw range_error("read_disk");

      return fd[u]->read(mode, pos, as, buf, nbytes);

    default:
      abort();
    }
}

sint32_type
machine::write_disk(const address_space &as, uint_type mode, uint32_type pos,
		    uint32_type buf, uint32_type nbytes) const
{
  uint_type u = mode >> 8 & 0xf;

#ifdef HAVE_NANA_H
  L("machine: writing disk %#x %#x %#x %#x\n", mode, pos, buf, nbytes);
#endif
  switch (mode >> 12)
    {
    case 0x9:
      if (u >= NFDS)
	throw range_error("write_disk");

      return fd[u]->write(mode, pos, as, buf, nbytes);

    default:
      abort();
    }
}

void
machine::boot(context &c)
{
  /* c.mem must be of class x68k_address_space.  */

  sint32_type st = read_disk(*c.mem, 0x9070, 0x03000001, 0x2000, 1024);
  if (st >> 24 & 0xc0)
    {
#ifdef HAVE_NANA_H
      L("machine: boot error %#x\n", st);
#endif
      throw runtime_error("machine");
    }

  c.regs.pc = 0x2000;
 rerun:
  try
    {
      eu.run(c);
    }
  catch (illegal_instruction_exception &e)
    {
      uint_type op = c.mem->getw(SUPER_DATA, c.regs.pc);

      if ((op & 0xf000u) == 0xf000u)
	{
	  uint_type oldsr = c.sr();
	  c.set_supervisor_state(true);
	  c.regs.a[7] -= 6;
	  c.mem->putl(SUPER_DATA, c.regs.a[7] + 2, c.regs.pc);
	  c.mem->putw(SUPER_DATA, c.regs.a[7] + 0, oldsr);
	  c.regs.pc = c.mem->getl(SUPER_DATA, 11u * 4u);
	  goto rerun;
	}
      else if ((op & 0xfff0u) == 0x4e40)
	{
	  c.regs.pc += 2;

	  uint_type oldsr = c.sr();
	  c.set_supervisor_state(true);
	  c.regs.a[7] -= 6;
	  c.mem->putl(SUPER_DATA, c.regs.a[7] + 2, c.regs.pc);
	  c.mem->putw(SUPER_DATA, c.regs.a[7] + 0, oldsr);
	  c.regs.pc = c.mem->getl(SUPER_DATA, ((op & 0xfu) + 32) * 4u);
	  goto rerun;
	}

      throw;
    }
  catch (special_exception &e)
    {
      uint32_type vecaddr = e.vecno * 4u;
      uint32_type addr = c.mem->getl(SUPER_DATA, vecaddr);
      if (addr != vecaddr + 0xfe0000)
	{
#ifdef HAVE_NANA_H
	  L("machine: Installed bus/address error handler used\n");
#endif
	  uint_type oldsr = c.sr();
	  c.set_supervisor_state(true);
	  c.regs.a[7] -= 14;
	  c.mem->putl(SUPER_DATA, c.regs.a[7] + 10, c.regs.pc);
	  c.mem->putw(SUPER_DATA, c.regs.a[7] + 8, oldsr);
	  c.mem->putw(SUPER_DATA, c.regs.a[7] + 6,
		      c.mem->getw(SUPER_DATA, c.regs.pc));
	  c.mem->putl(SUPER_DATA, c.regs.a[7] + 2, e.address);
	  c.mem->putw(SUPER_DATA, c.regs.a[7] + 0, e.status);
	  c.regs.pc = addr;
	  goto rerun;
	}

      throw;
    }
}

void
machine::boot()
{
  boot(*master_context());
}

void
machine::queue_key(uint_type key)
{
  auto_lock<pthread_mutex_t> lock(&key_queue_mutex);

  key_queue.push(key);
  pthread_cond_signal(&key_queue_not_empty);
}

uint_type
machine::peek_key()
{
  if (key_queue.empty())
    {
      sched_yield();
      pthread_testcancel();
    }

  auto_lock<pthread_mutex_t> lock(&key_queue_mutex);

  uint_type key;
  if (key_queue.empty())
    key = 0;
  else
    key = key_queue.front();

  return key;
}

uint_type
machine::get_key()
{
  auto_lock<pthread_mutex_t> lock(&key_queue_mutex);

  while (key_queue.empty())
    pthread_cond_wait(&key_queue_not_empty, &key_queue_mutex);

  uint_type key = key_queue.front();
  key_queue.pop();

  return key;
}

void
machine::set_key_modifiers(uint_type mask, uint_type value)
{
  _key_modifiers = (_key_modifiers & ~mask) ^ value;
}

void
machine::load_fd(unsigned int u, int fildes)
{
  if (u >= NFDS)
    throw range_error("machine");

  auto_ptr<iocs::image_file_floppy_disk> d
    (new iocs::image_file_floppy_disk(fildes));

  unload_fd(u);
  fd[u] = d.release();
}

void
machine::unload_fd(unsigned int u)
{
  if (u >= NFDS)
    throw range_error("machine");

  delete fd[u];
  fd[u] = NULL;
}

void
machine::get_image(int x, int y, int width, int height,
		   unsigned char *rgb_buf, size_t row_size)
{
  for (int i = 0; i != height; ++i)
    {
      for (int j = 0; j != width; ++j)
	{
	  unsigned char *p = rgb_buf + i * row_size + j * 3;
	  p[0] = 0;
	  p[1] = 0;
	  p[2] = 0;
	}

      text_video_memory::raster_iterator q = tvram.raster(x, y + i);
      for (int j = 0; j != width; ++j)
	{
	  static const unsigned char vv[16 * 3] = {0, 0, 0,
						   0, 255, 255,
						   255, 255, 0,
						   255, 255, 255};
	  uint_type v = 3 * *q++;
	  unsigned char *p = rgb_buf + i * row_size + j * 3;
	  p[0] = vv[v];
	  p[1] = vv[v + 1];
	  p[2] = vv[v + 2];
	}
    }
}

void
machine::check_timers(uint32_type t)
{
  opm.check_timeout(*master_context());
  last_check_time = t;
}

void
machine::configure(address_space &as)
{
  as.set_pages(0 >> PAGE_SHIFT, _memory_size >> PAGE_SHIFT, &mem);
#if 0
  as.set_pages(0xc00000 >> PAGE_SHIFT, 0xe00000 >> PAGE_SHIFT, &graphic_vram);
#endif
  as.set_pages(0xe00000 >> PAGE_SHIFT, 0xe80000 >> PAGE_SHIFT, &tvram);
  as.set_pages(0xe80000 >> PAGE_SHIFT, 0xe82000 >> PAGE_SHIFT, &crtc);
  as.set_pages(0xe82000 >> PAGE_SHIFT, 0xe84000 >> PAGE_SHIFT, &palettes);
  as.set_pages(0xe86000 >> PAGE_SHIFT, 0xe88000 >> PAGE_SHIFT, &_area_set);
  as.set_pages(0xe90000 >> PAGE_SHIFT, 0xe92000 >> PAGE_SHIFT, &opm);
  as.set_pages(0xe98000 >> PAGE_SHIFT, 0xe9a000 >> PAGE_SHIFT, &scc);
  as.set_pages(0xe9a000 >> PAGE_SHIFT, 0xe9c000 >> PAGE_SHIFT, &ppi);
  as.set_pages(0xeb0000 >> PAGE_SHIFT, 0xeb8000 >> PAGE_SHIFT, &sprites);
  as.set_pages(0xed0000 >> PAGE_SHIFT, 0xed4000 >> PAGE_SHIFT, &_sram);
  as.set_pages(0xf00000 >> PAGE_SHIFT, 0xfc0000 >> PAGE_SHIFT, &font);
  as.set_pages(0xfc0000 >> PAGE_SHIFT, 0x1000000 >> PAGE_SHIFT, &rom);

  rom.initialize(as);
}

machine::~machine()
{
  for (iocs::disk **i = fd + 0; i != fd + NFDS; ++i)
    delete *i;

  rom.detach(&eu);

  pthread_mutex_destroy(&key_queue_mutex);
  pthread_cond_destroy(&key_queue_not_empty);
}

machine::machine(size_t memory_size)
  : _memory_size(memory_size),
    mem(memory_size),
    _area_set(&mem),
    master_as(new x68k_address_space(this)),
    _master_context(new context(master_as.get())),
    _key_modifiers(0),
    curx(0), cury(0),
    saved_byte1(0)
{
  pthread_cond_init(&key_queue_not_empty, NULL);
  pthread_mutex_init(&key_queue_mutex, NULL);

  fill(fd + 0, fd + NFDS, (iocs::disk *) NULL);

  rom.attach(&eu);
}
