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

#define DONTDOIT
#include "rRender.h"

#include "gWall.h"
#include "gZone.h"
#include "gCycle.h"
#include "eAdvWall.h"
#include "eRectangle.h"
#include "eTimer.h"
#include "ePlayer.h"
#include "gHudMap.h"

extern tList<gNetPlayerWall> sg_netPlayerWallsGridded;
extern std::deque<gZone *> sg_Zones;

static void DrawRimWalls(tList<eWallRim> &list) {
	glColor4f(1, 1, 1, .5);
	glBegin(GL_LINES);
	unsigned i, len=list.Len();
	for(i=0; i<len; i++) {
		eWallRim *wall = list[i];
		const eCoord &begin = wall->EndPoint(0), &end = wall->EndPoint(1);
		glVertex2f(begin.x, begin.y);
		glVertex2f(end.x, end.y);
	}
	glEnd();
}

static void DrawWalls(tList<gNetPlayerWall> &list) {
	unsigned i, len=list.Len();
	double currentTime = se_GameTime();
	double wallsLength = gCycle::WallsLength();
	double wallsStayUpDelay = gCycle::WallsStayUpDelay();
	bool limitedLength = wallsLength > 0;
	glBegin(GL_LINES);
	for(i=0; i<len; i++) {
		gNetPlayerWall *wall = list[i];
		gCycle *cycle = wall->Cycle();
		if(!cycle) continue;
		double alpha = 1;
		if(!cycle->Alive() && wallsStayUpDelay >= 0) {
			alpha -= 2 * (currentTime - cycle->DeathTime() - wallsStayUpDelay);
			if(!(alpha > 0)) continue;
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
			if(prevDangerous) {
				glVertex2f(begPos.x + prevDist * (endPos.x - begPos.x), begPos.y + prevDist * (endPos.y - begPos.y));
				glVertex2f(begPos.x +  curDist * (endPos.x - begPos.x), begPos.y +  curDist * (endPos.y - begPos.y));
			}
			prevDangerous = curDangerous;
			prevDist = curDist;
		}
	}
	glEnd();
}

static void DrawCycles(tList<ePlayerNetID> &list, double xscale, double yscale) {
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
				(float)(xscale * dir.x), (float)(yscale * dir.y), 0, 0,
				(float)(-xscale * dir.y), (float)(yscale * dir.x), 0, 0,
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

static void DrawZones(std::deque<gZone *> &list)
{
	glEnable( GL_LINE_SMOOTH );
	glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
	glLineWidth( 1.5 );
	
	static auto c = eCoord( 0, 0 );
	for(auto i=list.begin();i!=list.end();i++)
	{
		static_cast<eGameObject *>(*i)->Render2D( c );
	}
	
	glDisable( GL_LINE_SMOOTH );
	glLineWidth( 1 );
}


void DrawMap( gHudMapDrawConf c )
{
	const eRectangle &bounds = eWallRim::GetBounds();
	double lx = bounds.GetLow().x - c.border, hx = bounds.GetHigh().x + c.border;
	double ly = bounds.GetLow().y - c.border, hy = bounds.GetHigh().y + c.border;
	double mw = hx - lx, mh = hy - ly;
	double xpos, ypos, xscale, yscale;
	double xrat = (c.rw * mh) / (c.rh * mw);
	double yrat = (c.rh * mw) / (c.rw * mh);
	if(xrat > yrat) {
		xscale = (c.w * c.rh) / (mh * c.rw);
		yscale = c.h / mh;
		xpos = c.x + c.ix * (c.w - c.w * yrat);
		ypos = c.y;
	} else {
		xscale = c.w / mw;
		yscale = (c.h * c.rw) / (mw * c.rh);
		xpos = c.x;
		ypos = c.y + c.iy * (c.h - c.h * xrat);
	}
	glPushMatrix();
	GLfloat m[16] = {
		(float)xscale, 0, 0, 0,
		0, (float)yscale, 0, 0,
		0, 0, 1, 0,
		(float)(xpos - xscale * lx), (float)(ypos - yscale * ly), 0, 1
	};
	glMultMatrixf(m);
	if(c.rimWalls)
		DrawRimWalls(se_rimWalls);
	if(c.cycleWalls) {
		DrawWalls(sg_netPlayerWallsGridded);
		DrawWalls(sg_netPlayerWalls);
	}
	if(c.cycles)
		DrawCycles(se_PlayerNetIDs, (c.cycleSize * c.w) / (c.rw * xscale), (c.cycleSize * c.h) / (c.rh * yscale));
	if(c.zones)
		DrawZones(sg_Zones);
	
	glPopMatrix();
}
