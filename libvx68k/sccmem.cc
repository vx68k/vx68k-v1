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

#include <cstdio>

#ifdef HAVE_NANA_H
# include <nana.h>
#else
# include <cassert>
# define I assert
#endif

using vx68k::scc_memory;
using vm68k::mutex_lock;
using namespace vm68k::types;
using namespace std;

#ifdef HAVE_NANA_H
extern bool nana_iocs_call_trace;
#endif

bool
scc_memory::mouse_state(unsigned int button) const
{
  mutex_lock lock(&mutex);

  if (button >= mouse_states.size())
    throw out_of_range("class scc_memory");

  return mouse_states[button];
}

void
scc_memory::set_mouse_state(unsigned int button, bool state)
{
  mutex_lock lock(&mutex);

  if (button >= mouse_states.size())
    throw out_of_range("class scc_memory");

  mouse_states[button] = state;
}

scc_memory::point
scc_memory::mouse_position() const
{
  mutex_lock lock(&mutex);

  return _mouse_position;
}

void
scc_memory::set_mouse_position(int x, int y)
{
  mutex_lock lock(&mutex);

  if (x < mouse_left)
    x = mouse_left;
  else if (x >= mouse_right)
    x = mouse_right - 1;
  if (y < mouse_top)
    y = mouse_top;
  else if (y >= mouse_bottom)
    y = mouse_bottom - 1;

  _mouse_position.x = x;
  _mouse_position.y = y;
}

scc_memory::point
scc_memory::mouse_motion() const
{
  mutex_lock lock(&mutex);

  return _mouse_motion;
}

void
scc_memory::track_mouse()
{
  mutex_lock lock(&mutex);

  _mouse_motion.x = _mouse_position.x - old_mouse_position.x;
  _mouse_motion.y = _mouse_position.y - old_mouse_position.y;

  old_mouse_position = _mouse_position;
}

void
scc_memory::set_mouse_bounds(int l, int t, int r, int b)
{
  mutex_lock lock(&mutex);

  mouse_left = l;
  mouse_top = t;
  mouse_right = r;
  mouse_bottom = b;
}

void
scc_memory::initialize_mouse()
{
  mutex_lock lock(&mutex);

  mouse_left = 0;
  mouse_top = 0;
  mouse_right = 768;
  mouse_bottom = 512;

  _mouse_motion.x = 0;
  _mouse_motion.y = 0;
}

namespace
{
  using vm68k::word_size;
  using vm68k::long_word_size;
  using vm68k::context;
  using vx68k::system_rom;

  /* Handles a _MS_CURGT IOCS call.  */
  void
  iocs_ms_curgt(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    LG(nana_iocs_call_trace, "IOCS _MS_CURGT\n");
#endif
    scc_memory *m = reinterpret_cast<scc_memory *>(data);

    scc_memory::point position = m->mouse_position();

    uint32_type value = (uint32_type(position.x & 0xffff) << 16
			 | uint32_type(position.y & 0xffff));

    long_word_size::put(c.regs.d[0], value);
  }

  /* Handles a _MS_CUROF IOCS call.  */
  void
  iocs_ms_curof(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    LG(nana_iocs_call_trace, "IOCS _MS_CUROF\n");
#endif
    static bool once;
    if (!once++)
      fprintf(stderr, "iocs_ms_curof: FIXME: not implemented\n");
  }

  /* Handles a _MS_CURON IOCS call.  */
  void
  iocs_ms_curon(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    LG(nana_iocs_call_trace, "IOCS _MS_CURON\n");
#endif
    static bool once;
    if (!once++)
      fprintf(stderr, "iocs_ms_curon: FIXME: not implemented\n");
  }

  /* Handles a _MS_CURST IOCS call.  */
  void
  iocs_ms_curst(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    LG(nana_iocs_call_trace,
       "IOCS _MS_CURST; %%d1=0x%08lx\n",
       long_word_size::get(c.regs.d[1]) + 0UL);
#endif

    static bool once;
    if (!once++)
      fprintf(stderr, "iocs_ms_curst: FIXME: not implemented\n");
  }

  /* Handles a _MS_GETDT IOCS call.  */
  void
  iocs_ms_getdt(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    LG(nana_iocs_call_trace, "IOCS _MS_GETDT\n");
#endif
    scc_memory *m = reinterpret_cast<scc_memory *>(data);

    bool left_button = m->mouse_state(0);
    bool right_button = m->mouse_state(1);
    scc_memory::point delta = m->mouse_motion();

    if (delta.x < -127)
      delta.x = -127;
    else if (delta.x > 127)
      delta.x = 127;
    if (delta.y < -127)
      delta.y = -127;
    else if (delta.y > 127)
      delta.y = 127;

    uint32_type value
      = (uint32_type(delta.x & 0xff) << 24 | uint32_type(delta.y & 0xff) << 16
	 | (left_button ? 0xff : 0) << 8 | (right_button ? 0xff : 0));

    long_word_size::put(c.regs.d[0], value);
  }

