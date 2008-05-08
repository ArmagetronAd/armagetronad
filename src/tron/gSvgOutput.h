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
//! @brief Contains the class for Svg output
//!

#ifndef ARMAGETRON_GSVGOUTPUT_H
#define ARMAGETRON_GSVGOUTPUT_H

#include "tList.h"
#include "gWall.h"
#include <deque>
#include <vector>

//! Write a svg file of a 2D map of the grid
class SvgOutput {
private:
    std::ofstream svgFile;	//!< file to write svg output
    long afterRimWallsPos;	//!< position in this file after header and rim walls as they will not change during the round
    float lx, ly, hx, hy;	//!< lower and higher coordinate of the map
	
    void WriteSvgHeader();	//!< Write to svg output file the appropriate svg header
    void WriteSvgFooter();	//!< Write to svg output file the appropriate svg footer

    void DrawRimWalls( tList<eWallRim> &list );		//!< Draws all the rim walls
    void DrawWalls(tList<gNetPlayerWall> &list);	//!< Draws all player walls
    void DrawObjects();								//!< Draws all game objects

public:
    void Create();	//!< create svg file with header, rim walls, player walls, other objects and footer 

    SvgOutput();	//!< default constructor
    ~SvgOutput();	//!< default destructor
};

struct gSvgColor {
    REAL r, g, b;
    gSvgColor(REAL R, REAL G, REAL B) : r(R), g(G), b(B) {}
    gSvgColor(gRealColor const &c) : r(c.r), g(c.g), b(c.b) {}
};
std::ostream &operator<<(std::ostream &s, gSvgColor const &c);

#endif
