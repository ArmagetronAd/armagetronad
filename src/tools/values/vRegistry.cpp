/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2000  Manuel Moos (manuel@moosnet.de)

**************************************************************************

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

***************************************************************************

*/

#include "vCore.h"
#include "vRegistry.h"

namespace vValue {
namespace Registry {

Registration::Registration(std::vector<tString> flags, tString fname, int argc, fptr ctor):
        m_flags(flags),
        m_fname(fname),
        m_argc(argc),
        m_ctor(ctor)
{
    Registry::theRegistry().reg(this);
}

Registration::Registration(const char *flags, const char *fname, int argc, fptr ctor):
        m_fname(fname),
        m_argc(argc),
        m_ctor(ctor)
{
    m_flags.clear();
    for (
        const char *ptr = flags, *ep;
        ptr;
        ptr = (ep[0] == '\0') ? 0 : (ep + 1)
    )
    {
        ep = strchr(ptr, '\n');
        if (!ep)
            ep = ptr + strlen(ptr);
        std::string s(ptr, ep - ptr);
        m_flags.push_back(s);
    }
    Registry::theRegistry().reg(this);
}

Expr::
Base *
Registration::use(arglist args) {
    switch (args.size()) {
    case 0:	return ((ctor::a0*)m_ctor)();
    case 1:	return ((ctor::a1*)m_ctor)(args[0]);
    case 2:	return ((ctor::a2*)m_ctor)(args[0], args[1]);
    case 3:	return ((ctor::a3*)m_ctor)(args[0], args[1], args[2]);
    case 4:	return ((ctor::a4*)m_ctor)(args[0], args[1], args[2], args[3]);
    case 5:	return ((ctor::a5*)m_ctor)(args[0], args[1], args[2], args[3], args[4]);
    }
    throw("Unimplemented # of args");
}

bool
Registration::match(std::vector<tString> flags, tString fname, int argc)
{
    if (fname.Compare(m_fname, true))
        return false;
    for (std::vector<tString>::iterator i = flags.begin(); i != flags.end(); ++i)
    {
        bool flagmatch = (flags[0] == "!");
        tString flag = *i;
        if (flagmatch)
            flag = flag.SubStr(1);
        if (!flag.Compare(*i, true))
            flagmatch = !flagmatch;
        if (!flagmatch)
            return false;
    }
    if (argc != m_argc)
        return false;
    return true;
}

void
Registry::reg(Registration *it)
{
    registrations.push_back(it);
}

Expr::
Base *
Registry::create(std::vector<tString> flags, tString fname, arglist args)
{
    for (std::vector<Registration *>::iterator i = registrations.begin(); i != registrations.end(); ++i)
        if ((*i)->match(flags, fname, args.size()))
            return (*i)->use(args);
    return NULL;
}

Registry & Registry::theRegistry()
{
    static Registry theRegistry;
    return theRegistry;
}

}
}
