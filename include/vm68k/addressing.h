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

#ifndef _VM68K_ADDRESSING_H
#define _VM68K_ADDRESSING_H 1

#include <vm68k/cpu.h>

namespace vm68k
{
  namespace addressing
  {
    class data_register
    {
    private:
      int reg;
    public:
      data_register(int r, int)
	: reg(r) {}
    public:
      int getb(const execution_context *ec) const
	{return extsl(ec->regs.d[reg]);}
      int getw(const execution_context *ec) const
	{return extsw(ec->regs.d[reg]);}
      int32 getl(const execution_context *ec) const
	{return extsl(ec->regs.d[reg]);}
      void putb(execution_context *ec, int value) const
	{
	  const uint32 MASK = ((uint32) 1u << 8) - 1;
	  ec->regs.d[reg] = ec->regs.d[reg] & ~MASK | (uint32) value & MASK;
	}
      void putw(execution_context *ec, int value) const
	{
	  const uint32 MASK = ((uint32) 1u << 16) - 1;
	  ec->regs.d[reg] = ec->regs.d[reg] & ~MASK | (uint32) value & MASK;
	}
      void putl(execution_context *ec, int32 value) const
	{ec->regs.d[reg] = value;}
      void finishb(execution_context *) const {}
      void finishw(execution_context *) const {}
      void finishl(execution_context *) const {}
    };

    class address_register
    {
    private:
      int reg;
    public:
      address_register(int r, int)
	: reg(r) {}
    public:
      // XXX: getb, putb, and finishb are not available.
      int getw(const execution_context *ec) const
	{return extsw(ec->regs.a[reg]);}
      int32 getl(const execution_context *ec) const
	{return extsl(ec->regs.a[reg]);}
      void putw(execution_context *ec, int value) const
	{ec->regs.a[reg] = extsw(value);}
      void putl(execution_context *ec, int32 value) const
	{ec->regs.a[reg] = value;}
      void finishw(execution_context *) const {}
      void finishl(execution_context *) const {}
    };

    class indirect
    {
    protected:
      int reg;
    public:
      indirect(int r, int)
	: reg(r) {}
    public:
      int getb(const execution_context *ec) const
	{return extsb(ec->mem->getb(ec->data_fc(), ec->regs.a[reg]));}
      int getw(const execution_context *ec) const
	{return extsw(ec->mem->getw(ec->data_fc(), ec->regs.a[reg]));}
      int32 getl(const execution_context *ec) const
	{return extsl(ec->mem->getl(ec->data_fc(), ec->regs.a[reg]));}
      void putb(execution_context *ec, int value) const
	{ec->mem->putb(ec->data_fc(), ec->regs.a[reg], value);}
      void putw(execution_context *ec, int value) const
	{ec->mem->putw(ec->data_fc(), ec->regs.a[reg], value);}
      void putl(execution_context *ec, int32 value) const
	{ec->mem->putl(ec->data_fc(), ec->regs.a[reg], value);}
      void finishb(execution_context *) const {}
      void finishw(execution_context *) const {}
      void finishl(execution_context *) const {}
    };

    class postincrement_indirect
      : public indirect
    {
    public:
      postincrement_indirect(int r, int off)
	: indirect(r, off) {}
    public:
      void finishb(execution_context *ec) const
	{ec->regs.a[reg] += 1;}	// FIXME: %a7 is special.
      void finishw(execution_context *ec) const
	{ec->regs.a[reg] += 2;}
      void finishl(execution_context *ec) const
	{ec->regs.a[reg] += 4;}
    };

    class predecrement_indirect
      : public indirect
    {
    public:
      predecrement_indirect(int r, int off)
	: indirect(r, off) {}
    public:
      int getb(const execution_context *ec) const
	{return extsb(ec->mem->getb(ec->data_fc(), ec->regs.a[reg] - 1));}
				// FIXME: %a7 is special.
      int getw(const execution_context *ec) const
	{return extsw(ec->mem->getw(ec->data_fc(), ec->regs.a[reg] - 2));}
      int32 getl(const execution_context *ec) const
	{return extsl(ec->mem->getl(ec->data_fc(), ec->regs.a[reg] - 4));}
      void putb(execution_context *ec, int value) const
	{ec->mem->putb(ec->data_fc(), ec->regs.a[reg] - 1, value);}
				// FIXME: %a7 is special.
      void putw(execution_context *ec, int value) const
	{ec->mem->putw(ec->data_fc(), ec->regs.a[reg] - 2, value);}
      void putl(execution_context *ec, int32 value) const
	{ec->mem->putl(ec->data_fc(), ec->regs.a[reg] - 4, value);}
      void finishb(execution_context *ec) const
	{ec->regs.a[reg] -= 1;} // FIXME: %a7 is special.
      void finishw(execution_context *ec) const
	{ec->regs.a[reg] -= 2;}
      void finishl(execution_context *ec) const
	{ec->regs.a[reg] -= 4;}
    };
  } // addressing
} // vm68k

#endif /* not _VM68K_ADDRESSING_H */

