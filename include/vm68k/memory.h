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

#include <vm68k/types.h>

#include <vector>
#include <iterator>
#include <string>
#include <exception>

namespace vm68k
{
  using namespace std;

  const unsigned int PAGE_SHIFT = 12;
  const uint32_type PAGE_SIZE = uint32_type(1) << PAGE_SHIFT;

  // External mc68000 address is 24-bit size.
  const unsigned int ADDRESS_BIT = 24;
  const uint32_type NPAGES = uint32_type(1) << ADDRESS_BIT - PAGE_SHIFT;

  /* Helper iterator for 16-bit values.  This iterator accesses two
     bytes once to handle a 16-bit value.  */
  template <class T>
  class basic_uint16_iterator: random_access_iterator<uint16_type, ptrdiff_t>
  {
  protected:
    class ref
    {
    private:
      T bp;

    public:
      ref(T ptr): bp(ptr) {}

    public:
      ref &operator=(uint16_type);
      operator uint16_type() const;
    };

  private:
    T bp;

  public:
    basic_uint16_iterator(T ptr): bp(ptr) {}
    template <class U> explicit basic_uint16_iterator(U ptr)
      : bp(static_cast<T>(ptr)) {}

  public:
    uint16_type operator*() const {return ref(bp);}
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

    uint16_type operator[](ptrdiff_t n) const {return *(*this + n);}
    ref operator[](ptrdiff_t n) {return *(*this + n);}

  public:
    operator T() const {return bp;}
  };

  template <class T> inline basic_uint16_iterator<T>::ref &
  basic_uint16_iterator<T>::ref::operator=(uint16_type value)
  {
    bp[0] = value >> 8;
    bp[1] = value;
    return *this;
  }

