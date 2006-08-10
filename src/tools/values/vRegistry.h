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

//! @file
//! @brief Contains the class declarations for the vValue registry

#ifndef ARMAGETRON_vRegistry_h
#define ARMAGETRON_vRegistry_h

#include <vector>

#include "values/vCore.h"

namespace vValue {
namespace Registry {
namespace ctor {
typedef Expr::Base *a0();
typedef Expr::Base *a1(BasePtr);
typedef Expr::Base *a2(BasePtr, BasePtr);
typedef Expr::Base *a3(BasePtr, BasePtr, BasePtr);
typedef Expr::Base *a4(BasePtr, BasePtr, BasePtr, BasePtr);
typedef Expr::Base *a5(BasePtr, BasePtr, BasePtr, BasePtr, BasePtr);
}

class Registration {
public:
    typedef void (*fptr)();
private:
    std::vector<tString> m_flags;
    tString m_fname;
    int m_argc;
    fptr m_ctor;
public:
    Registration(std::vector<tString> flags, tString fname, int argc, fptr ctor);
    Registration(const char *flags, const char *fname, int argc, fptr ctor);
    Expr::
    Base *use(arglist);
    bool match(std::vector<tString> flags, tString fname, int argc);
};

class Registry {
    std::vector<Registration *> registrations;
public:
    void reg(Registration *me);
    Expr::
    Base *create(std::vector<tString> flags, tString fname, arglist);
};

extern Registry theRegistry;

}
}

#endif
