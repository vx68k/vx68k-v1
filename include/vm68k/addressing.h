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

    template <class Size> class basic_d_register
    {
    public:
      typedef Size size_type;
      typedef typename Size::uvalue_type uvalue_type;
      typedef typename Size::svalue_type svalue_type;

    public:
      static size_t extension_size() {return 0;}

    private:
      unsigned int reg;

    public:
      basic_d_register(unsigned int r, size_t off): reg(r) {}

    public:
      // XXX address is left unimplemented.

      typename Size::svalue_type get(const context &c) const;

      void put(context &c, svalue_type value) const
	{Size::put(c.regs.d[reg], value);}
      void finish(context &c) const {}

    public:
      const char *text(const context &c) const
	{
	  static char buf[8];
	  sprintf(buf, "%%d%u", reg);
	  return buf;
	}
    };

    template <class Size> inline typename Size::svalue_type
    basic_d_register<Size>::get(const context &c) const
    {
      return Size::get(c.regs.d[reg]);
    }

    typedef basic_d_register<byte_size> byte_d_register;
    typedef basic_d_register<word_size> word_d_register;
    typedef basic_d_register<long_word_size> long_word_d_register;

    template <class Size> class basic_a_register
    {
    public:
      typedef Size size_type;
      typedef typename Size::uvalue_type uvalue_type;
      typedef typename Size::svalue_type svalue_type;

    public:
      static size_t extension_size() {return 0;}

    private:
      unsigned int reg;

    public:
      basic_a_register(unsigned int r, size_t off): reg(r) {}

    public:
      // XXX address is left unimplemented.

      typename Size::svalue_type get(const context &c) const;

      void put(context &c, svalue_type value) const
	{long_word_size::put(c.regs.a[reg], value);}
      void finish(context &c) const {}

    public:
      const char *text(const context &c) const
	{
	  static char buf[8];
	  sprintf(buf, "%%a%u", reg);
	  return buf;
	}
    };

    template <class Size> inline typename Size::svalue_type
    basic_a_register<Size>::get(const context &c) const
    {
      return Size::get(c.regs.a[reg]);
    }

    // XXX Address register direct is not allowed for byte size.
    typedef basic_a_register<word_size> word_a_register;
    typedef basic_a_register<long_word_size> long_word_a_register;

    template <class Size> class basic_indirect
    {
    public:
      typedef Size size_type;
      typedef typename Size::uvalue_type uvalue_type;
      typedef typename Size::svalue_type svalue_type;

    public:
      static size_t extension_size() {return 0;}

    private:
      unsigned int reg;

    public:
      basic_indirect(unsigned int r, size_t off): reg(r) {}

    public:
      uint32_type address(const context &c) const {return c.regs.a[reg];}

      typename Size::svalue_type get(const context &c) const;

      void put(context &c, svalue_type value) const
	{Size::put(*c.mem, c.data_fc(), address(c), value);}
      void finish(context &c) const {}

    public:
      const char *text(const context &c) const;
    };

    template <class Size> inline typename Size::svalue_type
    basic_indirect<Size>::get(const context &c) const
    {
      return Size::get(*c.mem, c.data_fc(), address(c));
    }

    template <class Size> const char *
    basic_indirect<Size>::text(const context &c) const
    {
      static char buf[64];
      sprintf(buf, "%%a%u@/* %#lx */", reg, address(c) + 0UL);
      return buf;
    }

    typedef basic_indirect<byte_size> byte_indirect;
    typedef basic_indirect<word_size> word_indirect;
    typedef basic_indirect<long_word_size> long_word_indirect;

    template <class Size> class basic_postinc_indirect
    {
    public:
      typedef Size size_type;
      typedef typename Size::uvalue_type uvalue_type;
      typedef typename Size::svalue_type svalue_type;

    public:
      static size_t extension_size() {return 0;}

    private:
      unsigned int reg;

    public:
      basic_postinc_indirect(unsigned int r, size_t off)
	: reg(r)
      {}

    protected:
      size_t advance_size() const
      {
	if (reg == 7)		// XXX: %a7 is special.
	  return Size::aligned_value_size();
	else
	  return Size::value_size();
      }

    public:
      // XXX: address is left unimplemented.

      typename Size::svalue_type get(const context &c) const;

      void put(context &c, svalue_type value) const
      {Size::put(*c.mem, c.data_fc(), c.regs.a[reg], value);}

      void finish(context &c) const
      {c.regs.a[reg] += advance_size();}

    public:
      const char *text(const context &c) const;
    };

    template <class Size> inline typename Size::svalue_type
    basic_postinc_indirect<Size>::get(const context &c) const
    {
      return Size::get(*c.mem, c.data_fc(), c.regs.a[reg]);
    }

    template <class Size> const char *
    basic_postinc_indirect<Size>::text(const context &c) const
    {
      static char buf[64];
      sprintf(buf, "%%a%u@+/* %#lx */", reg, c.regs.a[reg] + 0UL);
      return buf;
    }

    typedef basic_postinc_indirect<byte_size> byte_postinc_indirect;
    typedef basic_postinc_indirect<word_size> word_postinc_indirect;
    typedef basic_postinc_indirect<long_word_size> long_word_postinc_indirect;

    template <class Size> class basic_predec_indirect
    {
    public:
      typedef Size size_type;
      typedef typename Size::uvalue_type uvalue_type;
      typedef typename Size::svalue_type svalue_type;

    public:
      static size_t extension_size() {return 0;}

    private:
      unsigned int reg;

    public:
      basic_predec_indirect(unsigned int r, size_t off): reg(r) {}

    protected:
      size_t advance_size() const
	{
	  if (reg == 7)		// XXX: %a7 is special.
	    return Size::aligned_value_size();
	  else
	    return Size::value_size();
	}

    public:
      // XXX: address is left unimplemented.

      typename Size::svalue_type get(const context &c) const;

      void put(context &c, svalue_type value) const
	{Size::put(*c.mem, c.data_fc(),
		   c.regs.a[reg] - advance_size(), value);}
      void finish(context &c) const
        {c.regs.a[reg] -= advance_size();}

    public:
      const char *text(const context &c) const;
    };

    template <class Size> inline typename Size::svalue_type
    basic_predec_indirect<Size>::get(const context &c) const
    {
      return Size::get(*c.mem, c.data_fc(), c.regs.a[reg] - advance_size());
    }

    template <class Size> const char *
    basic_predec_indirect<Size>::text(const context &c) const
    {
      static char buf[64];
      sprintf(buf, "%%a%u@-/* %#lx */", reg,
	      c.regs.a[reg] - advance_size() + 0UL);
      return buf;
    }

    typedef basic_predec_indirect<byte_size> byte_predec_indirect;
    typedef basic_predec_indirect<word_size> word_predec_indirect;
    typedef basic_predec_indirect<long_word_size> long_word_predec_indirect;

    template <class Size> class basic_disp_indirect
    {
    public:
      typedef Size size_type;
      typedef typename Size::uvalue_type uvalue_type;
      typedef typename Size::svalue_type svalue_type;

    public:
      static size_t extension_size() {return 2;}

    private:
      unsigned int reg;

    private:
      size_t offset;

    public:
      basic_disp_indirect(unsigned int r, size_t off): reg(r), offset(off) {}

    public:
      uint32_type address(const context &c) const;

      typename Size::svalue_type get(const context &c) const;

      void put(context &c, svalue_type value) const
	{Size::put(*c.mem, c.data_fc(), address(c), value);}
      void finish(context &c) const {}

    public:
      const char *text(const context &c) const;
    };

    template <class Size> inline uint32_type
    basic_disp_indirect<Size>::address(const context &c) const
    {
      return c.regs.a[reg] + c.fetch(word_size(), offset);
    }

    template <class Size> inline typename Size::svalue_type
    basic_disp_indirect<Size>::get(const context &c) const
    {
      return Size::get(*c.mem, c.data_fc(), address(c));
    }

    template <class Size> const char *
    basic_disp_indirect<Size>::text(const context &c) const
    {
      static char buf[64];
      sprintf(buf, "%%a%u@(%d)/* %#lx */", reg, c.fetch(word_size(), offset),
	      address(c) + 0UL);
      return buf;
    }

    typedef basic_disp_indirect<byte_size> byte_disp_indirect;
    typedef basic_disp_indirect<word_size> word_disp_indirect;
    typedef basic_disp_indirect<long_word_size> long_word_disp_indirect;

    template <class Size> class basic_index_indirect
    {
    public:
      typedef Size size_type;
      typedef typename Size::uvalue_type uvalue_type;
      typedef typename Size::svalue_type svalue_type;

    public:
      static size_t extension_size() {return 2;}

    private:
      unsigned int reg;

    private:
      size_t offset;

    public:
      basic_index_indirect(unsigned int r, size_t off)
	: reg(r), offset(off)
      {}

    public:
      uint32_type address(const context &c) const;

      typename Size::svalue_type get(const context &c) const;

      void put(context &c, svalue_type value) const
      {Size::put(*c.mem, c.data_fc(), address(c), value);}

      void finish(context &c) const
      {}

    public:
      const char *text(const context &c) const;
    };

    template <class Size> inline uint32_type
    basic_index_indirect<Size>::address(const context &c) const
    {
      uint_type w = c.fetch(word_size(), offset);
      unsigned int r = w >> 12 & 0xf;
      uint32_type x = r >= 8 ? c.regs.a[r - 8] : c.regs.d[r];
      if (w & 0x800)
	return c.regs.a[reg] + byte_size::get(w) + long_word_size::get(x);
      else
	return c.regs.a[reg] + byte_size::get(w) + word_size::get(x);
    }

    template <class Size> inline typename Size::svalue_type
    basic_index_indirect<Size>::get(const context &c) const
    {
      return Size::get(*c.mem, c.data_fc(), address(c));
    }

    template <class Size> const char *
    basic_index_indirect<Size>::text(const context &c) const
    {
      uint_type w = c.fetch(word_size(), offset);
      unsigned int r = w >> 12 & 0xf;
      static char buf[64];
      if (r >= 8)
	sprintf(buf, "%%a%u@(%d,%%a%u%s)/* %#lx */", reg, byte_size::get(w),
		r - 8, w & 0x800 ? ":l" : ":w", address(c) + 0UL);
      else
	sprintf(buf, "%%a%u@(%d,%%d%u%s)/* %#lx */", reg, byte_size::get(w),
		r, w & 0x800 ? ":l" : ":w", address(c) + 0UL);
      return buf;
    }

    typedef basic_index_indirect<byte_size> byte_index_indirect;
    typedef basic_index_indirect<word_size> word_index_indirect;
    typedef basic_index_indirect<long_word_size> long_word_index_indirect;

    template <class Size> class basic_abs_short
    {
    public:
      typedef Size size_type;
      typedef typename Size::uvalue_type uvalue_type;
      typedef typename Size::svalue_type svalue_type;

    public:
      static size_t extension_size() {return 2;}

    private:
      size_t offset;

    public:
      basic_abs_short(unsigned int r, size_t off): offset(off) {}

    public:
      uint32_type address(const context &c) const;

      typename Size::svalue_type get(const context &c) const;

      void put(context &c, svalue_type value) const
	{Size::put(*c.mem, c.data_fc(), address(c), value);}
      void finish(context &c) const {}

    public:
      const char *text(const context &c) const
      {
	static char buf[32];
	sprintf(buf, "%#lx:s", address(c) + 0UL);
	return buf;
      }
    };

    template <class Size> inline uint32_type
    basic_abs_short<Size>::address(const context &c) const
    {
      return c.fetch(word_size(), offset);
    }

    template <class Size> inline typename Size::svalue_type
    basic_abs_short<Size>::get(const context &c) const
    {
      return Size::get(*c.mem, c.data_fc(), address(c));
    }

    typedef basic_abs_short<byte_size> byte_abs_short;
    typedef basic_abs_short<word_size> word_abs_short;
    typedef basic_abs_short<long_word_size> long_word_abs_short;

    template <class Size> class basic_abs_long
    {
    public:
      typedef Size size_type;
      typedef typename Size::uvalue_type uvalue_type;
      typedef typename Size::svalue_type svalue_type;

    public:
      static size_t extension_size() {return 4;}

    private:
      size_t offset;

    public:
      basic_abs_long(unsigned int r, size_t off): offset(off) {}

    public:
      uint32_type address(const context &c) const;

      typename Size::svalue_type get(const context &c) const;

      void put(context &c, svalue_type value) const
	{Size::put(*c.mem, c.data_fc(), address(c), value);}
      void finish(context &c) const {}

    public:
      const char *text(const context &c) const;
    };

    template <class Size> inline uint32_type
    basic_abs_long<Size>::address(const context &c) const
    {
      return c.fetch(long_word_size(), offset);
    }

    template <class Size> inline typename Size::svalue_type
    basic_abs_long<Size>::get(const context &c) const
    {
      return Size::get(*c.mem, c.data_fc(), address(c));
    }

    template <class Size> const char *
    basic_abs_long<Size>::text(const context &c) const
    {
      static char buf[32];
      sprintf(buf, "%#lx", address(c) + 0UL);
      return buf;
    }

    typedef basic_abs_long<byte_size> byte_abs_long;
    typedef basic_abs_long<word_size> word_abs_long;
    typedef basic_abs_long<long_word_size> long_word_abs_long;

    template <class Size> class basic_disp_pc_indirect
    {
    public:
      typedef Size size_type;
      typedef typename Size::uvalue_type uvalue_type;
      typedef typename Size::svalue_type svalue_type;

    public:
      static size_t extension_size() {return 2;}

    private:
      size_t offset;

    public:
      basic_disp_pc_indirect(unsigned int r, size_t off): offset(off) {}

    public:
      uint32_type address(const context &c) const;

      typename Size::svalue_type get(const context &c) const;

      // XXX put is left unimplemented.
      void finish(context &c) const {}

    public:
      const char *text(const context &c) const;
    };

    template <class Size> inline uint32_type
    basic_disp_pc_indirect<Size>::address(const context &c) const
    {
      return c.regs.pc + offset + c.fetch(word_size(), offset);
    }

    template <class Size> inline typename Size::svalue_type
    basic_disp_pc_indirect<Size>::get(const context &c) const
    {
      return Size::get(*c.mem, c.data_fc(), address(c));
    }

    template <class Size> const char *
    basic_disp_pc_indirect<Size>::text(const context &c) const
    {
      static char buf[64];
      sprintf(buf, "%%pc@(%d)/* %#lx */", c.fetch(word_size(), offset),
	      address(c) + 0UL);
      return buf;
    }

    typedef basic_disp_pc_indirect<byte_size> byte_disp_pc_indirect;
    typedef basic_disp_pc_indirect<word_size> word_disp_pc_indirect;
    typedef basic_disp_pc_indirect<long_word_size> long_word_disp_pc_indirect;

    template <class Size> class basic_index_pc_indirect
    {
    public:
      typedef Size size_type;
      typedef typename Size::uvalue_type uvalue_type;
      typedef typename Size::svalue_type svalue_type;

    public:
      static size_t extension_size() {return 2;}

    private:
      size_t offset;

    public:
      basic_index_pc_indirect(unsigned int r, size_t off): offset(off) {}

    public:
      uint32_type address(const context &c) const;

      typename Size::svalue_type get(const context &c) const;

      void put(context &c, svalue_type value) const
	{Size::put(*c.mem, c.data_fc(), address(c), value);}
      void finish(context &c) const {}

    public:
      const char *text(const context &c) const;
    };

    template <class Size> inline uint32_type
    basic_index_pc_indirect<Size>::address(const context &c) const
    {
      uint_type w = c.fetch(word_size(), offset);
      unsigned int r = w >> 12 & 0xf;
      uint32_type x = r >= 8 ? c.regs.a[r - 8] : c.regs.d[r];
      if (w & 0x800)
	return c.regs.pc + offset + byte_size::get(w) + long_word_size::get(x);
      else
	return c.regs.pc + offset + byte_size::get(w) + word_size::get(x);
    }

    template <class Size> inline typename Size::svalue_type
    basic_index_pc_indirect<Size>::get(const context &c) const
    {
      return Size::get(*c.mem, c.data_fc(), address(c));
    }

    template <class Size> const char *
    basic_index_pc_indirect<Size>::text(const context &c) const
    {
      uint_type w = c.fetch(word_size(), offset);
      unsigned int r = w >> 12 & 0xf;
      static char buf[64];
      if (r >= 8)
	sprintf(buf, "%%pc@(%d,%%a%u%s)/* %#lx */", byte_size::get(w), r - 8,
		w & 0x800 ? ":l" : ":w", address(c) + 0UL);
      else
	sprintf(buf, "%%pc@(%d,%%d%u%s)/* %#lx */", byte_size::get(w), r,
		w & 0x800 ? ":l" : ":w", address(c) + 0UL);
      return buf;
    }

    typedef basic_index_pc_indirect<byte_size> byte_index_pc_indirect;
    typedef basic_index_pc_indirect<word_size> word_index_pc_indirect;
    typedef basic_index_pc_indirect<long_word_size>
      long_word_index_pc_indirect;

    template <class Size> class basic_immediate
    {
    public:
      typedef Size size_type;
      typedef typename Size::uvalue_type uvalue_type;
      typedef typename Size::svalue_type svalue_type;

    public:
      static size_t extension_size() {return Size::aligned_value_size();}

    private:
      size_t offset;

    public:
      basic_immediate(unsigned int r, size_t off): offset(off) {}

    public:
      // XXX address is left unimplemented.

      typename Size::svalue_type get(const context &) const;

      // XXX put is left unimplemented.
      void finish(context &c) const {}

    public:
      const char *text(const context &c) const;
    };

    template <class Size> inline typename Size::svalue_type
    basic_immediate<Size>::get(const context &c) const
    {
      return c.fetch(Size(), offset);
    }

    template <class Size> const char *
    basic_immediate<Size>::text(const context &c) const
    {
      static char buf[32];
      sprintf(buf, "#%#lx", (get(c) & Size::value_mask()) + 0UL);
      return buf;
    }

    typedef basic_immediate<byte_size> byte_immediate;
    typedef basic_immediate<word_size> word_immediate;
    typedef basic_immediate<long_word_size> long_word_immediate;

