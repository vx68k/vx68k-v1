/* Virtual X68000 - X68000 virtual machine
   Copyright (C) 1998-2001 Hypercore Software Design, Ltd.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA.  */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#undef const			// C++ must have `const'.

#define _GNU_SOURCE 1
#define _POSIX_C_SOURCE 199506L // We want POSIX.1c if not GNU.

#include "gtk_gl.h"

#include <gdk/gdkx.h>
#include <GL/glx.h>
#include <stdexcept>

namespace vx68k
{
  namespace gtk
  {
    using namespace std;

    namespace
    {
      int glx_error_base;
      int glx_event_base;
      int glx_major_version;
      int glx_minor_version;
    }

    void
    gl::initialize()
    {
      static bool initialized;
      if (!initialized)
	{
	  Display *d = GDK_DISPLAY();

	  if (!glXQueryExtension(d, &glx_error_base, &glx_event_base))
	    throw runtime_error("glXQueryExtension");
	  if (!glXQueryVersion(d, &glx_major_version, &glx_minor_version))
	    throw runtime_error("glXQueryVersion");

	  initialized = true;
	}
    }

    GdkVisual *
    gl::best_visual()
    {
      static GdkVisual *bv;
      if (bv == 0)
	{
	  initialize();

	  Display *d = GDK_DISPLAY();

	  int glx_attr[]
	    = {GLX_RGBA,
	       GLX_DOUBLEBUFFER,
	       GLX_RED_SIZE, 6,
	       GLX_GREEN_SIZE, 6,
	       GLX_BLUE_SIZE, 6,
	       None};
	  XVisualInfo *xvi = glXChooseVisual(d, DefaultScreen(d), glx_attr);
	  if (xvi == NULL)
	    throw runtime_error("glXChooseVisual");

	  bv = gdkx_visual_get(xvi->visualid);

	  XFree(xvi);
	}

      return bv;
    }

    gl_context
    gl::create_context(GdkVisual *v)
    {
      Display *d = GDK_DISPLAY();

      XVisualInfo xvit;
      xvit.visualid = XVisualIDFromVisual(GDK_VISUAL_XVISUAL(v));
      int n;
      XVisualInfo *xvi = XGetVisualInfo(d, VisualIDMask, &xvit, &n);
      if (xvi == 0)
	throw runtime_error("XGetVisualInfo");

      GLXContext xc = glXCreateContext(d, &xvi[0], 0, true);

      XFree(xvi);

      return reinterpret_cast<gl_context>(xc);
    }

    void
    gl::destroy_context(gl_context c)
    {
      if (c != 0)
	{
	  Display *d = GDK_DISPLAY();
	  glXDestroyContext(d, reinterpret_cast<GLXContext>(c));
	}
    }

    void
    gl::make_current(gl_context c, GdkDrawable *d)
    {
      Display *dd;
      Drawable dw;
      if (c == 0)
	{
	  dd = GDK_DISPLAY();
	  dw = None;
	}
      else if (d == 0)
	{
	  dd = GDK_DISPLAY();
	  dw = GDK_ROOT_WINDOW();
	}
      else
	{
	  dd = GDK_WINDOW_XDISPLAY(d);
	  dw = GDK_WINDOW_XWINDOW(d);
	}

      if (!glXMakeCurrent(dd, dw, reinterpret_cast<GLXContext>(c)))
	throw runtime_error("glXMakeCurrent");
    }
  }
}
