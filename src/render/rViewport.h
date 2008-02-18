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

#ifndef ArmageTron_VIEWPORT_H
#define ArmageTron_VIEWPORT_H

#define MAX_VIEWPORTS 4

#include "defs.h"
#include "tString.h"
#include "tCoord.h"
#include "tSafePTR.h"

class rViewport{
    REAL left,bottom,width,height;
public:
    rViewport(REAL l,REAL b,REAL w,REAL h):left(l),bottom(b),width(w),height(h){}
    // create a subviewport of top
    rViewport(rViewport &top,rViewport &sub)
            :left  (top.left+top.width*sub.left),
            bottom(top.bottom+top.height*sub.bottom),
            width(top.width*sub.width),
    height(top.height*sub.height){}

    ~rViewport(){tCHECK_DEST;}

#ifndef DEDICATED
    void Select();
#endif

    void Perspective(REAL fov,REAL zNear=1,REAL zFar=10000000,REAL xShift = 0);

    REAL UpDownFOV(REAL fov);

    //! returns a viewport with normal aspect ratio that coincides with this viewport in the bottom line
    rViewport CorrectAspectBottom() const;

    //! returns a viewport that has the same scale horizontally and vertically
    rViewport EqualAspectBottom() const;

    //! returns the height and width of the viewport
    tCoord GetDimensions() const {return tCoord(width, height);}

    static rViewport s_viewportFullscreen,
    s_viewportLeft,s_viewportRight,
    s_viewportTop,s_viewportBottom,
    s_viewportTopLeft,s_viewportBottomLeft,
    s_viewportTopRight,s_viewportBottomRight, s_viewportDemonstation;

    static void CorrectViewport(int i, int mp);
    static void CorrectViewports(int mp);
    static void SetDirectionOfCorrection(int vp, int dir);
    static void Update(int mp);

};

extern int      sr_viewportBelongsToPlayer[MAX_VIEWPORTS];
extern int  s_newViewportBelongsToPlayer[MAX_VIEWPORTS];


class rViewportConfiguration{
    rViewport *viewports[MAX_VIEWPORTS];
public:
    const int num_viewports;

    static int next_conf_num;

    rViewportConfiguration(rViewport *first);
    rViewportConfiguration(rViewport *first,rViewport *second);
    rViewportConfiguration(rViewport *first,rViewport *second,rViewport *third);
    rViewportConfiguration(rViewport *first,rViewport *second,rViewport *third,
                           rViewport *forth);
#ifndef DEDICATED
    void Select(int i);
    static void DemonstrateViewport(tString *titles);
#endif
    rViewport * Port(int i);

    static rViewportConfiguration *s_viewportConfigurations[];
    static const int               s_viewportNumConfigurations;
    static char const *            s_viewportConfigurationNames[];

    static rViewportConfiguration *CurrentViewportConfiguration();

    static rViewport * CurrentViewport(int i);

    static void UpdateConf();
};

#endif