#ifdef VM68K_ENABLE_DEPRECATED

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
	{return byte_size::svalue(byte_size::get(ec.regs.d[reg]));}
      sint_type getw(const context &ec) const
	{return word_size::svalue(word_size::get(ec.regs.d[reg]));}
      sint32_type getl(const context &ec) const
	{return long_word_size::svalue(long_word_size::get(ec.regs.d[reg]));}
      void putb(context &ec, int value) const
	{byte_size::put(ec.regs.d[reg], value);}
      void putw(context &ec, sint_type value) const
	{word_size::put(ec.regs.d[reg], value);}
      void putl(context &ec, sint32_type value) const
	{long_word_size::put(ec.regs.d[reg], value);}
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
      sint_type getw(const context &ec) const
	{return word_size::svalue(word_size::get(ec.regs.a[reg]));}
      sint32_type getl(const context &ec) const
	{return long_word_size::svalue(long_word_size::get(ec.regs.a[reg]));}
      void putw(context &ec, sint_type value) const
	{long_word_size::put(ec.regs.a[reg], value);}
      void putl(context &ec, sint32_type value) const
	{long_word_size::put(ec.regs.a[reg], value);}
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
      uint32_type address(const context &ec) const
	{return ec.regs.a[reg];}
      int getb(const context &ec) const
	{return byte_size::svalue(byte_size::get(*ec.mem, ec.data_fc(),
						 address(ec)));}
      sint_type getw(const context &ec) const
	{return word_size::svalue(word_size::get(*ec.mem, ec.data_fc(),
						 address(ec)));}
      sint32_type getl(const context &ec) const
	{return long_word_size::svalue(long_word_size::get(*ec.mem,
							   ec.data_fc(),
							   address(ec)));}
      void putb(context &ec, int value) const
	{byte_size::put(*ec.mem, ec.data_fc(), address(ec), value);}
      void putw(context &ec, sint_type value) const
	{word_size::put(*ec.mem, ec.data_fc(), address(ec), value);}
      void putl(context &ec, sint32_type value) const
	{long_word_size::put(*ec.mem, ec.data_fc(), address(ec), value);}
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
      {return extsb(ec.mem->get_8(ec.data_fc(), ec.regs.a[reg]));}
      int getw(const context &ec) const
      {return extsw(ec.mem->get_16(ec.data_fc(), ec.regs.a[reg]));}
      sint32_type getl(const context &ec) const
      {return extsl(ec.mem->get_32(ec.data_fc(), ec.regs.a[reg]));}
      void putb(context &ec, int value) const
      {ec.mem->put_8(ec.data_fc(), ec.regs.a[reg], value);}
      void putw(context &ec, int value) const
      {ec.mem->put_16(ec.data_fc(), ec.regs.a[reg], value);}
      void putl(context &ec, sint32_type value) const
      {ec.mem->put_32(ec.data_fc(), ec.regs.a[reg], value);}
      void finishb(context &ec) const
	{ec.regs.a[reg] += reg == 7 ? 2 : 1;} // XXX: %a7 is special.
      void finishw(context &ec) const
	{ec.regs.a[reg] += 2;}
      void finishl(context &ec) const
	{ec.regs.a[reg] += 4;}
      const char *textb(const context &ec) const
	{return textw(ec);}
      const char *textw(const context &c) const
      {
	static char buf[64];
	sprintf(buf, "%%a%d@+/* %#lx */", reg, c.regs.a[reg] + 0UL);
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
      {return extsb(ec.mem->get_8(ec.data_fc(),
				  ec.regs.a[reg] - (reg == 7 ? 2 : 1)));}
				// XXX: %a7 is special.
      int getw(const context &ec) const
      {return extsw(ec.mem->get_16(ec.data_fc(), ec.regs.a[reg] - 2));}
      sint32_type getl(const context &ec) const
      {return extsl(ec.mem->get_32(ec.data_fc(), ec.regs.a[reg] - 4));}
      void putb(context &ec, int value) const
      {ec.mem->put_8(ec.data_fc(),
		     ec.regs.a[reg] - (reg == 7 ? 2 : 1), value);}
				// XXX: %a7 is special.
      void putw(context &ec, int value) const
      {ec.mem->put_16(ec.data_fc(), ec.regs.a[reg] - 2, value);}
      void putl(context &ec, sint32_type value) const
      {ec.mem->put_32(ec.data_fc(), ec.regs.a[reg] - 4, value);}
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
      uint32_type address(const context &ec) const
	{return ec.regs.a[reg] + extsw(ec.fetch(word_size(), offset));}
      int getb(const context &ec) const
      {return extsb(ec.mem->get_8(ec.data_fc(), address(ec)));}
      int getw(const context &ec) const
      {return extsw(ec.mem->get_16(ec.data_fc(), address(ec)));}
      sint32_type getl(const context &ec) const
      {return extsl(ec.mem->get_32(ec.data_fc(), address(ec)));}
      void putb(context &ec, int value) const
      {ec.mem->put_8(ec.data_fc(), address(ec), value);}
      void putw(context &ec, int value) const
      {ec.mem->put_16(ec.data_fc(), address(ec), value);}
      void putl(context &ec, sint32_type value) const
      {ec.mem->put_32(ec.data_fc(), address(ec), value);}
      void finishb(context &) const {}
      void finishw(context &) const {}
      void finishl(context &) const {}
      const char *textb(const context &ec) const
	{return textw(ec);}
      const char *textw(const context &ec) const
      {
	static char buf[64];
	sprintf(buf, "%%a%d@(%d)/* %#lx */", reg,
		extsw(ec.fetch(word_size(), offset)),
		address(ec) + 0UL);
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
	  uint_type w = ec.fetch(word_size(), offset);
	  unsigned int r = w >> 12 & 0xf;
	  uint32_type x = r >= 8 ? ec.regs.a[r - 8] : ec.regs.d[r];
	  return ec.regs.a[reg] + extsb(w) + (w & 0x800 ? extsl(x) : extsw(x));
	}
      sint_type getb(const context &ec) const
      {return extsb(ec.mem->get_8(ec.data_fc(), address(ec)));}
      sint_type getw(const context &ec) const
      {return extsw(ec.mem->get_16(ec.data_fc(), address(ec)));}
      sint32_type getl(const context &ec) const
      {return extsl(ec.mem->get_32(ec.data_fc(), address(ec)));}
      void putb(context &ec, int value) const
      {ec.mem->put_8(ec.data_fc(), address(ec), value);}
      void putw(context &ec, int value) const
      {ec.mem->put_16(ec.data_fc(), address(ec), value);}
      void putl(context &ec, sint32_type value) const
      {ec.mem->put_32(ec.data_fc(), address(ec), value);}
      void finishb(context &) const {}
      void finishw(context &) const {}
      void finishl(context &) const {}
      const char *textb(const context &ec) const
	{return textw(ec);}
      const char *textw(const context &ec) const
	{
	  uint_type w = ec.fetch(word_size(), offset);
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
      uint32_type address(const context &ec) const
	{return ec.fetch(word_size(), offset);}
      int getb(const context &ec) const
      {return extsb(ec.mem->get_8(ec.data_fc(), address(ec)));}
      int getw(const context &ec) const
      {return extsw(ec.mem->get_16(ec.data_fc(), address(ec)));}
      sint32_type getl(const context &ec) const
      {return extsl(ec.mem->get_32(ec.data_fc(), address(ec)));}
      void putb(context &ec, int value) const
      {ec.mem->put_8(ec.data_fc(), address(ec), value);}
      void putw(context &ec, int value) const
      {ec.mem->put_16(ec.data_fc(), address(ec), value);}
      void putl(context &ec, sint32_type value) const
      {ec.mem->put_32(ec.data_fc(), address(ec), value);}
      void finishb(context &) const {}
      void finishw(context &) const {}
      void finishl(context &) const {}
      const char *textb(const context &ec) const
	{return textw(ec);}
      const char *textw(const context &ec) const
      {
	static char buf[32];
	sprintf(buf, "%#lx:s", address(ec) + 0UL);
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
      uint32_type address(const context &ec) const
	{return ec.fetch(long_word_size(), offset);}
      int getb(const context &ec) const
      {return extsb(ec.mem->get_8(ec.data_fc(), address(ec)));}
      int getw(const context &ec) const
      {return extsw(ec.mem->get_16(ec.data_fc(), address(ec)));}
      sint32_type getl(const context &ec) const
      {return extsl(ec.mem->get_32(ec.data_fc(), address(ec)));}
      void putb(context &ec, int value) const
      {ec.mem->put_8(ec.data_fc(), address(ec), value);}
      void putw(context &ec, int value) const
      {ec.mem->put_16(ec.data_fc(), address(ec), value);}
      void putl(context &ec, sint32_type value) const
      {ec.mem->put_32(ec.data_fc(), address(ec), value);}
      void finishb(context &) const {}
      void finishw(context &) const {}
      void finishl(context &) const {}
      const char *textb(const context &ec) const
	{return textw(ec);}
      const char *textw(const context &ec) const
      {
	static char buf[32];
	sprintf(buf, "%#lx", address(ec) + 0UL);
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
      uint32_type address(const context &ec) const
	{return ec.regs.pc + offset + extsw(ec.fetch(word_size(), offset));}
      int getb(const context &ec) const
      {return extsb(ec.mem->get_8(ec.data_fc(), address(ec)));}
      int getw(const context &ec) const
      {return extsw(ec.mem->get_16(ec.data_fc(), address(ec)));}
      sint32_type getl(const context &ec) const
      {return extsl(ec.mem->get_32(ec.data_fc(), address(ec)));}
      // XXX: putb, putw, and putl are unimplemented.
      void finishb(context &ec) const {}
      void finishw(context &ec) const {}
      void finishl(context &ec) const {}
      const char *textb(const context &ec) const
	{return textw(ec);}
      const char *textw(const context &ec) const
	{
	  static char buf[16];
	  sprintf(buf, "%%pc@(%d)", extsw(ec.fetch(word_size(), offset)));
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
	  uint_type w = ec.fetch(word_size(), offset);
	  unsigned int r = w >> 12 & 0xf;
	  uint32_type x = r >= 8 ? ec.regs.a[r - 8] : ec.regs.d[r];
	  return ec.regs.pc + offset + extsb(w)
	    + (w & 0x800 ? extsl(x) : extsw(x));
	}
      sint_type getb(const context &ec) const
      {return extsb(ec.mem->get_8(ec.data_fc(), address(ec)));}
      sint_type getw(const context &ec) const
      {return extsw(ec.mem->get_16(ec.data_fc(), address(ec)));}
      sint32_type getl(const context &ec) const
      {return extsl(ec.mem->get_32(ec.data_fc(), address(ec)));}
      void putb(context &ec, int value) const
      {ec.mem->put_8(ec.data_fc(), address(ec), value);}
      void putw(context &ec, int value) const
      {ec.mem->put_16(ec.data_fc(), address(ec), value);}
      void putl(context &ec, sint32_type value) const
      {ec.mem->put_32(ec.data_fc(), address(ec), value);}
      void finishb(context &) const {}
      void finishw(context &) const {}
      void finishl(context &) const {}
      const char *textb(const context &ec) const
	{return textw(ec);}
      const char *textw(const context &ec) const
	{
	  uint_type w = ec.fetch(word_size(), offset);
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
	{return extsb(ec.fetch(word_size(), offset));}
      int getw(const context &ec) const
	{return extsw(ec.fetch(word_size(), offset));}
      sint32_type getl(const context &ec) const
	{return extsl(ec.fetch(long_word_size(), offset));}
      // XXX: putb, putw, and putl are unimplemented.
      void finishb(context &) const {}
      void finishw(context &) const {}
      void finishl(context &) const {}
      const char *textb(const context &ec) const
      {
	static char buf[32];
	sprintf(buf, "#0x%x", getb(ec));
	return buf;
      }
      const char *textw(const context &ec) const
      {
	static char buf[32];
	sprintf(buf, "#0x%x", getw(ec));
	return buf;
      }
      const char *textl(const context &ec) const
      {
	static char buf[32];
	sprintf(buf, "#%#lx", getl(ec) + 0UL);
	return buf;
      }
    };

#endif /* VM68K_ENABLE_DEPRECATED */
  } // addressing
} // vm68k

#endif /* not _VM68K_ADDRESSING_H */

