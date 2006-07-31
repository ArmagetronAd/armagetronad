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
//! @brief Contains the label widget
//!
//! This file is a candidate for being moved to src/ui.

#ifndef ARMAGETRON_CLABEL_H
#define ARMAGETRON_CLABEL_H

#include "cockpit/cWidgetBase.h"

#ifndef DEDICATED

namespace cWidget {

//! Processes and renders a Table of values with an optional caption
class Label : public WithCoordinates, public WithCaption, public WithTable {
public:
    virtual ~Label() { }; //!< Do- nothing destructor

    void Render(); //!< Renders the label and the caption
    bool Process(tXmlParser::node cur); //!< Passes on to all Process() functions of the base classes and calls Base::DisplayError() on failure
};

}

#endif
#endif
