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

#ifndef _VX68K_MACHINE_H
#define _VX68K_MACHINE_H 1

#include <vx68k/memory.h>
#include <vx68k/iocs.h>
#include <vm68k/cpu.h>
#include <pthread.h>
#include <queue>

namespace vx68k
{
  using namespace vm68k;
  using namespace std;

  /* Number of FD units.  */
  const size_t NFDS = 2;

  /* X68000 main memory.  */
  class main_memory: public memory
  {
  private:
    uint32_type end;
    unsigned short *array;

  public:
    explicit main_memory(size_t);
    ~main_memory();

  public:
    uint_type get_16(int, uint32_type) const;
    uint_type get_8(int, uint32_type) const;
    uint32_type get_32(int, uint32_type) const;

    void put_16(int, uint32_type, uint_type);
    void put_8(int, uint32_type, uint_type);
    void put_32(int, uint32_type, uint32_type);
  };

  /* SRAM.  */
  class sram: public memory
  {
  private:
    unsigned char *buf;

  public:
    ~sram();
    sram();

  public:
    /* Reads data from this object.  */
    uint_type get_16(int, uint32_type) const;
    uint_type get_8(int, uint32_type) const;

    /* Writes data to this object.  */
    void put_16(int, uint32_type, uint_type);
    void put_8(int, uint32_type, uint_type);
  };

  /* System ROM.  This object handles the IOCS calls.  */
  class system_rom: public memory
  {
  public:
    typedef void (*iocs_function_handler)(context &, unsigned long);
    typedef pair<iocs_function_handler, unsigned long> iocs_function_type;

  protected:
    static void invalid_iocs_function(context &, unsigned long);

  private:
    /* Table of the IOCS functions.  */
    vector<iocs_function_type> iocs_functions;

    /* Attached execution unit.  */
    exec_unit *attached_eu;

  public:
    system_rom();
    ~system_rom();

  public:
    /* Attaches or detaches an execution unit.  */
    void attach(exec_unit *);
    void detach(exec_unit *);

    /* Initializes memory in an address space.  */
    void initialize(address_space &);

  public:
    /* Sets an IOCS function.  */
    void set_iocs_function(uint_type, const iocs_function_type &);

    /* Dispatch to an IOCS function.  */
    void dispatch_iocs_function(context &);

  public:
    /* Reads data from this object.  */
    uint_type get_16(int, uint32_type) const;
    uint_type get_8(int, uint32_type) const;

    /* Writes data to this object.  These methods shall always fail.  */
    void put_16(int, uint32_type, uint_type);
    void put_8(int, uint32_type, uint_type);
  };

  const size_t GRAPHICS_VRAM_SIZE = 2 * 1024 * 1024;

  /* Graphics VRAM.  */
  class graphics_vram
    : public memory
  {
  private:
    unsigned short *base;
    console *connected_console;

  public:
    graphics_vram();
    ~graphics_vram();

  public:
    size_t read(int, uint32_type, void *, size_t) const;
    uint_type getb(int, uint32_type) const;
    uint_type getw(int, uint32_type) const;

  public:
    size_t write(int, uint32_type, const void *, size_t);
    void putb(int, uint32_type, uint_type);
    void putw(int, uint32_type, uint_type);

  public:
    void connect(console *);
  };

  /* Machine of X68000.  */
  class machine
  {
  private:
    size_t _memory_size;
    main_memory mem;
    text_video_memory tvram;
    crtc_memory crtc;
    area_set _area_set;
    scc_memory scc;
    ppi_memory ppi;
    sram _sram;
    system_rom rom;
    class exec_unit eu;

    /* Cursor position.  */
    unsigned int curx, cury;

    /* Saved byte 1 of double-byte character.  */
    unsigned char saved_byte1;

    /* Key input queue.  */
    queue<uint_type> key_queue;

    /* Ready condition for key_queue.  */
    pthread_cond_t key_queue_not_empty;

    /* Mutex for key_queue.  */
    pthread_mutex_t key_queue_mutex;

    /* Key modifiers.  */
    uint_type _key_modifiers;

    /* Floppy disks.  */
    iocs::disk *fd[NFDS];

  public:
    explicit machine(size_t);
    ~machine();

  public:
    size_t memory_size() const
      {return _memory_size;}
    class exec_unit *exec_unit()
      {return &eu;}

  public:
    void connect(console *con)
      {tvram.connect(con);}
    void get_image(int x, int y, int width, int height,
		   unsigned char *rgb_buf, size_t row_size);

    /* Configures address space AS.  */
    void configure(address_space &as);

  public:
    /* Loads a file on a FD unit.  */
    void load_fd(unsigned int u, int fildes);

    /* Unloads the disk on a FD unit.  */
    void unload_fd(unsigned int u);

  public:
    void b_putc(uint_type);
    void b_print(const address_space *as, uint32_type);

  public:			// Keyboard Input
    /* Queues a key input.  */
    void queue_key(uint_type key);

    /* Peeks a key input.  */
    uint_type peek_key();

    /* Gets a key input from the queue.  */
    uint_type get_key();

    /* Sets the key modifiers.  */
    void set_key_modifiers(uint_type mask, uint_type value);

    /* Returns the key modifiers.  */
    uint_type key_modifiers() const {return _key_modifiers;}

  public:
    /* Reads blocks from a FD unit.  */
    sint32_type read_disk(address_space &, uint_type u, uint32_type pos,
			  uint32_type buf, uint32_type nbytes);

    /* Writes blocks from a FD unit.  */
    sint32_type write_disk(const address_space &, uint_type u, uint32_type pos,
			   uint32_type buf, uint32_type nbytes) const;

    /* Boots up this machine using an address space.  */
    void boot(context &c);

    /* Boots up this machine.  */
    void boot();
  };

  /* X68000-specific address space.  This object acts as a program
     interface to the machine.  */
  class x68k_address_space: public address_space
  {
  private:
    class machine *_m;

  public:
    x68k_address_space(class machine *m);

  public:
    /* Shortcut for the emulated machine.  */
    class machine *machine() const
    {return _m;}
  };
} // vx68k

#endif /* not _VX68K_MACHINE_H */
