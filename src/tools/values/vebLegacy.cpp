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

#include "tConfiguration.h"
#include "tError.h"

#include "vCore.h"
#include "vebLegacy.h"

namespace vValue {
namespace Expr {
namespace Bindings {
namespace Legacy {

//! Reads from the Configuration item
//! @returns the result as a string
tString ConfItem::Read() const {
    tASSERT(m_value);
    std::ostringstream ret("");
    m_value->WriteVal(ret);
    return(ret.str());
}

//! Searches the right configuration item and uses it if it finds it
//! @param value the name of the configuration item to be searched for
ConfItem::ConfItem(tString const &value):
        Base(),
        m_value(tConfItemBase::FindConfigItem(value))
{ }

//! Initializes the ConfItem object using the information from another one
//! @param other another ConfItem object
ConfItem::ConfItem(ConfItem const &other):
        Base(other),
        m_value(other.m_value)
{ }

//! Overwritten copy function
//! @returns a new copy of the current object
Base *ConfItem::copy(void) const {
    return new ConfItem(*this);
}

//! Does nothing different than the default assignment operator, but gets rid of a compiler warning
//! @param other the ConfItem to be copied
void ConfItem::operator=(ConfItem const &other) {
    this->Base::operator=(other);
    m_value=other.m_value;
}

//! Tests if the search for the right configuration item at construction time was successful
//! @returns true on success, false on failure (read operations will segfault)
bool ConfItem::Good() const {
    return m_value != 0;
}

Variant ConfItem::GetValue() const {
    return GetString();
}

//! Reads from the configuration item
//! @returns the result as a string
tString ConfItem::GetString(Base const *other) const {
    return Output(Read(), other);
}

}
}
}
}
