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

struct memory
{
  struct iterator: bidirectional_iterator <uint16, int32>
  {
  };
  virtual ~memory ();
  virtual uint16 read16 (uint32) throw (bus_error) = 0;
  virtual uint32 read32 (uint32) throw (bus_error);
  virtual uint8 read8 (uint32) throw (bus_error) = 0;
  virtual void write16 (uint32, uint16) throw (bus_error) = 0;
  virtual void write32 (uint32, uint32) throw (bus_error);
  virtual void write8 (uint32, uint8) throw (bus_error) = 0;
};

struct interrupt_listener
{
  virtual ~interrupt_listener () {}
  virtual void bus_error (cpu_regs *, memory *) = 0;
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
  void set_handler (int code, int mask, void (*) (cpu_regs *, memory *));
private:
  cpu_regs regs;
  memory *mem;
  void (*insn[0x10000]) (cpu_regs *, memory *);
  interrupt_listener *interrupt;
};

#endif

