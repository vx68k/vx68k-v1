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
#include <cstdio>

namespace vm68k
{
  namespace addressing
  {
    using std::sprintf;

    class data_register
    {
    private:
      int reg;
    public:
      data_register(int r, int)
	: reg(r) {}
    public:
      size_t isize(size_t) const
	{return 0;}
      // XXX: address is unimplemented.
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
      const char *textb(const execution_context *ec) const
	{return textw(ec);}
      const char *textw(const execution_context *) const
	{
	  static char buf[8];
	  sprintf(buf, "%%d%d", reg);
	  return buf;
	}
      const char *textl(const execution_context *ec) const
	{return textw(ec);}
    };

    class address_register
    {
    private:
      int reg;
    public:
      address_register(int r, int)
	: reg(r) {}
    public:
      size_t isize(size_t) const
	{return 0;}
      // XXX: address is unimplemented.
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
      const char *textb(const execution_context *ec) const
	{return textw(ec);}
      const char *textw(const execution_context *) const
	{
	  static char buf[8];
	  sprintf(buf, "%%a%d", reg);
	  return buf;
	}
      const char *textl(const execution_context *ec) const
	{return textw(ec);}
    };

    class indirect
    {
    private:
      int reg;
    public:
      indirect(int r, int)
	: reg(r) {}
    public:
      size_t isize(size_t) const
	{return 0;}
      uint32 address(const execution_context *ec) const
	{return ec->regs.a[reg];}
      int getb(const execution_context *ec) const
	{return extsb(ec->mem->getb(ec->data_fc(), address(ec)));}
      int getw(const execution_context *ec) const
	{return extsw(ec->mem->getw(ec->data_fc(), address(ec)));}
      int32 getl(const execution_context *ec) const
	{return extsl(ec->mem->getl(ec->data_fc(), address(ec)));}
      void putb(execution_context *ec, int value) const
	{ec->mem->putb(ec->data_fc(), address(ec), value);}
      void putw(execution_context *ec, int value) const
	{ec->mem->putw(ec->data_fc(), address(ec), value);}
      void putl(execution_context *ec, int32 value) const
	{ec->mem->putl(ec->data_fc(), address(ec), value);}
      void finishb(execution_context *) const {}
      void finishw(execution_context *) const {}
      void finishl(execution_context *) const {}
      const char *textb(const execution_context *ec) const
	{return textw(ec);}
      const char *textw(const execution_context *) const
	{
	  static char buf[8];
	  sprintf(buf, "%%a%d@", reg);
	  return buf;
	}
      const char *textl(const execution_context *ec) const
	{return textw(ec);}
    };

    class postincrement_indirect
    {
    private:
      int reg;
    public:
      postincrement_indirect(int r, int)
	: reg(r) {}
    public:
      size_t isize(size_t) const
	{return 0;}
      // XXX: address is unimplemented.
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
      void finishb(execution_context *ec) const
	{ec->regs.a[reg] += 1;}	// FIXME: %a7 is special.
      void finishw(execution_context *ec) const
	{ec->regs.a[reg] += 2;}
      void finishl(execution_context *ec) const
	{ec->regs.a[reg] += 4;}
      const char *textb(const execution_context *ec) const
	{return textw(ec);}
      const char *textw(const execution_context *) const
	{
	  static char buf[8];
	  sprintf(buf, "%%a%d@+", reg);
	  return buf;
	}
      const char *textl(const execution_context *ec) const
	{return textw(ec);}
    };

    class predecrement_indirect
    {
    private:
      int reg;
    public:
      predecrement_indirect(int r, int)
	: reg(r) {}
    public:
      size_t isize(size_t) const
	{return 0;}
      // XXX: address is unimplemented.
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
      const char *textb(const execution_context *ec) const
	{return textw(ec);}
      const char *textw(const execution_context *) const
	{
	  static char buf[8];
	  sprintf(buf, "%%a%d@-", reg);
	  return buf;
	}
      const char *textl(const execution_context *ec) const
	{return textw(ec);}
    };

    class absolute_long
    {
    protected:
      int offset;
    public:
      absolute_long(int, int off)
	: offset(off) {}
    public:
      size_t isize(size_t) const
	{return 4;}
      uint32 address(const execution_context *ec) const
	{return ec->fetchl(offset);}
      int getb(const execution_context *ec) const
	{return extsb(ec->mem->getb(ec->data_fc(), address(ec)));}
      int getw(const execution_context *ec) const
	{return extsw(ec->mem->getw(ec->data_fc(), address(ec)));}
      int32 getl(const execution_context *ec) const
	{return extsl(ec->mem->getl(ec->data_fc(), address(ec)));}
      void putb(execution_context *ec, int value) const
	{ec->mem->putb(ec->data_fc(), address(ec), value);}
      void putw(execution_context *ec, int value) const
	{ec->mem->putw(ec->data_fc(), address(ec), value);}
      void putl(execution_context *ec, int32 value) const
	{ec->mem->putl(ec->data_fc(), address(ec), value);}
      void finishb(execution_context *) const {}
      void finishw(execution_context *) const {}
      void finishl(execution_context *) const {}
      const char *textb(const execution_context *ec) const
	{return textw(ec);}
      const char *textw(const execution_context *ec) const
	{
	  static char buf[32];
	  sprintf(buf, "0x%lx", (unsigned long) address(ec));
	  return buf;
	}
      const char *textl(const execution_context *ec) const
	{return textw(ec);}
    };

    class immediate
    {
    protected:
      int offset;
    public:
      immediate(int, int off)
	: offset(off) {}
    public:
      size_t isize(size_t size) const
	{return size;}
      // XXX: address in unimplemented.
      int getb(const execution_context *ec) const
	{return extsb(ec->fetchw(offset));}
      int getw(const execution_context *ec) const
	{return extsw(ec->fetchw(offset));}
      int32 getl(const execution_context *ec) const
	{return extsl(ec->fetchl(offset));}
      // XXX: putb, putw, and putl are unimplemented.
      void finishb(execution_context *) const {}
      void finishw(execution_context *) const {}
      void finishl(execution_context *) const {}
      const char *textb(const execution_context *ec) const
	{
	  static char buf[32];
	  sprintf(buf, "#0x%x", (unsigned int) getb(ec));
	  return buf;
	}
      const char *textw(const execution_context *ec) const
	{
	  static char buf[32];
	  sprintf(buf, "#0x%x", (unsigned int) getw(ec));
	  return buf;
	}
      const char *textl(const execution_context *ec) const
	{
	  static char buf[32];
	  sprintf(buf, "#0x%lx", (unsigned long) getl(ec));
	  return buf;
	}
    };
  } // addressing
} // vm68k

#endif /* not _VM68K_ADDRESSING_H */
