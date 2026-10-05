/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2000  Manuel Moos (manuel@moosnet.de)
Copyright (C) 2004  Armagetron Advanced Team (http://sourceforge.net/projects/armagetronad/)

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

***************************************************************************

*/

#ifndef ArmageTron_tDefer_H
#define ArmageTron_tDefer_H

#include <utility>

// executes a function when it goes out of scope
template <typename F>
class tDeferrer
{
public:
    tDeferrer(F&& f) : f_{std::forward<F>(f)}
    {
    }
    ~tDeferrer() { std::move(f_)(); }

private:
    F f_;
};

template <typename F>
// [[nodiscard]] // not yet
tDeferrer<F> tDefer(F&& f)
{
    return tDeferrer<F>{std::forward<F>(f)};
}

#define MERGE_(a, b) a##b
#define LABEL_(a) MERGE_(unique_name_, a)
#define UNIQUE_LABEL LABEL_(__LINE__)

// for tests: executes CODE once when the macro is used, and once again when the current scope ends
// CODE needs to end with a semicolon
#define INVARIANT(CODE) \
    {                   \
        CODE            \
    }                   \
    auto const UNIQUE_LABEL = tDefer([&]() { CODE });

// for tests: check condition now and when the scope is exited
#define INVARIANT_CHECK(CONDITION) INVARIANT(CHECK(CONDITION);)

#endif
