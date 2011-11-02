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

#ifndef ArmageTron_PATH_H
#define ArmageTron_PATH_H

#include "tArray.h"
#include "eCoord.h"

class eHalfEdge;

class ePath{
public:
#ifdef DEBUG
    static void RenderLast();  // renders the last found path
#endif

    friend class eHalfEdge;

    ePath();
    ~ePath();
    bool     Valid()           const { return current >= 0 && current < positions.Len(); }
    eCoord&  CurrentPosition() const { return positions(current); }
    eCoord&  CurrentOffset()   const { return offsets(current); }
    bool     Proceed();
    bool     GoBack();

    void Clear();

    void Add(const eCoord&  point);
protected:
    tArray<eCoord> positions;
    tArray<eCoord> offsets;
    int            current;

    void Add(eHalfEdge     *edge);

};

#endif
