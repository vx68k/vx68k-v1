/* -*-C++-*- */

#ifndef M68K_CPU_H
#define M68k_CPU_H

#include <iterator>
#include <climits>

#if INT_MAX >= 0x7fffffff
typedef int int32;
typedef unsigned int uint32;
#else
typedef long int32;
typedef unsigned long uint32;
#endif

typedef unsigned short uint16;
typedef unsigned char uint8;

/* User view of CPU registers.  */
struct user_cpu_regs
{
  uint32 r[16];
  uint32 pc;
  uint16 ccr;
};

/* CPU registers (mc68000).  */
struct cpu_regs: user_cpu_regs
{
  uint32 usp;
  uint32 ssp;
  uint16 sr;
};

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
  memory_page *page[1 << 32 - PAGE_SIZE_SHIFT];
};

struct interrupt_listener
{
  virtual ~interrupt_listener () {}
  virtual void bus_error (cpu_regs *, memory *) = 0;
  virtual void address_error (cpu_regs *, memory *) = 0;
  virtual void trap (int, cpu_regs *, memory *) = 0;
  virtual void interrupt (int, cpu_regs *, memory *) = 0;
};

class cpu
{
public:
  cpu (memory *);
  void set_pc (uint32);
  void run ();
  void set_interrupt_listener (interrupt_listener *);
protected:
  typedef void (*insn_handler) (int, cpu_regs *, memory *);
  void set_handlers (int begin, int end, insn_handler);
  static void undefined_insn (int, cpu_regs *, memory *);
private:
  cpu_regs regs;
  memory *mem;
  interrupt_listener *interrupt;
  insn_handler insn[0x10000];
};

#endif

