/* vx68k - Virtual X68000
   Copyright (C) 1998-2000 Hypercore Software Design, Ltd.

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

#include <vm68k/processor.h>

using vm68k::condition_code;
using vm68k::bitset_condition_tester;
using namespace vm68k::types;

namespace
{
  using vm68k::condition_tester;

  /* Condition tester for general case.  */
  class general_condition_tester
    : public condition_tester
  {
  public:
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
    bool le(const sint32_type *values) const
    {return values[0] <= 0;}
  };

  /* Condition tester for ADD.  */
  class add_condition_tester
    : public general_condition_tester
  {
  public:
    bool ls(const sint32_type *values) const
    {return eq(values) || cs(values);}

    bool cs(const sint32_type *values) const
    {
      return (values[0] >= 0 && (values[1] < 0 || values[2] < 0)
	      || values[1] < 0 && values[2] < 0);
    }

    // FIXME: Check if these tests are correct.
    bool lt(const sint32_type *values) const
    {return values[1] < 1 - values[2];}
    bool le(const sint32_type *values) const
    {return values[1] <= 1 - values[2];}
  };

  /* Condition tester for SUB and CMP.  */
  class sub_condition_tester
    : public general_condition_tester
  {
  public:
    bool ls(const sint32_type *values) const
      {return uint32_type(values[1]) <= uint32_type(values[2]);}
    bool cs(const sint32_type *values) const
      {return uint32_type(values[1]) < uint32_type(values[2]);}
    bool lt(const sint32_type *values) const
      {return values[1] < values[2];}
    bool le(const sint32_type *values) const
      {return values[1] <= values[2];}
  };

  /* Condition tester for ASR.  */
  class asr_condition_tester
    : public general_condition_tester
  {
  public:
    bool ls(const sint32_type *values) const
      {return values[0] == 0 || cs(values);}
    bool cs(const sint32_type *values) const
      {return (values[2] >= 1
	       && uint32_type(values[1]) & uint32_type(1) << values[2] - 1);}
  };

  /* Condition tester for LSL.  */
  class lsl_condition_tester
    : public general_condition_tester
  {
  public:
    bool ls(const sint32_type *values) const
      {return values[0] == 0 || cs(values);}
    bool cs(const sint32_type *values) const
      {return (values[2] >= 1
	       && uint32_type(values[1]) & uint32_type(1) << 32 - values[2]);}
  };

  const general_condition_tester const_general_condition_tester;
  const add_condition_tester const_add_condition_tester;
  const sub_condition_tester const_sub_condition_tester;
  const asr_condition_tester const_asr_condition_tester;
  const lsl_condition_tester const_lsl_condition_tester;
} // (unnamed namespace)

void
condition_code::set_cc_cmp(sint32_type r, sint32_type d, sint32_type s)
{
  cc_eval = &const_sub_condition_tester;
  cc_values[0] = r;
  cc_values[1] = d;
  cc_values[2] = s;
}

void
condition_code::set_cc_sub(sint32_type r, sint32_type d, sint32_type s)
{
  x_eval = cc_eval = &const_sub_condition_tester;
  x_values[0] = cc_values[0] = r;
  x_values[1] = cc_values[1] = d;
  x_values[2] = cc_values[2] = s;
}

void
condition_code::set_cc_asr(sint32_type r, sint32_type d, uint_type s)
{
  x_eval = cc_eval = &const_asr_condition_tester;
  x_values[0] = cc_values[0] = r;
  x_values[1] = cc_values[1] = d;
  x_values[2] = cc_values[2] = s;
}

void
condition_code::set_cc_lsl(sint32_type r, sint32_type d, uint_type s)
{
  x_eval = cc_eval = &const_lsl_condition_tester;
  x_values[0] = cc_values[0] = r;
  x_values[1] = cc_values[1] = d;
  x_values[2] = cc_values[2] = s;
}

condition_code::operator uint16_type() const
{
  uint16_type v = value & 0xff00;
  if (cs())
    v |= 0x01;
  if (eq())
    v |= 0x04;
  if (mi())
    v |= 0x08;
  if (x())
    v |= 0x10;

  return v;
}

condition_code::condition_code()
  : cc_eval(general_condition_tester),
    x_eval(general_condition_tester),
    value(S)
{
}

const bitset_condition_tester condition_code::bitset_tester;
const condition_tester *const
condition_code::general_condition_tester = &const_general_condition_tester;
const condition_tester *const
condition_code::add_condition_tester = &const_add_condition_tester;

bool
bitset_condition_tester::cs(const sint32_type *v) const
{
  return v[0] & 0x1;
}

bool
bitset_condition_tester::eq(const sint32_type *v) const
{
  return v[0] & 0x4;
}

bool
bitset_condition_tester::mi(const sint32_type *v) const
{
  return v[0] & 0x8;
}

bool
bitset_condition_tester::lt(const sint32_type *v) const
{
  return mi(v) != bool(v[0] & 0x2);
}
