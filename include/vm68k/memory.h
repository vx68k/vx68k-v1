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

  const unsigned int PAGE_SHIFT = 12;
  const uint32_type PAGE_SIZE = uint32_type(1) << PAGE_SHIFT;

  // External mc68000 address is 24-bit size.
  const unsigned int ADDRESS_BIT = 24;
  const uint32_type NPAGES = uint32_type(1) << ADDRESS_BIT - PAGE_SHIFT;

  /* Helper iterator for 16-bit value.  */
  template <class T>
  class basic_uint16_iterator: random_access_iterator<uint_type, ptrdiff_t>
  {
  protected:
    class ref
    {
    private:
      T *bp;

    public:
      ref(T *ptr): bp(ptr) {}

    public:
      ref &operator=(uint_type);
      operator uint_type() const;
    };

  private:
    T *bp;

  public:
    basic_uint16_iterator(T *ptr): bp(ptr) {}
    template <class U> explicit basic_uint16_iterator(U *ptr)
      : bp(static_cast<T *>(ptr)) {}

  public:
    uint_type operator*() const {return ref(bp);}
    ref operator*() {return ref(bp);}

    basic_uint16_iterator &operator++();
    basic_uint16_iterator operator++(int);

    basic_uint16_iterator &operator--();
    basic_uint16_iterator operator--(int);

    basic_uint16_iterator &operator+=(ptrdiff_t n);
    basic_uint16_iterator operator+(ptrdiff_t n) const
    {return basic_uint16_iterator(*this) += n;}

    basic_uint16_iterator &operator-=(ptrdiff_t n);
    basic_uint16_iterator operator-(ptrdiff_t n) const
    {return basic_uint16_iterator(*this) -= n;}

    uint_type operator[](ptrdiff_t n) const {return *(*this + n);}
    ref operator[](ptrdiff_t n) {return *(*this + n);}

  public:
    operator T *() const {return bp;}
  };

  template <class T> inline basic_uint16_iterator<T>::ref &
  basic_uint16_iterator<T>::ref::operator=(uint_type value)
  {
    bp[0] = value >> 8 & 0xff;
    bp[1] = value & 0xff;
    return *this;
  }

  template <class T> inline
  basic_uint16_iterator<T>::ref::operator uint_type() const
  {
    return uint_type(bp[0]) << 8 | uint_type(bp[1]);
  }

  template <class T> inline basic_uint16_iterator<T> &
  basic_uint16_iterator<T>::operator++()
  {
    bp += 2;
    return *this;
  }

  template <class T> inline basic_uint16_iterator<T>
  basic_uint16_iterator<T>::operator++(int)
  {
    basic_uint16_iterator<T> old = *this;
    ++(*this);
    return old;
  }

  template <class T> inline basic_uint16_iterator<T> &
  basic_uint16_iterator<T>::operator--()
  {
    bp -= 2;
    return *this;
  }

  template <class T> inline basic_uint16_iterator<T>
  basic_uint16_iterator<T>::operator--(int)
  {
    basic_uint16_iterator<T> old = *this;
    --(*this);
    return old;
  }

  template <class T> inline basic_uint16_iterator<T> &
  basic_uint16_iterator<T>::operator+=(ptrdiff_t n)
  {
    bp += n * 2;
    return *this;
  }

  template <class T> inline basic_uint16_iterator<T> &
  basic_uint16_iterator<T>::operator-=(ptrdiff_t n)
  {
    bp -= n * 2;
    return *this;
  }

  typedef basic_uint16_iterator<unsigned char> uint16_iterator;
  typedef basic_uint16_iterator<const unsigned char> const_uint16_iterator;

  /* Helper iterator for 32-bit value.  */
  template <class T>
  class basic_uint32_iterator: random_access_iterator<uint32_type, ptrdiff_t>
  {
  protected:
    class ref
    {
    private:
      T *bp;

    public:
      ref(T *ptr): bp(ptr) {}

    public:
      ref &operator=(uint32_type);
      operator uint32_type() const;
    };

  private:
    T *bp;

  public:
    basic_uint32_iterator(T *ptr): bp(ptr) {}
    template <class U> explicit basic_uint32_iterator(U *ptr)
      : bp(static_cast<T *>(ptr)) {}

  public:
    uint32_type operator*() const {return ref(bp);}
    ref operator*() {return ref(bp);}

    basic_uint32_iterator &operator++();
    basic_uint32_iterator operator++(int);

    basic_uint32_iterator &operator--();
    basic_uint32_iterator operator--(int);

    basic_uint32_iterator &operator+=(ptrdiff_t n);
    basic_uint32_iterator operator+(ptrdiff_t n) const
    {return basic_uint32_iterator(*this) += n;}

    basic_uint32_iterator &operator-=(ptrdiff_t n);
    basic_uint32_iterator operator-(ptrdiff_t n) const
    {return basic_uint32_iterator(*this) -= n;}

    uint32_type operator[](ptrdiff_t n) const {return *(*this + n);}
    ref operator[](ptrdiff_t n) {return *(*this + n);}

  public:
    operator T *() const {return bp;}
  };

  template <class T> inline basic_uint32_iterator<T>::ref &
  basic_uint32_iterator<T>::ref::operator=(uint32_type value)
  {
    bp[0] = value >> 24 & 0xff;
    bp[1] = value >> 16 & 0xff;
    bp[2] = value >>  8 & 0xff;
    bp[3] = value       & 0xff;
    return *this;
  }

  template <class T> inline
  basic_uint32_iterator<T>::ref::operator uint32_type() const
  {
    return (uint32_type(bp[0]) << 24 | uint32_type(bp[1]) << 16
	    | uint32_type(bp[2]) << 8 | uint32_type(bp[3]));
  }

  template <class T> inline basic_uint32_iterator<T> &
  basic_uint32_iterator<T>::operator++()
  {
    bp += 4;
    return *this;
  }

  template <class T> inline basic_uint32_iterator<T>
  basic_uint32_iterator<T>::operator++(int)
  {
    basic_uint32_iterator<T> old = *this;
    ++(*this);
    return old;
  }

  template <class T> inline basic_uint32_iterator<T> &
  basic_uint32_iterator<T>::operator--()
  {
    bp -= 4;
    return *this;
  }

  template <class T> inline basic_uint32_iterator<T>
  basic_uint32_iterator<T>::operator--(int)
  {
    basic_uint32_iterator<T> old = *this;
    --(*this);
    return old;
  }

  template <class T> inline basic_uint32_iterator<T> &
  basic_uint32_iterator<T>::operator+=(ptrdiff_t n)
  {
    bp += n * 4;
    return *this;
  }

  template <class T> inline basic_uint32_iterator<T> &
  basic_uint32_iterator<T>::operator-=(ptrdiff_t n)
  {
    bp -= n * 4;
    return *this;
  }

  typedef basic_uint32_iterator<unsigned char> uint32_iterator;
  typedef basic_uint32_iterator<const unsigned char> const_uint32_iterator;

  inline uint_type
  getw(const void *p)
  {
    return *const_uint16_iterator(p);
  }

  inline uint32_type
  getl(const void *p)
  {
    return *const_uint32_iterator(p);
  }

  inline void
  putw(void *p, uint_type v)
  {
    *uint16_iterator(p) = v;
  }

  inline void
  putl(void *p, uint32_type v)
  {
    *uint32_iterator(p) = v;
  }

  /* Abstract memory class.  */
  class memory
  {
  public:
    /* Function code, which identifies an access type.  */
    enum function_code
    {
      USER_DATA = 1,
      USER_PROGRAM = 2,
      SUPER_DATA = 5,
      SUPER_PROGRAM = 6
    };

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
