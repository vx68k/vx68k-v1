/* vx68k - Virtual X68000
   Copyright (C) 1998, 1999 Hypercore Software Design, Ltd.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307, USA.  */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#undef const
#undef inline

#include <vx68k/human.h>

#include <sys/stat.h>
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <cstring>
#include <cerrno>

#ifdef HAVE_NANA_H
# include <nana.h>
# include <cstdio>
#else
# include <cassert>
# define I assert
#endif

using namespace vx68k::human;
using namespace vx68k;
using namespace std;

namespace
{
} // (unnamed namespace)

sint32_type
regular_file::read(memory_address_space *as,
		   uint32_type dataptr, uint32_type size)
{
  // FIXME.
  unsigned char *data = new unsigned char [size];

  ssize_t result = ::read(fd, data, size);
  if (result == -1)
    {
      delete [] data;
      return -6;			// FIXME.
    }

  as->write(SUPER_DATA, dataptr, data, result);
  delete [] data;
  return result;
}

sint32_type
regular_file::write(const memory_address_space *as, uint32_type dataptr,
		    uint32_type size)
{
  // FIXME.
  unsigned char *data = new unsigned char [size];
  as->read(SUPER_DATA, dataptr, data, size);

  ssize_t result = ::write(fd, data, size);
  if (result == -1)
    {
      delete [] data;
      return -6;			// FIXME.
    }

  delete [] data;
  return result;
}

sint_type
regular_file::fgetc()
{
  unsigned char data[1];
  ssize_t result = ::read(fd, data, 1);
  if (result == -1)
    return -6;			// FIXME.

  return data[0];
}

sint_type
regular_file::fputc(sint_type code)
{
  // FIXME.
  unsigned char data[1];
  data[0] = code;
  ::write(fd, data, 1);

  return 1;
}

sint32_type
regular_file::fputs(const memory_address_space *as, uint32_type mesptr)
{
  string mes = as->gets(SUPER_DATA, mesptr);

  ssize_t written_size = ::write(fd, mes.data(), mes.size());
  if (written_size == -1)
    switch (errno)
      {
      default:
	return -6;		// FIXME
      }

  return written_size;
}

sint32_type
regular_file::seek(sint32_type offset, uint_type mode)
{
  // FIXME.
  sint32_type pos = lseek(fd, offset, mode);
  if (pos == -1)
    return -6;			// FIXME.

  return pos;
}

regular_file::~regular_file()
{
  close(fd);
}

regular_file::regular_file(int f)
  : fd(f)
{
}

namespace
{
  class con_device_file
    : public file
  {
  private:
    machine *_m;

  public:
    explicit con_device_file(machine *m);

  public:
    sint32_type read(memory_address_space *, uint32_type, uint32_type);
    sint32_type write(const memory_address_space *, uint32_type, uint32_type);
    sint_type fgetc();
    sint_type fputc(sint_type);
    sint32_type fputs(const memory_address_space *, uint32_type);
  };
} // (unnamed namespace)

sint32_type
con_device_file::read(memory_address_space *, uint32_type, uint32_type)
{
  // FIXME not implemented
  return -1;
}

sint32_type
con_device_file::write(const memory_address_space *as,
		       uint32_type dataptr, uint32_type size)
{
  // FIXME.
  unsigned char *data = new unsigned char [size];
  as->read(SUPER_DATA, dataptr, data, size);

  for (unsigned char *i = data + 0;
       i != data + size;
       ++i)
    {
      _m->b_putc(*i);
    }

  delete [] data;
  return size;
}

sint_type
con_device_file::fgetc()
{
  /* FIXME.  This function must be implemented via class machine.  */
  unsigned char c[1];
  ::read(STDIN_FILENO, c, 1);
  return 1;
}

sint_type
con_device_file::fputc(sint_type code)
{
  _m->b_putc(code);
  return 1;
}

sint32_type
con_device_file::fputs(const memory_address_space *as, uint32_type str)
{
  _m->b_print(as, str);
  return 0;			// FIXME
}

con_device_file::con_device_file(machine *m)
  : _m(m)
{
}

