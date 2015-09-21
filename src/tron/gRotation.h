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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
  
***************************************************************************

*/

//! @file
//! @brief contains the map rotator
//!
//! This might get moved to src/ui, even though parts have to stay in
//! the higher regions of the sources.

#include "tXmlParser.h"

#ifndef ARMAGETRON_COCKPIT_H
#define ARMAGETRON_COCKPIT_H

class gRotationEvent {
public:
    virtual void Print() = 0; //virtual function, more or less just to ensure that polymorphism works...
};

class gRotationTag;

//! Rotator main class: Handles the root element and takes care of sending events
class gRotation : private tXmlResource {
    gRotationTag *m_mainRotation;

    gRotation();
public:
    void Parse();
    static void HandleNewRound();
    static void HandleNewMatch();
    static gRotation &GetRotator();

    ~gRotation();
};

#endif
