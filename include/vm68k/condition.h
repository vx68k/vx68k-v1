/* -*-C++-*- */
/* vx68k - Virtual X68000
   Copyright (C) 1998, 1999 Hypercore Software Design, Ltd.

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

#ifndef _VM68K_CONDITION_H
#define _VM68K_CONDITION_H 1

#include <vm68k/cpu.h>
#include <functional>

namespace vm68k
{
  using namespace std;

  namespace condition
  {
    struct hi
      : unary_function<context, bool>
    {
      bool operator()(const context &ec) const
	{return ec.regs.sr.hi();}
      const char *text() const
	{return "hi";}
    };

    struct ls
      : unary_function<context, bool>
    {
      bool operator()(const context &ec) const
	{return ec.regs.sr.ls();}
      const char *text() const
	{return "ls";}
    };

    struct cc
      : unary_function<context, bool>
    {
      bool operator()(const context &ec) const
	{return ec.regs.sr.cc();}
      const char *text() const
	{return "cc";}
    };

    struct cs
      : unary_function<context, bool>
    {
      bool operator()(const context &ec) const
	{return ec.regs.sr.cs();}
      const char *text() const
	{return "cs";}
    };

    struct pl
      : unary_function<context, bool>
    {
      bool operator()(const context &ec) const
	{return ec.regs.sr.pl();}
      const char *text() const
	{return "pl";}
    };

    struct mi
      : unary_function<context, bool>
    {
      bool operator()(const context &ec) const
	{return ec.regs.sr.mi();}
      const char *text() const
	{return "mi";}
    };

    struct ge
      : unary_function<context, bool>
    {
      bool operator()(const context &ec) const
	{return ec.regs.sr.ge();}
      const char *text() const
	{return "ge";}
    };

    struct lt
      : unary_function<context, bool>
    {
      bool operator()(const context &ec) const
	{return ec.regs.sr.lt();}
      const char *text() const
	{return "lt";}
    };

    struct gt
      : unary_function<context, bool>
    {
      bool operator()(const context &ec) const
	{return ec.regs.sr.gt();}
      const char *text() const
	{return "gt";}
    };

    struct le
      : unary_function<context, bool>
    {
      bool operator()(const context &ec) const
	{return ec.regs.sr.le();}
      const char *text() const
	{return "le";}
    };
  } // condition
} // vm68k

#endif /* not _VM68K_CONDITION_H */