string
file_system::export_file_name(const string &dos_name)
{
  // FIXME.  Kanji must be handled.
  string name;
  string::size_type pos = dos_name.find_first_not_of(' ');

  if (pos != string::npos && dos_name.at(pos) == '\\')
    {
      name.append(1, '/');
      ++pos;
    }

  for (;;)
    {
      if (pos == string::npos || pos == dos_name.size())
	break;

      string::size_type next_pos = dos_name.find_first_of(".\\/", pos);
      if (next_pos == string::npos)
	break;

      if (dos_name.at(next_pos) == '/')
	name.append(dos_name.substr(pos, next_pos + 1 - pos));
      else
	{
	  string::size_type end = next_pos - 1;
	  if (next_pos != pos)
	    {
	      end = dos_name.find_last_not_of(' ', end);
	      if (end == string::npos || end < pos - 1)
		end = next_pos - 1;
	    }
	  name.append(dos_name.substr(pos, end + 1 - pos));
	  char c = dos_name.at(next_pos);
	  if (c == '\\')
	    c = '/';
	  name.append(1, c);
	}

      pos = next_pos + 1;
    }

  string::size_type end = dos_name.size() - 1;
  if (dos_name.size() != pos)
    {
      end = dos_name.find_last_not_of(' ');
      if (end == string::npos || end < pos - 1)
	end = dos_name.size() - 1;
    }
  name.append(dos_name.substr(pos, end + 1 - pos));

  return name;
}

void
file_system::unref(file *f)
{
  if (f == NULL)
    return;

  map<file *, int>::iterator found = files.find(f);
  I(found != files.end());

  I(found->second > 0);
  --(found->second);
  if (found->second == 0)
    {
      files.erase(found);
      delete f;
    }
}

file *
file_system::ref(file *f)
{
  if (f == NULL)
    return NULL;

  map<file *, int>::iterator found = files.find(f);
  I(found != files.end());

  ++(found->second);
  I(found->second > 0);
  return found->first;
}

void
file_system::open(file *&ret, int fd)
{
  regular_file *f = new regular_file(fd);
  files.insert(make_pair(f, 1));
  ret = f;
}

sint_type
file_system::open(file *&ret, const string &name, uint_type mode)
{
  if ((mode & 0xf) > 2)
    return -12;			// FIXME.

  if (strcasecmp(name.c_str(), "con") == 0) // FIXME
    {
      con_device_file *f = new con_device_file(_m);
      files.insert(make_pair(f, 1));
      ret = f;
    }
  else
    {
      string ext_name = export_file_name(name);

      // FIXME.
      static const int uflag[] = {O_RDONLY, O_WRONLY, O_RDWR};

      int fd = ::open(ext_name.c_str(), uflag[mode & 0xf]);
      if (fd == -1)
	switch (errno)
	  {
#ifdef EPERM
	  case EPERM:
	    return -19;
#endif
#ifdef ENOENT
	  case ENOENT:
	    return -2;
#endif
	  default:
	    return -2;		// FIXME: errno test.
	  }

      open(ret, fd);
    }

  return 0;
}

sint_type
file_system::open(file *&ret, const memory_address_space *as,
		  uint32_type nameptr,
		  sint_type mode)
{
  string name = as->gets(SUPER_DATA, nameptr);

  return open(ret, name, mode);
}

sint_type
file_system::create(file *&ret, const memory_address_space *as,
		    uint32_type nameptr, sint_type atr)
{
  string name = export_file_name(as->gets(SUPER_DATA, nameptr));

  // FIXME.
  int fd = ::open(name.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
  if (fd == -1)
    switch (errno)
      {
#ifdef EPERM
      case EPERM:
	return -19;
#endif
      default:
	return -2;		// FIXME: errno test.
      }

  open(ret, fd);
  return 0;
}

sint_type
file_system::chmod(const memory_address_space *as,
		   uint32_type nameptr, sint_type atr)
{
  string name = export_file_name(as->gets(SUPER_DATA, nameptr));

  struct stat stbuf;
  if (stat(name.c_str(), &stbuf) == -1)
    switch (errno)
      {
#ifdef EPERM
      case EPERM:
	return -19;
#endif
#ifdef ENOENT
      case ENOENT:
	return -2;
#endif
      default:
	return -2;		// FIXME: errno test.
      }

  // FIXME.
  sint_type a = 0;
  if (S_ISDIR(stbuf.st_mode))
    a |= 0x10;
  else
    a |= 0x20;

  if (access(name.c_str(), W_OK) == -1)
    a |= 0x01;

  return a;
}

file_system::file_system(machine *m)
  : _m(m)
{
}

