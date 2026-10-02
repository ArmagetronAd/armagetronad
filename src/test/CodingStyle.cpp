/*
Armagetron Advanced -- a Tron clone in 3D
Copyright (C) 2000  Manuel Moos (manuel@moosnet.de)

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
*/

// This file demonstrates the *ideal* coding style (hah!)
// and, on the other end, demonstrates some idiosyncrasies of the code base.

// Comments starting with `FYI` in them are meant as comments just to describe
// what is going on HERE, they are not meant as templates to include in actual code.

// FYI: own include comes first, so we know it works standalone
#include "CodingStyle.h"

#include "doctest.h"

#include "defs.h"

/// @brief The number of test reference counted objects in exsitence
cCounter cReferenceCounted::s_numberOfObjects_{};

// FYI you can put doxygen structured commands at the end of .cpp files if, for
// one reason or another, you don't want them near the definitions.
// Disadvantage: Code editors usually can't pick them up and they
// do not appear in autocomplete. And they're harder to write. Advice: Stick to
// documentation next to the definition.

// *** cCounter ***

/// @fn cCounter::GetCount()
/// @brief returns the current counter value
/// @return the value

/// @fn cCounter::CountUp()
/// @brief increase the counter

// *** cDeepCopy ***

/// @fn cDeepCopy::GetTarget()
/// @brief returns the wrapped object

/// @fn cDeepCopy::cDeepCopy(cDeepCopy const &that)
/// @brief copy constructor
/// @param that the source of the copy
