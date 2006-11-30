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
#include "cockpit/cCockpit.h"
#include "nConfig.h"
#include "tCoord.h"

#ifndef DEDICATED

#include "rRender.h"
#include "rScreen.h"
#include "zone/zZone.h"
#include "eRectangle.h"
#include "ePlayer.h"
#include "eTimer.h"
#endif

#include <vector>

static bool stc_forbidHudMap = false;
// TODO find out why this is throwing an error: “Two tConfItems with the same name FORBID_HUD_MAP!”
// Z-Man: hmm, it doesn't throw an error for me.
static nSettingItem<bool> fcs("FORBID_HUD_MAP", stc_forbidHudMap);
extern std::vector<tCoord> se_rimWallRubberBand;

#ifndef DEDICATED

extern std::deque<zZone *> sg_Zones;

namespace cWidget {

bool Map::Process(tXmlParser::node cur) {
    if (
        WithCoordinates ::Process(cur) ||
        WithForeground ::Process(cur) ||
        WithBackground ::Process(cur))
        return true;
    if(cur.IsOfType("MapMode")) {
        int toggleKey;
        cur.GetProp("toggleKey", toggleKey);
        m_toggleKey = toggleKey;
        cCockpit::GetCockpit()->AddEventHandler(toggleKey, this);

        tString mode = cur.GetProp("allowedModes");
        std::map<tString, unsigned> modes;
        modes[tString("full")] = MODE_STD;
        modes[tString("closestZone")] = MODE_ZONE;
        modes[tString("cycle")] = MODE_CYCLE;
        modes[tString("all")] = MODE_ALL;
        modes[tString("*")] = MODE_ALL;

        m_allowedModes = 0;
        bool invert=false;
        if(mode(0) == '^') {
            invert=true;
            mode.erase(0,1);
        }
        mode += " ";

        tString::size_type pos = 0, next;
        while((next = mode.find(' ', pos)) != tString::npos) {
            tString thismode = mode.SubStr(pos, next - pos);
            if(modes.count(thismode)) {
                m_allowedModes |= modes[thismode];
            } else {
                tERR_WARN(tString("Nothing known about map mode '") + thismode + "'.");
            }
            pos = next+1;
        }
        if(invert) {
            m_allowedModes = ~m_allowedModes;
        }
        cur.GetProp("defaultMode", m_mode);
        return true;
    }
    DisplayError(cur);
    return false;
}
void Map::HandleEvent(bool state, int id) {
    if(id == m_toggleKey) {
        if(state) {
            ToggleMode();
        }
    } else {
        Base::HandleEvent(state, id);
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
    double pl_CurPosX, pl_CurPosY, pl_CurSpeed, min_dist2, dist2, rad;
    cCockpit* cp = cCockpit::GetCockpit();
    if(!rimWalls && !cycleWalls && !cycles) return;
    const eRectangle &bounds = eWallRim::GetBounds();
    double lx = bounds.GetLow().x - border, hx = bounds.GetHigh().x + border;
    double ly = bounds.GetLow().y - border, hy = bounds.GetHigh().y + border;
    //std::cerr << "lx: " << lx << std::endl;
    //std::cerr << "hx: " << hx << std::endl;
    //std::cerr << "ly: " << ly << std::endl;
    //std::cerr << "hy: " << hy << std::endl;
    double mw = hx - lx, mh = hy - ly;
    double xpos, ypos, xscale, yscale;
    double xrat = (rw * mh) / (rh * mw);
    double yrat = (rh * mw) / (rw * mh);
    // set scale and position
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
    // manage mode
    switch (m_mode) {
    case MODE_ZONE:
        pl_CurPosX = cp->GetFocusCycle()->Position().x;
        pl_CurPosY = cp->GetFocusCycle()->Position().y;
        min_dist2 = (mw*mw+mh*mh)*1000;
        rad = 0;
        for(std::deque<zZone *>::const_iterator i = sg_Zones.begin(); i != sg_Zones.end(); ++i) {
            tASSERT(*i);
            tCoord const &position = (*i)->GetPosition();
            const float radius = (*i)->GetRadius();
            dist2=(position.x-pl_CurPosX)*(position.x-pl_CurPosX)+(position.y-pl_CurPosY)*(position.y-pl_CurPosY);
            if (dist2<min_dist2) {
                min_dist2 = dist2;
                m_centre = position;
                rad = radius;
            }
        }
        m_zoom = (w>h)?h/(yscale*3*rad):w/(xscale*3*rad);
        m_zoom = (m_zoom<1)?1:m_zoom;
        // check if at least 1 zone was found, if not, toggle to mode 2 ...
        if (rad==0) {
            m_mode = MODE_CYCLE;
        } else {
            break;
        }
    case MODE_CYCLE:
        m_centre = cp->GetFocusCycle()->Position();
        pl_CurSpeed = cp->GetFocusCycle()->Speed();
        m_zoom = (w>h)?h/(yscale*3*pl_CurSpeed):w/(xscale*3*pl_CurSpeed);
        break;
    default :
        m_zoom = 1;
        m_centre.x = lx + mw / 2;
        m_centre.y = ly + mh / 2;
        break;
    }
    xscale *=m_zoom;
    yscale *=m_zoom;
    xpos -= m_centre.x * xscale - w / 2;
    ypos -= m_centre.y * yscale - h / 2;
    // set clipping frame in map coordinates
    clp_lx = m_centre.x - w / xscale / 2 - 1;
    clp_rx = m_centre.x + w / xscale / 2 + 1;
    clp_ty = m_centre.y + h / yscale / 2 + 1;
    clp_by = m_centre.y - h / yscale / 2 - 1;
    // set projection matrix
    glPushMatrix();
    GLfloat m[16];
    glGetFloatv(GL_PROJECTION_MATRIX, m);
    m[0]  *= xscale;
    m[5]  *= yscale;
    m[12] += xpos;
    m[13] += ypos;
    glLoadMatrixf(m);
    // if needed, draw a frame
    if (m_mode != MODE_STD) {
        // Add frame ...
        m_foreground.GetColor(tCoord(0.,0.)).Apply();
        glBegin(GL_LINE_STRIP);
        //TODO: this should use a function of the rGradient.
        glVertex2f(clp_lx, clp_ty);
        glVertex2f(clp_rx, clp_ty);
        glVertex2f(clp_rx, clp_by);
        glVertex2f(clp_lx, clp_by);
        glVertex2f(clp_lx, clp_ty);
        glEnd();
        m_background.SetGradientEdges(tCoord(clp_lx, clp_ty), tCoord(clp_rx, clp_by));
        m_background.DrawRect(tCoord(clp_lx, clp_ty), tCoord(clp_rx, clp_by));
    }
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
            eCoord begin = wall->EndPoint(0), end = wall->EndPoint(1);
            if (ClipLine(begin,end)) {
                glVertex2f(begin.x, begin.y);
                glVertex2f(end.x, end.y);
            }
        }


    }
    glEnd();
    if(sr_alphaBlend && m_mode == MODE_STD) {
        m_background.GetColor(tCoord(0.,0.)).Apply();
        glBegin(GL_POLYGON);
        //std::cerr << "===\n";
        for(std::vector<tCoord>::iterator iter = se_rimWallRubberBand.begin(); iter != se_rimWallRubberBand.end(); ++iter) {
            glVertex2f(iter->x, iter->y);
            //std::cerr << "result: " << *iter << std::endl;
        }
        glVertex2f(se_rimWallRubberBand.front().x, se_rimWallRubberBand.front().y);
        glEnd();
    }
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
        float prevDist = coords[0].Pos;
        if(prevDist < minDist) prevDist = minDist;
        prevDist = (prevDist - begDist) / lenDist;
        for(j=1; j<numcoords; j++) {
            bool curDangerous = coords[j].IsDangerous;
            float curDist = coords[j].Pos;
            if(curDist < minDist) curDist = minDist;
            curDist = (curDist - begDist) / lenDist;
            tCoord p1 = (1 - prevDist)* begPos + prevDist * endPos;
            tCoord p2 = (1 - curDist) * begPos + curDist * endPos;
            if (ClipLine(p1,p2)) {;
                glVertex2f(p1.x, p1.y);
                glVertex2f(p2.x, p2.y);
            }
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
            eCoord pos = cycle->PredictPosition(), dir = cycle->Direction();
            tCoord p = pos;
            if (ClipLine(p,p)) { // quite dirty, whatever ...
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
        for(int i = 0; i<steps/2; ++i) {
            tCoord p1 = currentPos.Turn(offset);
            currentPos = p1.Turn(offset);
            p1 += position;
            tCoord p2 = currentPos + position;
            if (i%2==1) {
                if (ClipLine(p1, p2)) {;
                    glBegin(GL_LINES);
                    glVertex2f(p1.x, p1.y);
                    glVertex2f(p2.x, p2.y);
                    glEnd();
                }
            }
        }
        RenderEnd();
    }
}

// Liang-Barsky 2D Line clipping ...
bool Map::ClipLine(tCoord &c1, tCoord &c2) const {
    double m1, m2;
    double p1, p2, p3, p4, q1, q2, q3, q4;

    // first check if line is a point ...
    if (c1 == c2) {
        // line is a point, easy case ...
        if ((c1.x>clp_lx)&&(c1.x<clp_rx)&&(c1.y>clp_by)&&(c1.y<clp_ty)) {
            return true;
        }
    } else {
        // apply Liang Barsky algorithm ...
        m1 = 0;
        m2 = 1;

        tCoord d = c2 - c1;

        p1=(-d.x);
        p2=d.x;
        p3=(-d.y);
        p4=d.y;
        q1=(c1.x-clp_lx);
        q2=(clp_rx-c1.x);
        q3=(c1.y-clp_by);
        q4=(clp_ty-c1.y);

        if(CheckEdge(p1,q1,m1,m2) && CheckEdge(p2,q2,m1,m2) &&
                CheckEdge(p3,q3,m1,m2) && CheckEdge(p4,q4,m1,m2)) {
            if(m2<1) {
                c2 = c1+m2*d;
            }
            if(m1>0) {
                c1 += m1*d;
            }
            return true;
        }
    }
    return false;
}

bool Map::CheckEdge(double p,double q,double &m1,double &m2) const {
    double r;

    if(p<0) {
        r=(q/p);
        if(r>m2) return false;
        else if(r>m1) m1=r;
    } else if(p>0) {
        r=(q/p);
        if(r<m1) return false;
        else if(r<m2) m2=r;
    } else {
        if(q<0) return false;
    }
    return true;
}

void Map::ToggleMode(void) {
    if(m_allowedModes & MODE_ALL) {
        do {
            m_mode = m_mode << 1;
            if(m_mode > MODE_ALL) {
                m_mode = 1;
            }
        } while(!(m_mode & m_allowedModes));
    }
}

Map::Map():
        m_mode(MODE_STD),
        m_allowedModes(MODE_ALL),
        m_toggleKey(0)
{}

}

#endif
