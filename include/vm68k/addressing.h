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
      int getb(const context &ec) const
	{return extsb(ec.regs.d[reg]);}
      int getw(const context &ec) const
	{return extsw(ec.regs.d[reg]);}
      int32 getl(const context &ec) const
	{return extsl(ec.regs.d[reg]);}
      void putb(context &ec, uint_type value) const
	{modify_b(ec.regs.d[reg], value);}
      void putw(context &ec, uint_type value) const
	{modify_w(ec.regs.d[reg], value);}
      void putl(context &ec, uint32_type value) const
	{modify_l(ec.regs.d[reg], value);}
      void finishb(context &) const {}
      void finishw(context &) const {}
      void finishl(context &) const {}
      const char *textb(const context &ec) const
	{return textw(ec);}
      const char *textw(const context &) const
	{
	  static char buf[8];
	  sprintf(buf, "%%d%d", reg);
	  return buf;
	}
      const char *textl(const context &ec) const
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
      int getw(const context &ec) const
	{return extsw(ec.regs.a[reg]);}
      int32 getl(const context &ec) const
	{return extsl(ec.regs.a[reg]);}
      void putw(context &ec, int value) const
	{ec.regs.a[reg] = extsw(value);}
      void putl(context &ec, int32 value) const
	{ec.regs.a[reg] = value;}
      void finishw(context &) const {}
      void finishl(context &) const {}
      const char *textb(const context &ec) const
	{return textw(ec);}
      const char *textw(const context &) const
	{
	  static char buf[8];
	  sprintf(buf, "%%a%d", reg);
	  return buf;
	}
      const char *textl(const context &ec) const
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
      uint32 address(const context &ec) const
	{return ec.regs.a[reg];}
      int getb(const context &ec) const
	{return extsb(ec.mem->getb(ec.data_fc(), address(ec)));}
      int getw(const context &ec) const
	{return extsw(ec.mem->getw(ec.data_fc(), address(ec)));}
      int32 getl(const context &ec) const
	{return extsl(ec.mem->getl(ec.data_fc(), address(ec)));}
      void putb(context &ec, int value) const
	{ec.mem->putb(ec.data_fc(), address(ec), value);}
      void putw(context &ec, int value) const
	{ec.mem->putw(ec.data_fc(), address(ec), value);}
      void putl(context &ec, int32 value) const
	{ec.mem->putl(ec.data_fc(), address(ec), value);}
      void finishb(context &) const {}
      void finishw(context &) const {}
      void finishl(context &) const {}
      const char *textb(const context &ec) const
	{return textw(ec);}
      const char *textw(const context &) const
	{
	  static char buf[8];
	  sprintf(buf, "%%a%d@", reg);
	  return buf;
	}
      const char *textl(const context &ec) const
	{return textw(ec);}
    };

    class postinc_indirect
    {
    private:
      int reg;
    public:
      postinc_indirect(int r, int)
	: reg(r) {}
    public:
      size_t isize(size_t) const
	{return 0;}
      // XXX: address is unimplemented.
      int getb(const context &ec) const
	{return extsb(ec.mem->getb(ec.data_fc(), ec.regs.a[reg]));}
      int getw(const context &ec) const
	{return extsw(ec.mem->getw(ec.data_fc(), ec.regs.a[reg]));}
      int32 getl(const context &ec) const
	{return extsl(ec.mem->getl(ec.data_fc(), ec.regs.a[reg]));}
      void putb(context &ec, int value) const
	{ec.mem->putb(ec.data_fc(), ec.regs.a[reg], value);}
      void putw(context &ec, int value) const
	{ec.mem->putw(ec.data_fc(), ec.regs.a[reg], value);}
      void putl(context &ec, int32 value) const
	{ec.mem->putl(ec.data_fc(), ec.regs.a[reg], value);}
      void finishb(context &ec) const
	{ec.regs.a[reg] += reg == 7 ? 2 : 1;} // XXX: %a7 is special.
      void finishw(context &ec) const
	{ec.regs.a[reg] += 2;}
      void finishl(context &ec) const
	{ec.regs.a[reg] += 4;}
      const char *textb(const context &ec) const
	{return textw(ec);}
      const char *textw(const context &) const
	{
	  static char buf[8];
	  sprintf(buf, "%%a%d@+", reg);
	  return buf;
	}
      const char *textl(const context &ec) const
	{return textw(ec);}
    };

    class predec_indirect
    {
    private:
      int reg;
    public:
      predec_indirect(int r, int)
	: reg(r) {}
    public:
      size_t isize(size_t) const
	{return 0;}
      // XXX: address is unimplemented.
      int getb(const context &ec) const
	{return extsb(ec.mem->getb(ec.data_fc(),
				   ec.regs.a[reg] - (reg == 7 ? 2 : 1)));}
				// XXX: %a7 is special.
      int getw(const context &ec) const
	{return extsw(ec.mem->getw(ec.data_fc(), ec.regs.a[reg] - 2));}
      int32 getl(const context &ec) const
	{return extsl(ec.mem->getl(ec.data_fc(), ec.regs.a[reg] - 4));}
      void putb(context &ec, int value) const
	{ec.mem->putb(ec.data_fc(),
		      ec.regs.a[reg] - (reg == 7 ? 2 : 1), value);}
				// XXX: %a7 is special.
      void putw(context &ec, int value) const
	{ec.mem->putw(ec.data_fc(), ec.regs.a[reg] - 2, value);}
      void putl(context &ec, int32 value) const
	{ec.mem->putl(ec.data_fc(), ec.regs.a[reg] - 4, value);}
      void finishb(context &ec) const
	{ec.regs.a[reg] -= reg == 7 ? 2 : 1;} // XXX: %a7 is special.
      void finishw(context &ec) const
	{ec.regs.a[reg] -= 2;}
      void finishl(context &ec) const
	{ec.regs.a[reg] -= 4;}
      const char *textb(const context &ec) const
	{return textw(ec);}
      const char *textw(const context &) const
	{
	  static char buf[8];
	  sprintf(buf, "%%a%d@-", reg);
	  return buf;
	}
      const char *textl(const context &ec) const
	{return textw(ec);}
    };

    typedef postinc_indirect postincrement_indirect;
    typedef predec_indirect predecrement_indirect;

    class disp_indirect
    {
    private:
      unsigned int reg;
      int offset;
    public:
      disp_indirect(unsigned int r, int off)
	: reg(r), offset(off) {}
    public:
      size_t isize(size_t) const
	{return 2;}
      uint32 address(const context &ec) const
	{return ec.regs.a[reg] + extsw(ec.fetchw(offset));}
      int getb(const context &ec) const
	{return extsb(ec.mem->getb(ec.data_fc(), address(ec)));}
      int getw(const context &ec) const
	{return extsw(ec.mem->getw(ec.data_fc(), address(ec)));}
      int32 getl(const context &ec) const
	{return extsl(ec.mem->getl(ec.data_fc(), address(ec)));}
      void putb(context &ec, int value) const
	{ec.mem->putb(ec.data_fc(), address(ec), value);}
      void putw(context &ec, int value) const
	{ec.mem->putw(ec.data_fc(), address(ec), value);}
      void putl(context &ec, int32 value) const
	{ec.mem->putl(ec.data_fc(), address(ec), value);}
      void finishb(context &) const {}
      void finishw(context &) const {}
      void finishl(context &) const {}
      const char *textb(const context &ec) const
	{return textw(ec);}
      const char *textw(const context &ec) const
	{
	  static char buf[32];
	  sprintf(buf, "%%a%d@(%d)", reg, extsw(ec.fetchw(offset)));
	  return buf;
	}
      const char *textl(const context &ec) const
	{return textw(ec);}
    };

    class indexed_indirect
    {
    private:
      unsigned int reg;
      int offset;
    public:
      indexed_indirect(unsigned int r, int off)
	: reg(r), offset(off) {}
    public:
      size_t isize(size_t) const
	{return 2;}
      uint32_type address(const context &ec) const
	{
	  uint_type w = ec.fetchw(offset);
	  unsigned int r = w >> 12 & 0xf;
	  uint32_type x = r >= 8 ? ec.regs.a[r - 8] : ec.regs.d[r];
	  return ec.regs.a[reg] + extsb(w) + (w & 0x800 ? extsl(x) : extsw(x));
	}
      sint_type getb(const context &ec) const
	{return extsb(ec.mem->getb(ec.data_fc(), address(ec)));}
      sint_type getw(const context &ec) const
	{return extsw(ec.mem->getw(ec.data_fc(), address(ec)));}
      sint32_type getl(const context &ec) const
	{return extsl(ec.mem->getl(ec.data_fc(), address(ec)));}
      void putb(context &ec, int value) const
	{ec.mem->putb(ec.data_fc(), address(ec), value);}
      void putw(context &ec, int value) const
	{ec.mem->putw(ec.data_fc(), address(ec), value);}
      void putl(context &ec, int32 value) const
	{ec.mem->putl(ec.data_fc(), address(ec), value);}
      void finishb(context &) const {}
      void finishw(context &) const {}
      void finishl(context &) const {}
      const char *textb(const context &ec) const
	{return textw(ec);}
      const char *textw(const context &ec) const
	{
	  uint_type w = ec.fetchw(offset);
	  unsigned int r = w >> 12 & 0xf;
	  static char buf[32];
	  if (r >= 8)
	    sprintf(buf, "%%a%u@(%d,%%a%u%s)", reg, extsb(w), r - 8,
		    w & 0x800 ? ":l" : ":w");
	  else
	    sprintf(buf, "%%a%u@(%d,%%d%u%s)", reg, extsb(w), r,
		    w & 0x800 ? ":l" : ":w");
	  return buf;
	}
      const char *textl(const context &ec) const
	{return textw(ec);}
    };

    class absolute_short
    {
    private:
      int offset;
    public:
      absolute_short(unsigned int r, int off)
	: offset(off) {}
    public:
      size_t isize(size_t) const
	{return 2;}
      uint32 address(const context &ec) const
	{return ec.fetchw(offset);}
      int getb(const context &ec) const
	{return extsb(ec.mem->getb(ec.data_fc(), address(ec)));}
      int getw(const context &ec) const
	{return extsw(ec.mem->getw(ec.data_fc(), address(ec)));}
      int32 getl(const context &ec) const
	{return extsl(ec.mem->getl(ec.data_fc(), address(ec)));}
      void putb(context &ec, int value) const
	{ec.mem->putb(ec.data_fc(), address(ec), value);}
      void putw(context &ec, int value) const
	{ec.mem->putw(ec.data_fc(), address(ec), value);}
      void putl(context &ec, int32 value) const
	{ec.mem->putl(ec.data_fc(), address(ec), value);}
      void finishb(context &) const {}
      void finishw(context &) const {}
      void finishl(context &) const {}
      const char *textb(const context &ec) const
	{return textw(ec);}
      const char *textw(const context &ec) const
	{
	  static char buf[32];
	  sprintf(buf, "0x%lx:s", (unsigned long) address(ec));
	  return buf;
	}
      const char *textl(const context &ec) const
	{return textw(ec);}
    };

    class absolute_long
    {
    private:
      int offset;
    public:
      absolute_long(unsigned int r, int off)
	: offset(off) {}
    public:
      size_t isize(size_t) const
	{return 4;}
      uint32 address(const context &ec) const
	{return ec.fetchl(offset);}
      int getb(const context &ec) const
	{return extsb(ec.mem->getb(ec.data_fc(), address(ec)));}
      int getw(const context &ec) const
	{return extsw(ec.mem->getw(ec.data_fc(), address(ec)));}
      int32 getl(const context &ec) const
	{return extsl(ec.mem->getl(ec.data_fc(), address(ec)));}
      void putb(context &ec, int value) const
	{ec.mem->putb(ec.data_fc(), address(ec), value);}
      void putw(context &ec, int value) const
	{ec.mem->putw(ec.data_fc(), address(ec), value);}
      void putl(context &ec, int32 value) const
	{ec.mem->putl(ec.data_fc(), address(ec), value);}
      void finishb(context &) const {}
      void finishw(context &) const {}
      void finishl(context &) const {}
      const char *textb(const context &ec) const
	{return textw(ec);}
      const char *textw(const context &ec) const
	{
	  static char buf[32];
	  sprintf(buf, "0x%lx", (unsigned long) address(ec));
	  return buf;
	}
      const char *textl(const context &ec) const
	{return textw(ec);}
    };

    class disp_pc
    {
    private:
      int offset;
    public:
      disp_pc(int r, int off)
	: offset(off) {}
    public:
      size_t isize(size_t size) const
	{return 2;}
      uint32 address(const context &ec) const
	{return ec.regs.pc + offset + extsw(ec.fetchw(offset));}
      int getb(const context &ec) const
	{return extsb(ec.mem->getb(ec.data_fc(), address(ec)));}
      int getw(const context &ec) const
	{return extsw(ec.mem->getw(ec.data_fc(), address(ec)));}
      int32 getl(const context &ec) const
	{return extsl(ec.mem->getl(ec.data_fc(), address(ec)));}
      // XXX: putb, putw, and putl are unimplemented.
      void finishb(context &ec) const {}
      void finishw(context &ec) const {}
      void finishl(context &ec) const {}
      const char *textb(const context &ec) const
	{return textw(ec);}
      const char *textw(const context &ec) const
	{
	  static char buf[16];
	  sprintf(buf, "%%pc@(%d)", extsw(ec.fetchw(offset)));
	  return buf;
	}
      const char *textl(const context &ec) const
	{return textw(ec);}
    };

    class indexed_pc_indirect
    {
    private:
      int offset;
    public:
      indexed_pc_indirect(unsigned int r, int off)
	: offset(off) {}
    public:
      size_t isize(size_t) const
	{return 2;}		// Size of an extention word.
      uint32_type address(const context &ec) const
	{
	  uint_type w = ec.fetchw(offset);
	  unsigned int r = w >> 12 & 0xf;
	  uint32_type x = r >= 8 ? ec.regs.a[r - 8] : ec.regs.d[r];
	  return ec.regs.pc + offset + extsb(w)
	    + (w & 0x800 ? extsl(x) : extsw(x));
	}
      sint_type getb(const context &ec) const
	{return extsb(ec.mem->getb(ec.data_fc(), address(ec)));}
      sint_type getw(const context &ec) const
	{return extsw(ec.mem->getw(ec.data_fc(), address(ec)));}
      sint32_type getl(const context &ec) const
	{return extsl(ec.mem->getl(ec.data_fc(), address(ec)));}
      void putb(context &ec, int value) const
	{ec.mem->putb(ec.data_fc(), address(ec), value);}
      void putw(context &ec, int value) const
	{ec.mem->putw(ec.data_fc(), address(ec), value);}
      void putl(context &ec, int32 value) const
	{ec.mem->putl(ec.data_fc(), address(ec), value);}
      void finishb(context &) const {}
      void finishw(context &) const {}
      void finishl(context &) const {}
      const char *textb(const context &ec) const
	{return textw(ec);}
      const char *textw(const context &ec) const
	{
	  uint_type w = ec.fetchw(offset);
	  unsigned int r = w >> 12 & 0xf;
	  static char buf[32];
	  if (r >= 8)
	    sprintf(buf, "%%pc@(%d,%%a%u%s)", extsb(w), r - 8,
		    w & 0x800 ? ":l" : ":w");
	  else
	    sprintf(buf, "%%pc@(%d,%%d%u%s)", extsb(w), r,
		    w & 0x800 ? ":l" : ":w");
	  return buf;
	}
      const char *textl(const context &ec) const
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
      int getb(const context &ec) const
	{return extsb(ec.fetchw(offset));}
      int getw(const context &ec) const
	{return extsw(ec.fetchw(offset));}
      int32 getl(const context &ec) const
	{return extsl(ec.fetchl(offset));}
      // XXX: putb, putw, and putl are unimplemented.
      void finishb(context &) const {}
      void finishw(context &) const {}
      void finishl(context &) const {}
      const char *textb(const context &ec) const
	{
	  static char buf[32];
	  sprintf(buf, "#0x%x", (unsigned int) getb(ec));
	  return buf;
	}
      const char *textw(const context &ec) const
	{
	  static char buf[32];
	  sprintf(buf, "#0x%x", (unsigned int) getw(ec));
	  return buf;
	}
      const char *textl(const context &ec) const
	{
	  static char buf[32];
	  sprintf(buf, "#0x%lx", (unsigned long) getl(ec));
	  return buf;
	}
    };
  } // addressing
} // vm68k

#endif /* not _VM68K_ADDRESSING_H */

