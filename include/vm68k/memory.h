/* -*- C++ -*- */
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

#ifndef _VM68K_MEMORY_H
#define _VM68K_MEMORY_H 1

#include <vm68k/except.h>
#include <vm68k/types.h>
#include <iterator>
#include <string>

namespace vm68k
{
  using namespace std;

  /* Bus function code.  */
  enum function_code
  {
    USER_DATA = 1,
    USER_PROGRAM = 2,
    SUPER_DATA = 5,
    SUPER_PROGRAM = 6
  };

  const int PAGE_SHIFT = 12;
  const size_t PAGE_SIZE = (size_t) 1 << PAGE_SHIFT;

  // External mc68000 address is 24-bit size.
  const int ADDRESS_BIT = 24;
  const size_t NPAGES = (size_t) 1 << ADDRESS_BIT - PAGE_SHIFT;

  uint_type getw(const void *);
  uint32_type getl(const void *);
  void putw(void *, uint_type);
  void putl(void *, uint32_type);

  /* Abstract memory class.  */
  class memory
  {
  public:
    virtual ~memory() {}

  protected:
    void generate_bus_error(bool, int, uint32_type) const;

  public:
    virtual uint_type get_16(int, uint32_type) const = 0;
    virtual uint_type get_8(int, uint32_type) const = 0;
    virtual uint32_type get_32(int, uint32_type) const;

    virtual void put_16(int, uint32_type, uint_type) = 0;
    virtual void put_8(int, uint32_type, uint_type) = 0;
    virtual void put_32(int, uint32_type, uint32_type);
  };

  /* Default memory that always raises a bus error.  */
  class default_memory: public memory
  {
  public:
    uint_type get_16(int, uint32_type) const;
    uint_type get_8(int, uint32_type) const;

    void put_16(int, uint32_type, uint_type);
    void put_8(int, uint32_type, uint_type);
  };

  /* Address space for memories.  An address space is a software view
     of a target machine.  */
  class memory_address_space
  {
  protected:
    /* Returns the canonical address for ADDRESS.  */
    static uint32_type canonical_address(uint32_type address)
      {return address & (uint32_type(1) << ADDRESS_BIT) - 1;}

  private:
    memory *page_table[NPAGES];

  public:
#if 0
    struct iterator: bidirectional_iterator <uint16, int32>
    {
    };
#endif
    virtual ~memory_address_space() {}
    memory_address_space();

  protected:
    /* Finds a page that contains canonical address ADDRESS.  */
    memory *find_page(uint32_type address) const
      {return page_table[address >> PAGE_SHIFT];}

  public:
    void set_pages(size_t begin, size_t end, memory *);

    void read(int, uint32_type, void *, size_t) const;

    /* Returns one byte at address ADDRESS in this address space.  */
    uint_type getb(int fc, uint32_type address) const
    {
      address = canonical_address(address);
      const memory *p = find_page(address);
      return p->get_8(fc, address);
    }

    /* Returns one word at address ADDRESS in this address space.
       The address must be word-aligned.  */
    uint_type getw_aligned(int fc, uint32_type address) const
    {
      address = canonical_address(address);
      const memory *p = find_page(address);
      return p->get_16(fc, address);
    }

    /* Returns one word at address ADDRESS in this address space.  Any
       unaligned address will be handled.  */
    uint_type getw(int fc, uint32_type address) const;

    /* Returns one long word at address ADDRESS in this address space.
       Any unaligned address will be handled.  */
    uint32_type getl(int fc, uint32_type address) const;

    string gets(int, uint32_type) const;

  public:
    void write(int, uint32_type, const void *, size_t);

    /* Stores byte VALUE at address ADDRESS in this address space.  */
    void putb(int fc, uint32_type address, uint_type value)
    {
      address = canonical_address(address);
      memory *p = find_page(address);
      p->put_8(fc, address, value);
    }

    /* Stores word VALUE at address ADDRESS in this address space.
       The address must be word-aligned.  */
    void putw_aligned(int fc, uint32_type address, uint_type value)
    {
      address = canonical_address(address);
      memory *p = find_page(address);
      p->put_16(fc, address, value);
    }

    /* Stores word VALUE at address ADDRESS in this address space.
       Any unaligned address will be handled.  */
    void putw(int fc, uint32_type address, uint_type value);

    /* Stores long word VALUE at address ADDRESS in this address
       space.  Any unaligned address will be handled.  */
    void putl(int fc, uint32_type address, uint32_type value);

    void puts(int, uint32_type, const string &);
  };
} // vm68k

#endif /* not _VM68K_MEMORY_H */
