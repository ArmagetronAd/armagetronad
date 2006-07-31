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

#include "cockpit/cMap.h"
#include "nConfig.h"

#ifndef DEDICATED

#include "rRender.h"
#include "rScreen.h"
#include "zone/zZone.h"
#include "eRectangle.h"
#include "ePlayer.h"
#include "eTimer.h"
#endif

static bool stc_forbidHudMap = false;
static nSettingItem<bool> fcs("FORBID_HUD_MAP", stc_forbidHudMap);

#ifndef DEDICATED

extern std::deque<zZone *> sg_Zones;

namespace cWidget {

bool Map::Process(tXmlParser::node cur) {
    if (
        WithCoordinates ::Process(cur))
        return true;
    else {
        DisplayError(cur);
        return false;
    }
}
void Map::Render() {
    // I haven't checked possible initial matrix state, so init to identity and modelview
    if(stc_forbidHudMap) return; // the server doesn't want us to do that
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glDisable(GL_LINE_SMOOTH);
    glHint (GL_LINE_SMOOTH_HINT, GL_FASTEST);
    DrawMap(true, true, true,
            5.5, 0.,
            m_position.x-m_size.x, m_position.y-m_size.y, 2.*m_size.x, 2.*m_size.y,
            sr_screenWidth*m_size.x, sr_screenWidth*m_size.y, .5, .5);
}
void Map::DrawMap(bool rimWalls, bool cycleWalls, bool cycles,
                  double cycleSize, double border,
                  double x, double y, double w, double h,
                  double rw, double rh, double ix, double iy) {
    if(!rimWalls && !cycleWalls && !cycles) return;
    const eRectangle &bounds = eWallRim::GetBounds();
    double lx = bounds.GetLow().x - border, hx = bounds.GetHigh().x + border;
    double ly = bounds.GetLow().y - border, hy = bounds.GetHigh().y + border;
    double mw = hx - lx, mh = hy - ly;
    double xpos, ypos, xscale, yscale;
    double xrat = (rw * mh) / (rh * mw);
    double yrat = (rh * mw) / (rw * mh);
    if(xrat > yrat) {
        xscale = (w * rh) / (mh * rw);
        yscale = h / mh;
        xpos = x + ix * (w - w * yrat);
        ypos = y;
    } else {
        xscale = w / mw;
        yscale = (h * rw) / (mw * rh);
        xpos = x;
        ypos = y + iy * (h - h * xrat);
    }
    xpos -= lx * xscale;
    ypos -= ly * yscale;
    glPushMatrix();
    GLfloat m[16];
    glGetFloatv(GL_PROJECTION_MATRIX, m);
    m[0]  *= xscale;
    m[5]  *= yscale;
    m[12] += xpos;
    m[13] += ypos;
    glLoadMatrixf(m);
    if(rimWalls)
        DrawRimWalls(se_rimWalls);
    if(cycleWalls) {
        DrawWalls(sg_netPlayerWallsGridded);
        DrawWalls(sg_netPlayerWalls);
    }
    DrawZones(sg_Zones);
    if(cycles)
        DrawCycles(se_PlayerNetIDs, (cycleSize * w) / (rw * xscale), (cycleSize * h) / (rh * yscale));
    glPopMatrix();
}

void Map::DrawRimWalls( tList<eWallRim> &list ) {
    glColor4f(1, 1, 1, .5);
    glBegin(GL_LINES);
    {
        for (int i=list.Len()-1; i >= 0; --i)
        {
            eWallRim *wall = list[i];
            const eCoord &begin = wall->EndPoint(0), &end = wall->EndPoint(1);
            glVertex2f(begin.x, begin.y);
            glVertex2f(end.x, end.y);
        }

    }
    glEnd();
}

void Map::DrawWalls(tList<gNetPlayerWall> &list) {
    unsigned i, len=list.Len();
    double currentTime = se_GameTime();
    bool limitedLength = gCycle::WallsLength() > 0;
    double wallsStayUpDelay = gCycle::WallsStayUpDelay();
    glBegin(GL_LINES);
    for(i=0; i<len; i++) {
        gNetPlayerWall *wall = list[i];
        gCycle *cycle = wall->Cycle();
        if(!cycle) continue;
        double wallsLength = cycle->ThisWallsLength();
        double alpha = 1;
        if(!cycle->Alive()) {
            alpha -= 2 * (currentTime - cycle->DeathTime() - wallsStayUpDelay);
            if(alpha <= 0) continue;
        }
        glColor4f(cycle->color_.r, cycle->color_.g, cycle->color_.b, alpha);
        double cycleDist = cycle->GetDistance();
        double minDist = limitedLength && cycleDist > wallsLength ? cycleDist - wallsLength : 0;
        const eCoord &begPos = wall->EndPoint(0), &endPos = wall->EndPoint(1);
        tArray<gPlayerWallCoord> &coords = wall->Coords();
        double begDist = wall->BegPos();
        double lenDist = wall->EndPos() - begDist;
        unsigned j, numcoords = coords.Len();
        if(numcoords < 2) continue;
        bool prevDangerous = coords[0].IsDangerous;
        double prevDist = coords[0].Pos;
        if(prevDist < minDist) prevDist = minDist;
        prevDist = (prevDist - begDist) / lenDist;
        for(j=1; j<numcoords; j++) {
            bool curDangerous = coords[j].IsDangerous;
            double curDist = coords[j].Pos;
            if(curDist < minDist) curDist = minDist;
            curDist = (curDist - begDist) / lenDist;
            glVertex2f((1 - prevDist) * begPos.x + prevDist * endPos.x, (1 - prevDist) * begPos.y + prevDist * endPos.y);
            glVertex2f((1 - curDist) * begPos.x + curDist * endPos.x, (1 - curDist) * begPos.y + curDist * endPos.y);
            prevDangerous = curDangerous;
            prevDist = curDist;
        }
    }
    glEnd();
}

void Map::DrawCycles(tList<ePlayerNetID> &list, double xscale, double yscale) {
    unsigned i, len=list.Len();
    double currentTime = se_GameTime();
    for(i=0; i<len; i++) {
        const gCycle *cycle = dynamic_cast<gCycle *>(list[i]->Object());
        if(cycle) {
            double alpha = 1;
            if(!cycle->Alive()) {
                alpha -= 2 * (currentTime - cycle->DeathTime());
                if(alpha <= 0) continue;
            }
            glColor4f(cycle->color_.r, cycle->color_.g, cycle->color_.b, alpha);
            eCoord pos = cycle->Position(), dir = cycle->Direction();
            glPushMatrix();
            GLfloat m[16] = {
                                xscale * dir.x, yscale * dir.y, 0, 0,
                                -xscale * dir.y, yscale * dir.x, 0, 0,
                                0, 0, 1, 0,
                                pos.x, pos.y, 0, 1
                            };
            glMultMatrixf(m);
            glBegin(GL_TRIANGLES);
            glVertex2f(.5, 0);
            glVertex2f(-.5, .5);
            glVertex2f(-.5, -.5);
            glEnd();
            glPopMatrix();
        }
    }
}

void Map::DrawZones(std::deque<zZone *> const &list) {
    for(std::deque<zZone *>::const_iterator i = list.begin(); i != list.end(); ++i) {
        tASSERT(*i);
        tCoord const &rotation = (*i)->GetRotation();
        tCoord const &position = (*i)->GetPosition();
        rColor const &color = (*i)->GetColor();
        const float radius = (*i)->GetRadius();

        tCoord currentPos = rotation*radius;

        const int steps=20;

        static tCoord offset(cos(M_PI*2./steps), sin(M_PI*2./steps));

        BeginLines();
        color.Apply();
        for(int i = 0; i<steps; ++i) {
            Vertex(currentPos.x + position.x, currentPos.y + position.y, 0.);
            currentPos = currentPos.Turn(offset);
        }
        RenderEnd();
    }
}

}

#endif
