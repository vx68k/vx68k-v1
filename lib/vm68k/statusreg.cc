/* vx68k - Virtual X68000
   Copyright (C) 1998, 1999 Hypercore Software Design, Ltd.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307, USA.  */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#undef const
#undef inline

#include <vm68k/cpu.h>

using namespace vm68k;

namespace
{
  struct result_cc_evaluator
    : cc_evaluator
  {
    bool ls(const sint32_type *values) const
      {return uint32_type(values[0]) <= 0;}
    bool cs(const sint32_type *values) const
      {return false;}
    bool eq(const sint32_type *values) const
      {return values[0] == 0;}
    bool mi(const sint32_type *values) const
      {return values[0] < 0;}
    bool lt(const sint32_type *values) const
      {return values[0] < 0;}
  };

  struct asr_cc_evaluator
    : result_cc_evaluator
  {
    bool ls(const sint32_type *values) const
      {return result_cc_evaluator::ls(values);}	// FIXME
    bool cs(const sint32_type *values) const
      {return (values[2] == 0
	       ? result_cc_evaluator::cs(values)
	       : (uint32_type(values[1]) >> values[2] - 1 & 1) != 0);}
  };

  const result_cc_evaluator result_cc;
  const asr_cc_evaluator asr_cc;
} // (unnamed namespace)

/* Sets the condition codes by a result.  */
void
status_register::set_cc(int32 r)
{
  cc_eval = &result_cc;
  cc_values[0] = r;
}

void
status_register::set_cc_asr(sint32_type r, sint32_type d, uint_type s)
{
  cc_eval = &asr_cc;
  cc_values[0] = r;
  cc_values[1] = d;
  cc_values[2] = s;
}

status_register::status_register()
  : cc_eval(&result_cc),
    value(S)
{
}

