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

#ifdef HAVE_NANA_H
# include <nana.h>
# include <cstdio>
#else
# include <cassert>
# define I assert
#endif

using vx68k::opm_memory;
using vm68k::bus_error_exception;
using namespace vm68k::types;
using namespace std;

void
opm_memory::reset(console::time_type t)
{
  last_check_time = t;
}

void
opm_memory::check_timeouts(console::time_type t, context &c)
{
  last_check_time = t;

  unsigned int old_status = status();
  unsigned int tcr = _regs[0x14];

  if ((tcr & 0x1) == 0x1 && (t - timer_a_start_time) >= timer_a_interval)
    {
      _status |= 0x2;
      timer_a_start_time += timer_a_interval;
    }
  if ((tcr & 0x2) == 0x2 && (t - timer_b_start_time) >= timer_b_interval)
    {
      _status |= 0x1;
      timer_a_start_time += timer_b_interval;
    }

  if (_interrupt_enabled)
    {
      unsigned int set_status = status() - ~old_status;
      if ((tcr & 0x4) == 0x4 && (set_status & 0x2) == 0x2
	  || (tcr & 0x8) == 0x8 && (set_status & 0x1) == 0x1)
	{
	  c.interrupt(6, 0x43);
	}
    }
}

void
opm_memory::set_reg(unsigned int regno, unsigned int value)
{
  regno &= 0xffu;
  value &= 0xffu;
  _regs[regno] = value;

  switch (regno)
    {
    case 0x10:
    case 0x11:
      {
	unsigned int k = _regs[0x10] << 2 | _regs[0x11] & 0x3;
	timer_a_interval = (0x400 - k) * 64 / 4000;
	timer_a_start_time = last_check_time;
      }
      break;

    case 0x12:
      {
	unsigned int k = _regs[0x12];
	timer_b_interval = (0x100 - k) * 1024 / 4000;
	timer_b_start_time = last_check_time;
      }
      break;

    case 0x14:
      {
	if ((value & 0x10) == 0x10)
	  _status &= ~0x2;
	if ((value & 0x20) == 0x20)
	  _status &= ~0x1;
      }
      break;

    default:
      break;
    }
}

void
opm_memory::set_interrupt_enabled(bool value)
{
  _interrupt_enabled = value;
}

unsigned int
opm_memory::get_8(int fc, uint32_type address) const
{
  address &= 0xffffffffu;
#ifdef HAVE_NANA_H
  L("class opm_memory: get_8: fc=%d address=%#010lx\n",
    fc, (unsigned long) address);
#endif

  switch (address & 0x1fffu) {
  case 0x1:
  case 0x3:
    {
      unsigned int value = status();
      return value;
    }
    break;

  default:
    {
      throw bus_error_exception(true, fc, address);
    }
    break;
  }
}

uint_type
opm_memory::get_16(int fc, uint32_type address) const
{
#ifdef HAVE_NANA_H
  L("class opm_memory: get_16: fc=%d address=%#010lx\n",
    fc, (unsigned long) address & 0xffffffffu);
#endif

  return get_8(fc, address | 0x1u);
}

void
opm_memory::put_8(int fc, uint32_type address, unsigned int value)
{
  address &= 0xffffffffu;
  value &= 0xffu;
#ifdef HAVE_NANA_H
  L("class opm_memory: put_8: fc=%d address=%#010lx value=%#04x\n",
    fc, (unsigned long) address, value);
#endif
  if (fc != vm68k::SUPER_DATA)
    throw bus_error_exception(false, fc, address);

  address &= 0x1fffu;
  switch (address)
    {
    case 0x1:
      reg_index = value;
      break;

    case 0x3:
      set_reg(reg_index, value);
      break;

    default:
      throw bus_error_exception(false, fc, address);
      break;
    }
}

void
opm_memory::put_16(int fc, uint32_type address, uint_type value)
{
  address &= 0xffffffffu;
  value &= 0xffffu;
#ifdef HAVE_NANA_H
  L("class opm_memory: put_16: fc=%d address=%#010lx value=%#06x\n",
    fc, (unsigned long) address, value);
#endif

  this->put_8(fc, address | 0x1u, value);
}

opm_memory::~opm_memory()
{
}

opm_memory::opm_memory()
  : _status(0),
    _regs(0x100, 0),
    _interrupt_enabled(false)
{
  reg_index = 0;
}
