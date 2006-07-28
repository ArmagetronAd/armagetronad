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
//! @brief Contains the class for rendering the HUD map
//!
//! This file has to stay in src/tron/cockpit or src/tron since
//! it uses zZone for rendering the zones.

#ifndef ARMAGETRON_CMAP_H
#define ARMAGETRON_CMAP_H

#include "cockpit/cWidgetBase.h"

#ifndef DEDICATED

#include "tList.h"
#include "gWall.h"
#include "ePlayer.h"
#include <deque>

class zZone;

namespace cWidget {

//! Processes and renders a map of the grid
class Map : public WithCoordinates {
    static void DrawRimWalls( tList<eWallRim> &list ); //!< Draws all the rim walls
    void DrawWalls(tList<gNetPlayerWall> &list); //!< Draws all player walls
    void DrawCycles(tList<ePlayerNetID> &list, double xscale, double yscale); //!< Draws all cycles as triangles
    void DrawZones(std::deque<zZone *> const &list); //!< Draws all Zones

    /*
    * rimWalls    draw rim walls?
    * cycleWalls  draw cycle walls?
    * cycles      draw cycles?
    * cycleSize   size of cycles in real-world units
    * border      border around bounds (in grid units)
    * x y w h     the map will always fit in this rectangle (in OpenGL units)
    * rw rh       width and height of the rectangle in real-world units
    * ix iy       position where inside? (range [0, 1], used if there's room left due to nonmatching aspect ratio)
    */
    void DrawMap(bool rimWalls, bool cycleWalls, bool cycles,
                 double cycleSize, double border,
                 double x, double y, double w, double h,
                 double rw, double rh, double ix, double iy); //!< Draws the entire map
public:
    void Render(); //!< calls DrawMap()
    virtual bool Process(tXmlParser::node cur); //!< Passes on to all Process() functions of the base classes and calls Base::DisplayError() on failure
};

}

#endif
#endif
