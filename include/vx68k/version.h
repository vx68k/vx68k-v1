/* -*-C++-*- */
/* vx68k - Virtual X68000
   Copyright (C) 2000 Hypercore Software Design, Ltd.

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

#ifndef _VX68K_VERSION_H
#define _VX68K_VERSION_H 1

namespace vx68k
{
  /* Returns the library version.  */
  const char *library_version() throw ();
} // namespace vx68k

#endif /* not _VX68K_VERSION_H */

