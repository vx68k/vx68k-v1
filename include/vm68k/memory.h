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

#ifndef VM68K_MEMORY_H
#define VM68K_MEMORY_H 1

#include <iterator>
#include "vm68k/types.h"
#include "vm68k/except.h"

namespace vm68k
{

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

uint16 getw (const void *);
uint32 getl (const void *);
void putw (void *, uint16);
void putl (void *, uint32);

  struct memory_page
  {
    virtual ~memory_page () {}
    virtual size_t read(int, uint32, void *, size_t) const = 0;
    virtual size_t write(int, uint32, const void *, size_t) = 0;
    virtual uint8 getb (int, uint32) const throw (bus_error) = 0;
    virtual uint16 getw (int, uint32) const throw (bus_error) = 0;
    virtual uint32 getl (int, uint32) const throw (bus_error);
    virtual void putb (int, uint32, uint8) throw (bus_error) = 0;
    virtual void putw (int, uint32, uint16) throw (bus_error) = 0;
    virtual void putl (int, uint32, uint32) throw (bus_error);
  };

  /* Memory page that always raises a bus error.  */
  class bus_error_page
    : public memory_page
  {
  public:
    virtual size_t read(int, uint32, void *, size_t) const;
    virtual size_t write(int, uint32, const void *, size_t);
    virtual uint8 getb (int, uint32) const throw (bus_error);
    virtual uint16 getw (int, uint32) const throw (bus_error);
    virtual void putb (int, uint32, uint8) throw (bus_error);
    virtual void putw (int, uint32, uint16) throw (bus_error);
  };

  class address_space
  {
  public:
#if 0
    struct iterator: bidirectional_iterator <uint16, int32>
    {
    };
#endif
    address_space ();
    void set_pages (size_t begin, size_t end, memory_page *);
    void read (int, uint32, void *, size_t) const;
    unsigned int getb (int, uint32) const;
    unsigned int getw (int, uint32) const;
    uint32 getl (int, uint32) const;
    size_t gets(int, uint32, char *, size_t) const;
    void write (int, uint32, const void *, size_t);
    void putb (int, uint32, unsigned int);
    void putw (int, uint32, unsigned int);
    void putl (int, uint32, uint32);
    size_t puts(int, uint32, const char *);
  private:
    bus_error_page default_page;
    memory_page *page_table[NPAGES];
  };

};				// namespace vm68k

#endif

