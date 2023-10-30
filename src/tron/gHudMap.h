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

#ifndef ARMAGETRON_RMAP_H
#define ARMAGETRON_RMAP_H

/*
 * rimWalls    draw rim walls?
 * cycleWalls  draw cycle walls?
 * cycles      draw cycles?
 * cycleSize   size of cycles in real-world units
 * border      border around bounds
 * x y w h     the map will always fit in this rectangle
 * rw rh       width and height of the rectangle in real-world units
 * ix iy       position where inside?
 */

struct gHudMapDrawConf
{
	bool rimWalls = true;
	bool cycleWalls = true;
	bool cycles = true;
	bool zones = true;
	
	double cycleSize, border;
	
	double x, y, w, h;
	double rw, rh;
	double ix, iy;
	
	struct {
		float r, g, b, a=0;
	} bg;
};


void DrawMap( gHudMapDrawConf c );

static inline void DrawMap(bool rimWalls, bool cycleWalls, bool cycles,
			 double cycleSize, double border,
			 double x, double y, double w, double h,
			 double rw, double rh, double ix, double iy)
{
	gHudMapDrawConf c;
	
	c.rimWalls = rimWalls;
	c.cycleWalls = cycleWalls;
	c.cycles = cycles;
	
	c.cycleSize = cycleSize;
	c.border = border;
	
	c.x = x; c.y = y; c.w = w; c.h = h;
	c.rw = rw; c.rh = rh;
	c.ix = ix; c.iy = iy;
	
	DrawMap( c );
}

#endif // ARMAGETRON_RMAP_H