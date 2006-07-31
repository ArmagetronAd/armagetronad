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
//! @brief Contains the class for rendering a rectangle as part of the cockpit
//!
//! This file is a candidate for being moved to src/ui.

#ifndef ARMAGETRON_CRECTANGLE_H
#define ARMAGETRON_CRECTANGLE_H

#include "cockpit/cWidgetBase.h"

#ifndef DEDICATED

namespace cWidget {

//! Processes and renders a simple rectangle with foreground and background gradients
class Rectangle : public WithSingleData, public WithBackground, public WithForeground, public WithCoordinates {
public:
    virtual ~Rectangle() { }; //!< Do- nothing destructor

    void Render(); //!< Renders the rectangle
    virtual bool Process(tXmlParser::node cur); //!< Passes on to all Process() functions of the base classes and calls Base::DisplayError() on failure
};

}

#endif
#endif
