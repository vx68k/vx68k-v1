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

/* Memory components for Virtual X68000.  */

#ifndef _VX68K_MEMORY_H
#define _VX68K_MEMORY_H 1

#include <vm68k/cpu.h>
#include <vm68k/memory.h>

#include <pthread.h>

#include <vector>

namespace vx68k
{
  using vm68k::memory;
  using vm68k::memory_address_space;
  using vm68k::context;
  using vm68k::exec_unit;
  using namespace vm68k::types;
  using namespace std;

  /* Interface to console.  */
  struct console
  {
    typedef uint32_type time_type;
    virtual time_type current_time() const = 0;

    virtual void get_b16_image(unsigned int, unsigned char *, size_t) const = 0;
    virtual void get_k16_image(unsigned int, unsigned char *, size_t) const = 0;
  };

  /* System ROM.  This object handles the IOCS calls.  */
  class system_rom: public memory
  {
  public:
    typedef void (*iocs_handler_function)(context &, unsigned long);
    typedef pair<iocs_handler_function, unsigned long> iocs_handler_type;

  protected:
    static void invalid_iocs_function(context &, unsigned long);

  private:
    /* Table of the IOCS functions.  */
    vector<iocs_handler_type> iocs_handlers;

    /* Attached execution unit.  */
    exec_unit *attached_eu;

  public:
    system_rom();
    ~system_rom();

  public:
    /* Reads data from this object.  */
    uint16_type get_16(function_code, uint32_type) const;
    int get_8(function_code, uint32_type) const;

    /* Writes data to this object.  These methods shall always fail.  */
    void put_16(function_code, uint32_type, uint16_type);
    void put_8(function_code, uint32_type, int);

  public:
    /* Attaches or detaches an execution unit.  */
    void attach(exec_unit *);
    void detach(exec_unit *);

    /* Initializes memory in an address space.  */
    void initialize(memory_address_space &);

  public:
    /* Sets an IOCS function.  */
    void set_iocs_handler(int, const iocs_handler_type &);

    /* Dispatch to an IOCS call handler.  */
    void call_iocs(int, context &);
  };

  /* Main memory.  This memory is mapped to the address range from 0
     to 0xc00000.  */
  class main_memory: public memory
  {
  private:
    /* End of available memory.  */
    uint32_type end;

    /* End of supervisor area.  */
    uint32_type super_area;

    /* Memory contents.  */
    unsigned short *data;

  public:
    explicit main_memory(size_t n);
    ~main_memory();

  public:
    /* Reads data from this object.  */
    uint16_type get_16(function_code, uint32_type) const;
    int get_8(function_code, uint32_type) const;
    uint32_type get_32(function_code, uint32_type) const;

    /* Writes data to this object.  */
    void put_16(function_code, uint32_type, uint16_type);
    void put_8(function_code, uint32_type, int);
    void put_32(function_code, uint32_type, uint32_type);

  public:
    void set_super_area(size_t n);
  };

  /* Graphics video memory.  This memory is mapped to the address
     range from 0xc00000 to 0xe00000 on X68000.  */
  class graphics_video_memory: public memory
  {
  public:
    /* Reads data from this object.  */
    uint16_type get_16(function_code, uint32_type) const;
    int get_8(function_code, uint32_type) const;

    /* Writes data to this object.  */
    void put_16(function_code, uint32_type, uint16_type);
    void put_8(function_code, uint32_type, int);
  };

  /* Raster iterator for the text VRAM.  This class is used by the
     video system to scan pixels on the text VRAM, and must be
     efficient for a forward sequential access.

     FIXME: this should be a random access iterator.  */
  class text_video_raster_iterator: public forward_iterator<int, int>
  {
  private:
    unsigned char *buf;
    unsigned int pos;
    unsigned char packs[4];

  public:
    text_video_raster_iterator()
      : buf(NULL), pos(0) {}

    text_video_raster_iterator(unsigned char *b, unsigned int p);

  public:
    bool operator==(const text_video_raster_iterator &another) const
    {
      /* BUF is not tested for speed.  */
      return pos == another.pos;
    }

    bool operator!=(const text_video_raster_iterator &another) const
    {
      return !(*this == another);
    }

  public:
    int operator*() const;

  public:
    text_video_raster_iterator &operator++();

    text_video_raster_iterator operator++(int)
    {
      text_video_raster_iterator tmp = *this;
      ++(*this);
      return tmp;
    }
  };