  template <class T> inline
  basic_uint16_iterator<T>::ref::operator uint16_type() const
  {
    return uint16_type(bp[0] & 0xff) << 8 | bp[1] & 0xff;
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

  typedef basic_uint16_iterator<unsigned char *> uint16_iterator;
  typedef basic_uint16_iterator<const unsigned char *> const_uint16_iterator;

  /* Helper iterator for 32-bit value.  This iterator accesses four
     bytes once to handle a 32-bit value.  */
  template <class T>
  class basic_uint32_iterator: random_access_iterator<uint32_type, ptrdiff_t>
  {
  protected:
    class ref
    {
    private:
      T bp;

    public:
      ref(T ptr): bp(ptr) {}

    public:
      ref &operator=(uint32_type);
      operator uint32_type() const;
    };

  private:
    T bp;

  public:
    basic_uint32_iterator(T ptr): bp(ptr) {}
    template <class U> explicit basic_uint32_iterator(U ptr)
      : bp(static_cast<T >(ptr)) {}

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
    operator T() const {return bp;}
  };

  template <class T> inline basic_uint32_iterator<T>::ref &
  basic_uint32_iterator<T>::ref::operator=(uint32_type value)
  {
    bp[0] = value >> 24;
    bp[1] = value >> 16;
    bp[2] = value >>  8;
    bp[3] = value;
    return *this;
  }

  template <class T> inline
  basic_uint32_iterator<T>::ref::operator uint32_type() const
  {
    return (uint32_type(bp[0] & 0xff) << 24 | uint32_type(bp[1] & 0xff) << 16
	    | uint32_type(bp[2] & 0xff) << 8 | uint32_type(bp[3] & 0xff));
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

  typedef basic_uint32_iterator<unsigned char *> uint32_iterator;
  typedef basic_uint32_iterator<const unsigned char *> const_uint32_iterator;

  inline uint16_type
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
  putw(void *p, uint16_type v)
  {
    *uint16_iterator(p) = v;
  }

  inline void
  putl(void *p, uint32_type v)
  {
    *uint32_iterator(p) = v;
  }

  /* Bus error or address error.  */
  struct memory_exception: exception
  {
    int vecno;
    uint16_type status;
    uint32_type address;
    memory_exception(int v, bool read, int fc, uint32_type a)
      : vecno(v), status(read ? fc | 0x10 : fc), address(a) {}
  };

  /* Bus error exception.  */
  struct bus_error_exception: memory_exception
  {
    bus_error_exception(bool read, int fc, uint32_type a)
      : memory_exception(2, read, fc, a) {}
    const char *what() {return "bus_error_exception";}
  };

  /* Address error exception.  */
  struct address_error_exception: memory_exception
  {
    address_error_exception(bool read, int fc, uint32_type a)
      : memory_exception(3, read, fc, a) {}
    const char *what() {return "address_error_exception";}
  };

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

  public:
    virtual int get_8(uint32_type address, function_code) const = 0;
    virtual uint16_type get_16(uint32_type address, function_code) const = 0;
    virtual uint32_type get_32(uint32_type address, function_code) const;

    virtual void put_8(uint32_type address, int, function_code) = 0;
    virtual void put_16(uint32_type address, uint16_type, function_code) = 0;
    virtual void put_32(uint32_type address, uint32_type, function_code);
  };

  /* Maps an address space to memories.  An address space is a
     software view of a target machine.  */
  class memory_map
  {
  public:
    typedef memory::function_code function_code;

  private:
    vector<memory *> page_table;

  public:
    memory_map();
    virtual ~memory_map() {}

  protected:
    /* Finds a page that contains address ADDRESS.  */
    vector<memory *>::const_iterator find_memory(uint32_type address) const;
    vector<memory *>::iterator find_memory(uint32_type address);

  public:
    /* Fills an address range with memory.  */
    void fill(uint32_type, uint32_type, memory *);

  public:
    /* Returns one byte at address ADDRESS in this address space.  */
    int get_8(uint32_type address, function_code fc) const
      throw (memory_exception);

    /* Returns one word at address ADDRESS in this address space.  Any
       unaligned address will be handled.  */
    uint16_type get_16(uint32_type address, function_code fc) const
      throw (memory_exception);

    /* Returns one word at address ADDRESS in this address space.
       The address must be word-aligned.  */
    uint16_type get_16_unchecked(uint32_type address, function_code fc) const
      throw (memory_exception);

    /* Returns one long word at address ADDRESS in this address space.
       Any unaligned address will be handled.  */
    uint32_type get_32(uint32_type address, function_code fc) const
      throw (memory_exception);

    string get_string(uint32_type address, function_code fc) const;

    void read(uint32_type, void *, size_t, function_code fc) const;

    /* Stores byte VALUE at address ADDRESS in this address space.  */
    void put_8(uint32_type address, int value, function_code fc)
      throw (memory_exception);

    /* Stores word VALUE at address ADDRESS in this address space.
       Any unaligned address will be handled.  */
    void put_16(uint32_type address, uint16_type value, function_code fc)
      throw (memory_exception);

    /* Stores word VALUE at address ADDRESS in this address space.
       The address must be word-aligned.  */
    void put_16_unchecked(uint32_type address, uint16_type value,
			  function_code fc)
      throw (memory_exception);

    /* Stores long word VALUE at address ADDRESS in this address
       space.  Any unaligned address will be handled.  */
    void put_32(uint32_type address, uint32_type value, function_code fc)
      throw (memory_exception);

    void put_string(uint32_type address, const string &, function_code fc);

    void write(uint32_type, const void *, size_t, function_code fc);
  };

  inline vector<memory *>::const_iterator
  memory_map::find_memory(uint32_type address) const
  {
    return page_table.begin() + (address >> PAGE_SHIFT) % NPAGES;
  }

  inline vector<memory *>::iterator
  memory_map::find_memory(uint32_type address)
  {
    return page_table.begin() + (address >> PAGE_SHIFT) % NPAGES;
  }

  inline int
  memory_map::get_8(uint32_type address, function_code fc) const
    throw (memory_exception)
  {
    const memory *p = *this->find_memory(address);
    return p->get_8(address, fc);
  }

  inline uint16_type
  memory_map::get_16_unchecked(uint32_type address, function_code fc) const
    throw (memory_exception)
  {
    const memory *p = *this->find_memory(address);
    return p->get_16(address, fc);
  }

  inline void
  memory_map::put_8(uint32_type address, int value, function_code fc)
    throw (memory_exception)
  {
    memory *p = *this->find_memory(address);
    p->put_8(address, value, fc);
  }

  inline void
  memory_map::put_16_unchecked(uint32_type address, uint16_type value,
			       function_code fc)
    throw (memory_exception)
  {
    memory *p = *this->find_memory(address);
    p->put_16(address, value, fc);
  }
}

#endif /* not _VM68K_MEMORY_H */
