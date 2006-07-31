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

#ifndef ArmageTron_rGRADIENT_H
#define ArmageTron_rGRADIENT_H

#include "defs.h"
#include "rColor.h"
#include "tCoord.h"
#include <map>
#include <deque>
#include <utility>

//! Gradient class, able to store a gradient and perform basic render functions with it
class rGradient: public std::map<float, rColor> {
    int m_dir; //!< the direction the gardient is laid in
    float m_at; //!< current value, used when m_dir == value
    tCoord m_origin; //!< bottom-left point of the gradient
    tCoord m_dimensions; //!< width and height of it

    //! return the relevant value (x, y or m_at) depending on m_dir
    float GetGradientPt(tCoord const &where);
    //! get the color for a given point on the gradient, using only
    //! the relevant coordinate (as returned by GetGradientPt)
    rColor GetColor(float where);
public:
    rGradient(); //!< Constructor

    //! Enum for describing the direction of the gradient
    enum direction {
        horizontal, //!< For a gradient going from the left to the right
        vertical, //!< For a gradient going from the bottom to the top
        value //!< For a soild area changing its color based on some other condition
    };
    //! Sets the type/direction of the gradient
    //! @param dir the desired type/direction
    void SetDir(direction dir) { m_dir = dir; };
    //! set the value, only used when the type is "value"
    //! @param at the value, 1 should be the maximum and 0 the minimum
    void SetValue(float at) { m_at = at; };
    //! set the boundaries of the gradient
    void SetGradientEdges(tCoord const &edge1, tCoord const edge2);

    //! get the color at a certain point
    //! @param where the point in the gradient. If it lies outside the edges of the gradient the nearest possible point will be used
    //! @returns the color at the specific point
    rColor GetColor(tCoord const &where) { return GetColor(GetGradientPt(where)); };

    //! Draw a rectangle using only the colors of the edges
    void DrawAtomicRect(tCoord const &edge1, tCoord const &edge2);
    //!Draw a rectangle, but split it up into multiple rectangles if necessary
    void DrawRect(tCoord const &edge1, tCoord const &edge2);
    //! call Color() with the color at the given coordinate and then call Vertex() on it
    void DrawPoint(tCoord const &where);
};

#endif