  /* Text VRAM.  */
  class text_video_memory: public memory
  {
  public:
    typedef text_video_raster_iterator raster_iterator;

  private:
    unsigned char *buf;
    console *connected_console;

    /* True if any update on a raster is pending.  */
    vector<bool> raster_update_marks;

    pthread_mutex_t mutex;

  public:
    text_video_memory();
    ~text_video_memory();

  public:
    uint16_type get_16(function_code, uint32_type) const;
    int get_8(function_code, uint32_type) const;

    void put_16(function_code, uint32_type, uint16_type);
    void put_8(function_code, uint32_type, int);

  public:
    /* Installs IOCS calls on the text video memory.  */
    void install_iocs_calls(system_rom &);

  public:
    /* Draw a character CODE at [X Y].  */
    void draw_char(int x, int y, unsigned int code);

    /* Scroll one line up.  */
    void scroll();

    /* Fills a plane.  */
    void fill_plane(int left, int top, int right, int bottom,
		    int plane, uint16_type pattern);

  public:
    void connect(console *);

    /* Returns truth vector once if any update is pending.  This
       function may be called in a separate thread.  */
    vector<bool> poll_update();

    /* Returns true once when the row is changed.  */
    bool row_changed(unsigned int);

    /* Get the visual image of this text VRAM.  Image is of size
       [WIDTH HEIGHT] at position [X Y].  RGB_BUF is an array of
       bytes.  ROW_SIZE is the row size of RGB_BUF.  */
    void get_image(int x, int y, int width, int height,
		   unsigned char *rgb_buf, size_t row_size);

    /* Returns an iterator for a raster.  This function may be called
       in a separate thread.  */
    raster_iterator raster(unsigned int, unsigned int);

  protected:
    /* Marks an update area.  */
    void mark_update_area(unsigned int left_x, unsigned int top_y,
			  unsigned int right_x, unsigned int bottom_y);
  };

  /* CRTC input/output port memory.  This object also generates VDISP
     interrupts.  */
  class crtc_memory: public memory
  {
  private:
    /* Time interval between VDISP interrupts in milliseconds.  */
    console::time_type vdisp_interval;

    /* Start time of the current interval for the next VDISP
       interrupt.  */
    console::time_type vdisp_start_time;

    /* Reload value of the VDISP counter.  If this value is zero,
       VDISP interrupts are disabled.  */
    unsigned int vdisp_counter_data;

    /* Current value of the VDISP counter.  */
    unsigned int vdisp_counter_value;

    /* Mutex for this object.  */
    pthread_mutex_t mutex;

  public:
    crtc_memory();
    ~crtc_memory();

  public:
    /* Reads data from this object.  */
    uint16_type get_16(function_code, uint32_type) const;
    int get_8(function_code, uint32_type) const;

    /* Writes data to this object.  */
    void put_16(function_code, uint32_type, uint16_type);
    void put_8(function_code, uint32_type, int);

  public:
    /* Returns true if VDISP interrupts are enabled.  */
    bool vdisp_interrupt_enabled() const {return vdisp_counter_data != 0;}

    /* Set the reload value of the VDISP counter.  If the value is
       zero, VDISP interrupts are disabled.  */
    void set_vdisp_counter_data(unsigned int);

  public:
    /* Resets internal timestamps.  */
    void reset(console::time_type t);

    /* Checks timeouts for interrupts.  This function may be called in
       a separate thread.  */
    void check_timeouts(console::time_type t, context &c);
  };

  /* Palettes and video controller registers memory.  This memory is
     mapped to the address range from 0xe82000 to 0xe84000.  */
  class palettes_memory: public memory
  {
    vector<unsigned short> _tpalette;
    bool text_colors_modified;

    pthread_mutex_t mutex;

  public:
    palettes_memory();
    ~palettes_memory();

  public:
    /* Reads data from this object.  */
    uint16_type get_16(function_code, uint32_type) const;
    int get_8(function_code, uint32_type) const;

    /* Writes data to this object.  */
    void put_16(function_code, uint32_type, uint16_type);
    void put_8(function_code, uint32_type, int);

  public:
    /* Checks if text colors are modified.  This function may be
       called in a separate thread.  */
    bool check_text_colors_modified();

    /* Retrieves text colors.  This function may be called in a
       separate thread.  */
    void get_text_colors(unsigned int i, unsigned int j, unsigned char *out);
    void get_text_colors(unsigned int i, unsigned int j, unsigned short *out);
  };

