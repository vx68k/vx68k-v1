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
#include <vm68k/mutex.h>

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
machine::b_print(const memory_address_space *as, uint32_type strptr)
{
  const string str = as->get_string(memory::SUPER_DATA, strptr);

  for (string::const_iterator i = str.begin();
       i != str.end();
       ++i)
    {
      b_putc((unsigned char) *i);
    }
}

sint32_type
machine::read_disk(memory_address_space &as, uint_type mode, uint32_type pos,
		   uint32_type buf, uint32_type nbytes)
{
  uint_type u = mode >> 8 & 0xf;

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
machine::write_disk(const memory_address_space &as,
		    uint_type mode, uint32_type pos,
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
      uint_type op = c.mem->get_16(memory::SUPER_DATA, c.regs.pc);

      if ((op & 0xf000u) == 0xf000u)
	{
	  uint_type oldsr = c.sr();
	  c.set_supervisor_state(true);
	  c.regs.a[7] -= 6;
	  c.mem->put_32(memory::SUPER_DATA, c.regs.a[7] + 2, c.regs.pc);
	  c.mem->put_16(memory::SUPER_DATA, c.regs.a[7] + 0, oldsr);
	  c.regs.pc = c.mem->get_32(memory::SUPER_DATA, 11u * 4u);
	  goto rerun;
	}
      else if ((op & 0xfff0u) == 0x4e40)
	{
	  c.regs.pc += 2;

	  uint_type oldsr = c.sr();
	  c.set_supervisor_state(true);
	  c.regs.a[7] -= 6;
	  c.mem->put_32(memory::SUPER_DATA, c.regs.a[7] + 2, c.regs.pc);
	  c.mem->put_16(memory::SUPER_DATA, c.regs.a[7] + 0, oldsr);
	  c.regs.pc = c.mem->get_32(memory::SUPER_DATA, ((op & 0xfu) + 32) * 4u);
	  goto rerun;
	}

      throw;
    }
  catch (special_exception &e)
    {
      uint32_type vecaddr = e.vecno * 4u;
      uint32_type addr = c.mem->get_32(memory::SUPER_DATA, vecaddr);
      if (addr != vecaddr + 0xfe0000)
	{
#ifdef HAVE_NANA_H
	  L("machine: Installed bus/address error handler used\n");
#endif
	  uint_type oldsr = c.sr();
	  c.set_supervisor_state(true);
	  c.regs.a[7] -= 14;
	  c.mem->put_32(memory::SUPER_DATA, c.regs.a[7] + 10, c.regs.pc);
	  c.mem->put_16(memory::SUPER_DATA, c.regs.a[7] + 8, oldsr);
	  c.mem->put_16(memory::SUPER_DATA, c.regs.a[7] + 6,
		      c.mem->get_16(memory::SUPER_DATA, c.regs.pc));
	  c.mem->put_32(memory::SUPER_DATA, c.regs.a[7] + 2, e.address);
	  c.mem->put_16(memory::SUPER_DATA, c.regs.a[7] + 0, e.status);
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
  mutex_lock lock(&key_queue_mutex);

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

  mutex_lock lock(&key_queue_mutex);

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
  mutex_lock lock(&key_queue_mutex);

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

#if 0
void
machine::update_image(unsigned char *rgb_buf, size_t row_size,
		      unsigned int width, unsigned int height,
		      rectangle &update_area)
{
  vector<bool> update = tvram.poll_update();
  vector<bool>::iterator u = update.begin();

  unsigned char *image_end = rgb_buf + height * row_size;
  unsigned char *update_begin = image_end;
  unsigned char *update_end = image_end;

  bool tc_modified = palettes.check_text_colors_modified();
  unsigned char text_colors[16 * 4];
  palettes.get_text_colors(0, 16, text_colors);

  for (unsigned char *i = rgb_buf; i != image_end;)
    {
      if (tc_modified || *u)
	{
	  unsigned char *row_end = i + width * 3;

	  for (unsigned char *j = i; j != row_end;)
	    {
	      *j++ = 0;
	      *j++ = 0;
	      *j++ = 0;
	    }

	  text_video_memory::raster_iterator r
	    = tvram.raster(0, (i - rgb_buf) / row_size);
	  for (unsigned char *j = i; j != row_end;)
	    {
	      const unsigned char *k = text_colors + *r++ * 4;
	      if (k[3] == 0)
		j += 3;
	      else
		{
		  *j++ = *k++;
		  *j++ = *k++;
		  *j++ = *k++;
		}
	    }

	  if (update_begin > i)
	    update_begin = i;
	  update_end = i + row_size;
	}

      I((i - rgb_buf) % row_size == 0);
      i += row_size;
      ++u;
    }

  I((update_begin - rgb_buf) % row_size == 0);
  I((update_end - rgb_buf) % row_size == 0);
  update_area.left_x = 0;
  update_area.top_y = (update_begin - rgb_buf) / row_size;
  update_area.right_x = width;
  update_area.bottom_y = (update_end - rgb_buf) / row_size;
}
#endif

void
machine::check_timers(uint32_type t)
{
  crtc.check_timeouts(t, *master_context());
  opm.check_timeouts(t, *master_context());
  scc.track_mouse();
  last_check_time = t;
}

void
machine::connect(console *c)
{
  console::time_type t = c->current_time();
  crtc.reset(t);
  opm.reset(t);
  tvram.connect(c);
  font.copy_data(c);
}

void
machine::configure(memory_address_space &as)
{
  as.fill(0, _memory_size, &mem);
  as.fill(0xc00000, 0xe00000, &gv);
  as.fill(0xe00000, 0xe80000, &tvram);
  as.fill(0xe80000, 0xe82000, &crtc);
  as.fill(0xe82000, 0xe84000, &palettes);
  as.fill(0xe84000, 0xe86000, &dmac);
  as.fill(0xe86000, 0xe88000, &_area_set);
  as.fill(0xe88000, 0xe8a000, &mfp);
  as.fill(0xe8e000, 0xe90000, &system_ports);
  as.fill(0xe90000, 0xe92000, &opm);
  as.fill(0xe98000, 0xe9a000, &scc);
  as.fill(0xe9a000, 0xe9c000, &ppi);
  as.fill(0xeb0000, 0xeb8000, &sprites);
  as.fill(0xed0000, 0xed4000, &_sram);
  as.fill(0xf00000, 0xfc0000, &font);
  as.fill(0xfc0000, 0x1000000, &rom);

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
  tvram.install_iocs_calls(rom);
  dmac.install_iocs_calls(rom);
  scc.install_iocs_calls(rom);
  font.install_iocs_calls(rom);
}
