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
	  if (!glXQueryExtension(GDK_DISPLAY(),
				 &glx_error_base, &glx_event_base))
	    throw runtime_error("glXQueryExtension");
	  if (!glXQueryVersion(GDK_DISPLAY(),
			       &glx_major_version, &glx_minor_version))
	    throw runtime_error("glXQueryVersion");

	  initialized = true;
	}
    }

    GdkVisual *
    gl::best_visual()
    {
      initialize();

      int glx_attr[]
	= {GLX_RGBA,
	   GLX_DOUBLEBUFFER,
	   GLX_RED_SIZE, 6,
	   GLX_GREEN_SIZE, 6,
	   GLX_BLUE_SIZE, 6,
	   GLX_ALPHA_SIZE, 4,
	   None};
      Display *d = GDK_DISPLAY();
      XVisualInfo *xvi = glXChooseVisual(d, DefaultScreen(d), glx_attr);
      GdkVisual *v = gdkx_visual_get(xvi->visualid);
      XFree(xvi);
      return v;
    }
  }
}
