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
#include <vm68k/processor.h>

#include <pthread.h>

#include <queue>
#include <memory>

namespace vx68k
{
  using namespace vm68k;
  using namespace std;

  /* Number of FD units.  */
  const size_t NFDS = 2;

  const size_t GRAPHICS_VRAM_SIZE = 2 * 1024 * 1024;

  /* Machine of X68000.  */
  class machine
  {
  public:
    static uint32_type jisx0201_16_address(unsigned int ch)
    {return font_rom::jisx0201_16_offset(ch) + 0xf00000;}

    static uint32_type jisx0201_24_address(unsigned int ch)
    {return font_rom::jisx0201_24_offset(ch) + 0xf00000;}

    static uint32_type jisx0208_16_address(unsigned int ch1, unsigned int ch2)
    {return font_rom::jisx0208_16_offset(ch1, ch2) + 0xf00000;}

    static uint32_type jisx0208_24_address(unsigned int ch1, unsigned int ch2)
    {return font_rom::jisx0208_24_offset(ch1, ch2) + 0xf00000;}

  private:
    size_t _memory_size;

    system_rom rom;

    main_memory mem;
    graphics_video_memory gv;
    text_video_memory tvram;
    crtc_memory crtc;
    palettes_memory palettes;
    dmac_memory dmac;
    area_set _area_set;
    mfp_memory mfp;
    system_ports_memory system_ports;
    opm_memory opm;
    msm6258v_memory adpcm;
    fdc_memory fdc;
    scc_memory scc;
    ppi_memory ppi;
    sprites_memory sprites;
    sram _sram;
    font_rom font;

    class processor eu;

    auto_ptr<memory_map> master_as;
    auto_ptr<context> _master_context;

  private:
    /* Last time when timers were checked.  */
    uint32_type last_check_time;

    /* Cursor position.  */
    unsigned int curx, cury;

    /* Saved byte 1 of double-byte character.  */
    unsigned char saved_byte1;

    /* Key input queue.  */
    queue<uint16_type> key_queue;

    /* Ready condition for key_queue.  */
    pthread_cond_t key_queue_not_empty;

    /* Mutex for key_queue.  */
    pthread_mutex_t key_queue_mutex;

    /* Key modifiers.  */
    uint16_type _key_modifiers;

    /* Floppy disks.  */
    iocs::disk *fd[NFDS];

  public:
    explicit machine(size_t);
    ~machine();

  public:
    size_t memory_size() const
      {return _memory_size;}
    class processor *processor()
      {return &eu;}

    context *master_context() const {return _master_context.get();}

  public:
    void connect(console *con);

    /* Configures address space AS.  */
    void configure(memory_map &as);

    /* Checks timers.  This function may be called in a separate
       thread.  */
    void check_timers(uint32_type t);

  public:
    unsigned int opm_status() const {return opm.status();}
    void set_opm_reg(unsigned int r, unsigned int v) {opm.set_reg(r, v);}
    bool opm_interrupt_enabled() const {return opm.interrupt_enabled();}
    void set_opm_interrupt_enabled(bool b) {opm.set_interrupt_enabled(b);}

    bool vdisp_interrupt_enabled() const
    {return crtc.vdisp_interrupt_enabled();}
    void set_vdisp_counter_data(unsigned int n)
    {crtc.set_vdisp_counter_data(n);}

    void set_mouse_state(unsigned int i, bool s) {scc.set_mouse_state(i, s);}
    void set_mouse_position(int x, int y) {scc.set_mouse_position(x, y);}

  public:
    /* Returns true once when the screen changed.  */
    bool screen_changed() {return palettes.check_text_colors_modified();}

    /* Returns true once when the row of the screen changed.  */
    bool row_changed(unsigned int y) {return tvram.row_changed(y);}

    /* Scans a row for display.  */
    template <class OutputIterator>
    void scan_row(unsigned int, OutputIterator first, OutputIterator last);

  public:
    /* Loads a file on a FD unit.  */
    void load_fd(unsigned int u, int fildes);

    /* Unloads the disk on a FD unit.  */
    void unload_fd(unsigned int u);

  public:
    void b_putc(uint16_type);
    void b_print(const memory_map *as, uint32_type);

  public:			// Keyboard Input
    /* Queues a key input.  */
    void queue_key(uint16_type key);

    /* Peeks a key input.  */
    uint16_type peek_key();

    /* Gets a key input from the queue.  */
    uint16_type get_key();

    /* Sets the key modifiers.  */
    void set_key_modifiers(uint16_type mask, uint16_type value);

    /* Returns the key modifiers.  */
    uint16_type key_modifiers() const {return _key_modifiers;}

  public:
    /* Reads blocks from a FD unit.  */
    sint32_type read_disk(memory_map &, uint16_type u, uint32_type pos,
			  uint32_type buf, uint32_type nbytes);

    /* Writes blocks from a FD unit.  */
    sint32_type write_disk(const memory_map &,
			   uint16_type u, uint32_type pos,
			   uint32_type buf, uint32_type nbytes) const;

    /* Boots up this machine using an address space.  */
    void boot(context &c);

    /* Boots up this machine.  */
    void boot();
  };

  template <class OutputIterator> void
  machine::scan_row(unsigned int y, OutputIterator first, OutputIterator last)
  {
    unsigned short text_colors[16];
    palettes.get_text_colors(0, 16, text_colors);

    text_video_memory::raster_iterator r = tvram.raster(0, y);
    for (; first != last; ++r, ++first)
      {
	uint16_type color = text_colors[*r];
	if (color != 0)
	  *first = color;
	else
	  *first = 0;
      }
  }

  /* X68000-specific address space.  This object acts as a program
     interface to the machine.  */
  class x68k_address_space: public memory_map
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
