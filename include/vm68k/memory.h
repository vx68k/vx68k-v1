/* -*-C++-*- */

#ifndef VM68K_MEMORY_H
#define VM68k_MEMORY_H

#include <iterator>
#include "vm68k/types.h"

class bus_error
{
};

const int PAGE_SIZE_SHIFT = 13;
const size_t PAGE_SIZE = 1 < PAGE_SIZE_SHIFT;

struct memory_page
{
  virtual ~memory_page ();
  virtual void read (uint32, void *, size_t) const throw (bus_error) = 0;
  virtual void write (uint32, const void *, size_t) = 0;
  virtual uint16 read16 (uint32) const throw (bus_error) = 0;
  virtual uint32 read32 (uint32) const throw (bus_error);
  virtual uint8 read8 (uint32) const throw (bus_error) = 0;
  virtual void write16 (uint32, uint16) throw (bus_error) = 0;
  virtual void write32 (uint32, uint32) throw (bus_error);
  virtual void write8 (uint32, uint8) throw (bus_error) = 0;
};

class memory
{
public:
  struct iterator: bidirectional_iterator <uint16, int32>
  {
  };
  memory ();
  void read (uint32, void *, size_t) const;
  void write (uint32, const void *, size_t);
  uint16 read16 (uint32) const throw (bus_error);
  uint32 read32 (uint32) const throw (bus_error);
  uint8 read8 (uint32) const throw (bus_error);
  void write16 (uint32, uint16) throw (bus_error);
  write32 (uint32, uint32) throw (bus_error);
  void write8 (uint32, uint8) throw (bus_error);
protected:
  void set_memory_pages (int begin, int end, memory_page *);
private:
  memory_page *page[1 << 24 - PAGE_SIZE_SHIFT];
};

#endif