  /* Memory of DMAC input/output ports.  This memory is mapped to the
     address range from 0xe84000 to 0xe86000 on X68000.  */
  class dmac_memory: public memory
  {
  public:
    dmac_memory();
    ~dmac_memory();

  public:
    /* Reads data from this object.  */
    uint16_type get_16(function_code, uint32_type) const;
    int get_8(function_code, uint32_type) const;

    /* Writes data to this object.  */
    void put_16(function_code, uint32_type, uint16_type);
    void put_8(function_code, uint32_type, int);

  public:
    /* Installs IOCS calls on the DMAC.  */
    void install_iocs_calls(system_rom &);
  };

  /* Area set register.  This object is mapped from 0xe86000 to
     0xe88000.  This object has only the area set register, which is
     at 0xe86001(b).  */
  class area_set: public memory
  {
  private:
    /* Reference to the main memory.  */
    main_memory *_mm;

  public:
    explicit area_set(main_memory *);

  public:
    /* Reads data from this object.  */
    uint16_type get_16(function_code, uint32_type) const;
    int get_8(function_code, uint32_type) const;

    /* Writes data to this object.  */
    void put_16(function_code, uint32_type, uint16_type);
    void put_8(function_code, uint32_type, int);
  };

  /* MFP input/output port memory.  This memory is mapped to the
     address range from 0xe88000 to 0xe8a000 on X68000.  */
  class mfp_memory: public memory
  {
  public:
    /* Reads data from this object.  */
    uint16_type get_16(function_code, uint32_type) const;
    int get_8(function_code, uint32_type) const;

    /* Writes data to this object.  */
    void put_16(function_code, uint32_type, uint16_type);
    void put_8(function_code, uint32_type, int);
  };

  /* System ports memory.  This memory is mapped to the address range
     from 0xe8e000 to 0xe90000 on X68000.  */
  class system_ports_memory: public memory
  {
  public:
    /* Reads data from this object.  */
    uint16_type get_16(function_code, uint32_type) const;
    int get_8(function_code, uint32_type) const;

    /* Writes data to this object.  */
    void put_16(function_code, uint32_type, uint16_type);
    void put_8(function_code, uint32_type, int);
  };

  /* OPM input/output port memory.  */
  class opm_memory: public memory
  {
  private:
    int _status;
    vector<unsigned char> _regs;
    bool _interrupt_enabled;

    int reg_index;

    console::time_type last_check_time;

    /* Intervals for timers A and B.  */
    console::time_type timer_a_interval, timer_b_interval;

    /* Reset times for timers A and B.  */
    console::time_type timer_a_start_time, timer_b_start_time;

    pthread_mutex_t mutex;

  public:
    opm_memory();
    ~opm_memory();

  public:
    /* Reads data from this object.  */
    uint16_type get_16(function_code, uint32_type) const;
    int get_8(function_code, uint32_type) const;

    /* Writes data to this object.  */
    void put_16(function_code, uint32_type, uint16_type);
    void put_8(function_code, uint32_type, int);

  public:
    int status() const {return _status;}
    void set_reg(int, int);
    bool interrupt_enabled() const {return _interrupt_enabled;}
    void set_interrupt_enabled(bool);

  public:
    /* Resets times.  */
    void reset(console::time_type t);

    /* Checks timeouts for interrupts.  This function may be called in
       a separate thread.  */
    void check_timeouts(console::time_type t, context &c);
  };

  /* Input/output memory for the MSM6258V ADPCM chip.  This memory is
     mapped to the address range from 0xe92000 to 0xe94000 on X68000.  */
  class msm6258v_memory: public memory
  {
  public:
    /* Reads data from this object.  */
    uint16_type get_16(function_code, uint32_type address) const;
    int get_8(function_code, uint32_type address) const;

    /* Writes data to this object.  */
    void put_16(function_code, uint32_type address, uint16_type value);
    void put_8(function_code, uint32_type address, int value);

  public:
    /* Installs IOCS calls on this ADPCM chip.  */
    void install_iocs_calls(system_rom &);
  };

  /* Input/output memory for the FDC.  This memory is mapped to the
     address range from 0xe94000 to 0xe96000 on X68000.  */
  class fdc_memory: public memory
  {
  public:
    /* Reads data from this object.  */
    uint16_type get_16(function_code, uint32_type address) const;
    int get_8(function_code, uint32_type address) const;

    /* Writes data to this object.  */
    void put_16(function_code, uint32_type address, uint16_type value);
    void put_8(function_code, uint32_type address, int value);

