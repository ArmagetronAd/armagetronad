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

#include "rGradient.h"
#include "tError.h"

#include <deque>
#include <utility>

rGradient::rGradient() : m_dir(value), m_texScale(1,1), m_tex() {
    //(*this)[0.]=rColor(); //make sure the beginning and end are defined
    //(*this)[1.]=rColor();
}
rGradient::~rGradient() {
}

//! @param edge1 the first edge of the gradient (preferably bottom- left)
//! @param edge2 the second edge of the gradient (preferably top- right)
void rGradient::SetGradientEdges(tCoord const &edge1, tCoord const edge2) {
    if(edge1.x < edge2.x) {
        m_origin.x = edge1.x;
        m_dimensions.x = edge2.x - edge1.x;
    } else {
        m_origin.x = edge2.x;
        m_dimensions.x = edge1.x - edge2.x;
    }
    if(edge1.y < edge2.y) {
        m_origin.y = edge1.y;
        m_dimensions.y = edge2.y - edge1.y;
    } else {
        m_origin.y = edge2.y;
        m_dimensions.y = edge1.y - edge2.y;
    }
}

float rGradient::GetGradientPt(tCoord const &where) {
    float ret = 0;
    switch (m_dir) {
    case horizontal:
        ret=(where.x-m_origin.x)/m_dimensions.x;
        break;
    case vertical:
        ret=(where.y-m_origin.y)/m_dimensions.y;
        break;
    case value:
        ret=m_at;
        break;
    default: tASSERT(0);
    }
    if(ret<0.) ret=0.;
    if(ret>1.) ret=1.;
    return ret;
}

rColor rGradient::GetColor(float where) {
#ifndef DEDICATED
    if(empty()) return rColor();
    if(begin()->first >= where) return begin()->second;
    iterator upper, lower;
    iterator i=begin();
    iterator j=begin();
    ++j;
    bool finished = false;
    for(; j!=end(); ++i, ++j) {
        if(j->first >= where) {
            lower = i;
            upper = j;
            finished = true;
            break;
        }
    }
    if (!finished) return rbegin()->second;
    float diff = upper->first - lower->first;
    float pos = where - lower->first;
    rColor &c1 = lower->second;
    rColor &c2 = upper->second;
    float r = c1.r_*(diff-pos)/diff + c2.r_*pos/diff;
    float g = c1.g_*(diff-pos)/diff + c2.g_*pos/diff;
    float b = c1.b_*(diff-pos)/diff + c2.b_*pos/diff;
    float a = c1.a_*(diff-pos)/diff + c2.a_*pos/diff;
    return rColor(r,g,b,a);
#else
    return rColor(0.,0.,0.,0.);
#endif
}

void rGradient::SetValues(tCoord const &where, float *position, float *color, float *texcoords) {
    position[0] = where.x;
    position[1] = where.y;
    rColor c = GetColor(GetGradientPt(where));
    color[0] = c.r_;
    color[1] = c.g_;
    color[2] = c.b_;
    texcoords[0] = (where.x-m_origin.x)/m_dimensions.x/m_texScale.x,
    texcoords[1] = (where.y-m_origin.y)/m_dimensions.y/m_texScale.y;
}

void rGradient::DrawAt(tCoord const &where) {
#ifndef DEDICATED
    GetColor(GetGradientPt(where)).Apply();
    if(m_tex.Valid()) {
        glTexCoord2f((where.x-m_origin.x)/m_dimensions.x/m_texScale.x, (m_origin.y-where.y)/m_dimensions.y/m_texScale.y);
    }
#endif
}

//! @param edge1 one edge of the rectangle
//! @param edge2 the opposite edge
void rGradient::DrawAtomicRect(tCoord const &edge1, tCoord const &edge2) {
#ifndef DEDICATED
    DrawPoint(edge1);
    DrawPoint(tCoord(edge1.x, edge2.y));
    DrawPoint(edge2);
    DrawPoint(tCoord(edge2.x, edge1.y));
#endif
}

//! @param edge1 one edge of the rectangle
//! @param edge2 the opposite edge
void rGradient::DrawRect(tCoord const &edge1, tCoord const &edge2) {
#ifndef DEDICATED
    float tCoord::*x; //those are correct for horizontal gradients,
    float tCoord::*y; //vertical ones just get turned around
	BeginDraw();
    BeginQuads();
    switch(m_dir) {
    case horizontal:
        x = &tCoord::x;
        y = &tCoord::y;
        break;
    case vertical:
        x = &tCoord::y;
        y = &tCoord::x;
        break;
    default:
        DrawAtomicRect(edge1, edge2);
        RenderEnd();
        return;
    }
    tCoord const &left = (edge1.*x < edge2.*x) ? edge1 : edge2;
    tCoord const &right = (edge1.*x < edge2.*x) ? edge2 : edge1;
    float min = GetGradientPt(left);
    float max = GetGradientPt(right);
    iterator i = upper_bound(min);
    float last = left.*x;
    for(; i != end() && i->first < max; ++i) {
        float newpt=i->first - min;
        float newx=newpt*m_dimensions.*x+m_origin.*x;
        tCoord todraw1;
        tCoord todraw2;
        todraw1.*x = last;
        todraw2.*x = newx;
        todraw1.*y = left.*y;
        todraw2.*y = right.*y;
        DrawAtomicRect(todraw1, todraw2);
        last = newx;
    }
    tCoord todraw;
    todraw.*x = last;
    todraw.*y = left.*y;
    DrawAtomicRect(todraw, right);
    RenderEnd();
#endif
}

void se_glFloorTexture();
void rGradient::BeginDraw() {
    if(m_tex.Valid()) {
        m_tex.Select();
    }
}

//! @param where the point the color should be taken from and drawn
void rGradient::DrawPoint(tCoord const &where) {
#ifndef DEDICATED
    DrawAt(where);
    Vertex(where.x, where.y);
#endif
}
