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
#include <vector>

class zZone;

namespace cWidget {

//! Processes and renders a map of the grid
class Map : public WithCoordinates, public WithBackground, public WithForeground {
    void DrawRimWalls( tList<eWallRim> &list ); //!< Draws all the rim walls
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

    //
    //! mode is a var to manage minimap mode :
    typedef enum {
        MODE_STD, //!< the whole map
        MODE_ZONE, //!< using zoom factor and clipping centered on the closest zone, when there's no zone, switch to MODE_CYCLE
        MODE_CYCLE //!< using zoom factor and clipping centered on the player position
    } mode_t;
    //! rotation mode
    typedef enum {
        ROTATION_FIXED, //!< default, no rotation
        ROTATION_SPAWN, //!< fixed in the direction of the last spawn point
        ROTATION_CYCLE, //!< always turn so the current cycle looks upwards
        ROTATION_CAMERA //!< always look in the same direcion as the camera
    } rotation_t;
    mode_t m_mode;
    tCoord m_centre; // minimap center position on overall map
    float m_zoom; // vars to store zoom factor
    rotation_t m_rotation;

    int m_toggleKey; //key to toggle the mode

    void ToggleMode(void); //!< Toggles the display mode.

    class Mode {
    public:
        mode_t m_mode;
        rotation_t m_rotation;
        float m_zoom;
        Mode(tXmlParser::node cur);
    };
    std::vector<Mode> m_modes;
    int m_currentMode;
    void Apply(Mode const &mode);

    class Clipper {
    public:
        virtual ~Clipper(){}
        virtual void Begin(Map &map, tCoord const &e1, tCoord const &e2) = 0;
        virtual void End() = 0;
    };
class ClipperRect : public Clipper {
    public:
        void Begin(Map &map, tCoord const &e1, tCoord const &e2);
        void End();
    };
    friend class ClipperRect;
    /* not done yet
    class ClipperCircle : public Clipper {
    	int m_edges;
    public:
    	ClipperCircle();
    	void Begin(Map &map, tCoord const &e1, tCoord const &e2);
    	void End();
    };
    friend class ClipperCircle;
    */
    std::auto_ptr<Clipper> m_clipper;
public:
    void HandleEvent(bool state, int id);
    void Render(); //!< calls DrawMap()
    virtual bool Process(tXmlParser::node cur); //!< Passes on to all Process() functions of the base classes and calls Base::DisplayError() on failure

    Map(); //!< default constructor
};

}

#endif
#endif
