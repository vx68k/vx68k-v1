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

#include <string>

#ifdef HAVE_NANA_H
# include <nana.h>
# include <cstdio>
#else
# include <cassert>
# define I assert
#endif

using namespace vx68k::human;
using namespace vm68k;
using namespace std;

uint32_type
shell::create_env(const char *const *envp)
{
  // FIXME

  const size_t ENV_SIZE = 512;
  uint32_type env = _context->malloc(ENV_SIZE);
  _context->mem->putl(SUPER_DATA, env, ENV_SIZE);

  return env;
}

int
shell::exec(const char *name, const char *const *argv,
	    const char *const *envp)
{
  string arg;
  const char *const *p = argv;
  if (*p != NULL)
    {
      arg.append(*p++);
      while (*p != NULL)
	{
	  arg.append(" ");
	  arg.append(*p++);
	}
    }

  // Environment
  uint32_type env = create_env(envp);

  // Argument string
  uint32_type pdb_base = pdb - 0x10;
  _context->mem->putb(SUPER_DATA, pdb_base + 0x100, arg.size());
  _context->mem->write(SUPER_DATA, pdb_base + 0x101, arg.c_str(), arg.size() + 1);

  _context->regs.a[7] = pdb + 0x2000;
  uint32_type child_pdb = _context->load(name, pdb_base + 0x100, env);
  _context->setpdb(child_pdb);
  sint_type status = _context->start(_context->regs.a[4], argv);

  return status;
}

shell::~shell()
{
  uint32_type pdb_base = pdb - 0x10;
  uint32_type parent = _context->mem->getl(SUPER_DATA, pdb_base + 4) + 0x10;
  _context->setpdb(parent);
  _context->mfree(pdb);
}

shell::shell(dos_exec_context *c)
  : _context(c),
    pdb(0)
{
  pdb = _context->malloc(0x2000);

  uint32_type pdb_base = pdb - 0x10;
  _context->mem->putl(SUPER_DATA, pdb_base + 0x10, 0);

  _context->setpdb(pdb);
}