  public:
    /* Installs IOCS calls on this ADPCM chip.  */
    void install_iocs_calls(system_rom &);
  };

  /* Memory for SCC input/output.  This memory also manages mouse
     input.  This memory is mapped to the address range from 0xe98000
     to 0xe9a000 on X68000.  */
  class scc_memory: public memory
  {
  public:
    struct point
    {
      int x, y;
    };

  private:
    /* Mouse bounds.  */
    int mouse_left, mouse_top, mouse_right, mouse_bottom;

    /* Button states of the mouse.  */
    vector<bool> mouse_states;

    /* Most recent X and Y position of the mouse.  */
    point _mouse_position;

    /* Second most recent X and Y position of the mouse.  */
    point old_mouse_position;

    /* Mouse motion vector.  */
    point _mouse_motion;

    /* Mutex for this object.  */
    mutable pthread_mutex_t mutex;

  public:
    scc_memory();
    ~scc_memory();

  public:
    /* Reads data from this object.  */
    uint16_type get_16(function_code, uint32_type) const;
    int get_8(function_code, uint32_type) const;

    /* Writes data to this object.  */
    void put_16(function_code, uint32_type, uint16_type);
    void put_8(function_code, uint32_type, int);

  public:
    /* Installs IOCS calls on the first COM port and the mouse.  */
    void install_iocs_calls(system_rom &);

  public:
    /* Initializes mouse states.  */
    void initialize_mouse();

    /* Sets mouse bounds.  */
    void set_mouse_bounds(int l, int t, int r, int b);

  public:
    /* Returns the state of a mouse button.  */
    bool mouse_state(unsigned int button) const;

    /* Sets the state of a mouse button.  */
    void set_mouse_state(unsigned int button, bool state);

    /* Return the mouse position.  */
    point mouse_position() const;

    /* Sets the mouse position.  */
    void set_mouse_position(int x, int y);

    /* Returns the mouse motion.  */
    point mouse_motion() const;

    /* Tracks the mouse motion.  */
    void track_mouse();
  };

  /* PPI (a.k.a 8255A) registers memory.  On X68000, a PPI is used for
     joypad interface and ADPCM contorl.  */
  class ppi_memory: public memory
  {
  public:
    /* Reads data from this object.  */
    uint16_type get_16(function_code, uint32_type) const;
    int get_8(function_code, uint32_type) const;

    /* Writes data to this object.  */
    void put_16(function_code, uint32_type, uint16_type);
    void put_8(function_code, uint32_type, int);
  };

  /* Sprite controller memory.  */
  class sprites_memory: public memory
  {
  public:
    /* Reads data from this object.  */
    uint16_type get_16(function_code, uint32_type) const;
    int get_8(function_code, uint32_type) const;

    /* Writes data to this object.  */
    void put_16(function_code, uint32_type, uint16_type);
    void put_8(function_code, uint32_type, int);
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
    uint16_type get_16(function_code, uint32_type) const;
    int get_8(function_code, uint32_type) const;

    /* Writes data to this object.  */
    void put_16(function_code, uint32_type, uint16_type);
    void put_8(function_code, uint32_type, int);
  };

  /* Font ROM.  */
  class font_rom: public memory
  {
  public:
    /* Returns the address offset of the 16-pixel font data for a JIS
       X 0201 character.  */
    static uint32_type jisx0201_16_offset(unsigned int ch);

    /* Returns the address offset of the 24-pixel font data for a JIS
       X 0201 character.  */
    static uint32_type jisx0201_24_offset(unsigned int ch);

    /* Returns the address offset of the 16-pixel font data for a JIS
       X 0208 character.  */
    static uint32_type jisx0208_16_offset(unsigned int ch1, unsigned int ch2);

    /* Returns the address offset of the 24-pixel font data for a JIS
       X 0208 character.  */
    static uint32_type jisx0208_24_offset(unsigned int ch1, unsigned int ch2);

  private:
    unsigned char *data;

  public:
    font_rom();
    ~font_rom();
    
  public:
    /* Reads data from this object.  */
    uint16_type get_16(function_code, uint32_type) const;
    int get_8(function_code, uint32_type) const;

    /* Writes data to this object.  */
    void put_16(function_code, uint32_type, uint16_type);
    void put_8(function_code, uint32_type, int);

  public:
    /* Installs IOCS calls on the system font.  */
    void install_iocs_calls(system_rom &);

    void copy_data(const console *);
  };
} // namespace vx68k

#endif /* not _VX68K_MEMORY_H */
