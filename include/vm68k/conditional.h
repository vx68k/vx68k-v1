/* -*-C++-*- */
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

#ifndef _VM68K_CONDITIONAL_H
#define _VM68K_CONDITIONAL_H 1

#include <vm68k/processor.h>

#include <functional>

namespace vm68k
{
  using namespace std;

  namespace conditional
  {
    /* T condition.  */
    struct t: unary_function<context, bool>
    {
      bool operator()(const context &c) const
      {return true;}

      static const char *text()
      {return "t";}
    };

    /* F condition.  */
    struct f: unary_function<context, bool>
    {
      bool operator()(const context &c) const
      {return false;}

      static const char *text()
      {return "f";}
    };

    /* HI condition.  */
    struct hi: unary_function<context, bool>
    {
      bool operator()(const context &c) const
      {return c.regs.ccr.hi();}

      static const char *text()
      {return "hi";}
    };

    /* LS condition.  */
    struct ls: unary_function<context, bool>
    {
      bool operator()(const context &c) const
      {return c.regs.ccr.ls();}

      static const char *text()
      {return "ls";}
    };

    /* CC (HS) condition.  */
    struct cc: unary_function<context, bool>
    {
      bool operator()(const context &c) const
      {return c.regs.ccr.cc();}

      static const char *text()
      {return "cc";}
    };

    /* CS (LO) condition.  */
    struct cs: unary_function<context, bool>
    {
      bool operator()(const context &c) const
      {return c.regs.ccr.cs();}

      static const char *text()
      {return "cs";}
    };

    /* NE condition.  */
    struct ne: unary_function<context, bool>
    {
      bool operator()(const context &c) const
      {return c.regs.ccr.ne();}

      static const char *text()
      {return "ne";}
    };

    /* EQ condition.  */
    struct eq: unary_function<context, bool>
    {
      bool operator()(const context &c) const
      {return c.regs.ccr.eq();}

      static const char *text()
      {return "eq";}
    };

    /* FIXME: VC and VS conditions missing.  */

    /* PL condition.  */
    struct pl: unary_function<context, bool>
    {
      bool operator()(const context &c) const
      {return c.regs.ccr.pl();}

      static const char *text()
      {return "pl";}
    };

    /* MI condition.  */
    struct mi: unary_function<context, bool>
    {
      bool operator()(const context &c) const
      {return c.regs.ccr.mi();}

      static const char *text()
      {return "mi";}
    };

    /* GE condition.  */
    struct ge: unary_function<context, bool>
    {
      bool operator()(const context &c) const
      {return c.regs.ccr.ge();}

      static const char *text()
      {return "ge";}
    };

    /* LT condition.  */
    struct lt: unary_function<context, bool>
    {
      bool operator()(const context &c) const
      {return c.regs.ccr.lt();}

      static const char *text()
      {return "lt";}
    };

    /* GT condition.  */
    struct gt: unary_function<context, bool>
    {
      bool operator()(const context &c) const
      {return c.regs.ccr.gt();}

      static const char *text()
      {return "gt";}
    };

    /* LE condition.  */
    struct le: unary_function<context, bool>
    {
      bool operator()(const context &c) const
      {return c.regs.ccr.le();}

      static const char *text()
      {return "le";}
    };
  }
}

#endif /* not _VM68K_CONDITIONAL_H */