  /* Handles a _MS_INIT IOCS call.  */
  void
  iocs_ms_init(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    LG(nana_iocs_call_trace, "IOCS _MS_INIT\n");
#endif
    scc_memory *m = reinterpret_cast<scc_memory *>(data);

    m->initialize_mouse();
  }

  /* Handles a _MS_LIMIT IOCS call.  */
  void
  iocs_ms_limit(context &c, unsigned long data)
  {
    long_word_size::uvalue_type tl = long_word_size::get(c.regs.d[1]);
    long_word_size::uvalue_type br = long_word_size::get(c.regs.d[2]);
#ifdef HAVE_NANA_H
    LG(nana_iocs_call_trace, "IOCS _MS_LIMIT; %%d1=0x%08lx %%d2=0x%08lx\n",
       tl + 0UL, br + 0UL);
#endif
    scc_memory *m = reinterpret_cast<scc_memory *>(data);

    m->set_mouse_bounds(tl >> 16, tl & 0xffff, br >> 16, br & 0xffff);
  }

  /* Handles a _MS_STAT IOCS call.  */
  void
  iocs_ms_stat(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    LG(nana_iocs_call_trace, "IOCS _MS_STAT\n");
#endif
    static bool once;
    if (!once++)
      fprintf(stderr, "iocs_ms_stat: FIXME: not implemented\n");

    word_size::put(c.regs.d[0], 0);
  }

  /* Handles a _SET232C IOCS call.  */
  void
  iocs_set232c(context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    LG(nana_iocs_call_trace,
       "IOCS _SET232C; %%d1:w=0x%04x\n", word_size::get(c.regs.d[1]));
#endif

    static bool once;
    if (!once++)
      fprintf(stderr, "iocs_set232c: FIXME: not implemented\n");
    c.regs.d[0] = 0;
  }
}

void
scc_memory::install_iocs_calls(system_rom &rom)
{
  unsigned long data = reinterpret_cast<unsigned long>(this);
  rom.set_iocs_function(0x30, make_pair(&iocs_set232c, data));
  // 0x31: _LOF232C
  // 0x32: _INP232C
  // 0x33: _ISNS232C
  // 0x34: _OSNS232C
  // 0x35: _OUT232C
  rom.set_iocs_function(0x70, make_pair(&iocs_ms_init, data));
  rom.set_iocs_function(0x71, make_pair(&iocs_ms_curon, data));
  rom.set_iocs_function(0x72, make_pair(&iocs_ms_curof, data));
  rom.set_iocs_function(0x73, make_pair(&iocs_ms_stat, data));
  rom.set_iocs_function(0x74, make_pair(&iocs_ms_getdt, data));
  rom.set_iocs_function(0x75, make_pair(&iocs_ms_curgt, data));
  rom.set_iocs_function(0x76, make_pair(&iocs_ms_curst, data));
  rom.set_iocs_function(0x77, make_pair(&iocs_ms_limit, data));
  // 0x78: _MS_OFFTM
  // 0x79: _MS_ONTM
  // 0x7a: _MS_PATST
  // 0x7b: _MS_SEL
  // 0x7c: _MS_SEL2
}

uint16_type
scc_memory::get_16(function_code fc, uint32_type address) const
{
#ifdef HAVE_NANA_H
  DL("class scc_memory: get_16: fc=%d address=0x%08lx\n", fc, address + 0UL);
#endif
  static bool once;
  if (!once++)
    fprintf(stderr, "class scc_memory: FIXME: `get_16' not implemented\n");
  return 0;
}

int
scc_memory::get_8(function_code fc, uint32_type address) const
{
#ifdef HAVE_NANA_H
  DL("class scc_memory: get_8: fc=%d address=0x%08lx\n", fc, address + 0UL);
#endif
  static bool once;
  if (!once++)
    fprintf(stderr, "class scc_memory: FIXME: `get_8' not implemented\n");
  address &= 0x1fff;
  switch (address) {
  default:
    return 0;
  }
}

void
scc_memory::put_16(function_code fc, uint32_type address, uint16_type value)
{
#ifdef HAVE_NANA_H
  DL("class scc_memory: put_16: fc=%d address=0x%08lx value=0x%04x\n",
     fc, address + 0UL, value);
#endif
  static bool once;
  if (!once++)
    fprintf(stderr, "class scc_memory: FIXME: `put_16' not implemented\n");
}

void
scc_memory::put_8(function_code fc, uint32_type address, int value)
{
#ifdef HAVE_NANA_H
  DL("class scc_memory: put_8: fc=%d address=0x%08lx value=0x%02x\n",
     fc, address + 0UL, value);
#endif
  static bool once;
  if (!once++)
    fprintf(stderr, "class scc_memory: FIXME: `put_8' not implemented\n");
}

scc_memory::~scc_memory()
{
  pthread_mutex_destroy(&mutex);
}

scc_memory::scc_memory()
  : mouse_left(0), mouse_top(0), mouse_right(768), mouse_bottom(512),
    mouse_states(2, false)
{
  pthread_mutex_init(&mutex, 0);
}
