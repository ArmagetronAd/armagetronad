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
#include "nConfig.h"
#include "gWinZone.h"
#include "eRectangle.h"
#include "ePlayer.h"
#include "eTimer.h"
#include "eGrid.h"
#include "gCycle.h"
#include "tDirectories.h"
#include "gSvgOutput.h"

extern std::vector<eCoord> se_rimWallRubberBand;

void SvgOutput::WriteSvgHeader() {
	// Assume the file is already open
	svgFile << "<?xml version=\"1.0\" standalone=\"no\"?>\n";
	svgFile << "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n";
	svgFile << "<svg width=\"100%\" height=\"100%\" version=\"1.1\" viewBox=\"" << -hx << " " << ly << " "
			<< hx-lx << " " << hy-ly << "\" xmlns=\"http://www.w3.org/2000/svg\">\n";
	svgFile << "<rect x=\"" << -hx << "\" y=\"" << ly << "\" width=\"" << hx-lx << "\" height=\"" << hy-ly
			<< "\" stroke=\"none\" fill=\"#333333\" />\n\n";
}

void SvgOutput::WriteSvgFooter() {
	// Assume the file is already open
	svgFile << "\n</svg>\n";
}

void SvgOutput::DrawRimWalls( tList<eWallRim> &list ) {
	for (int i=list.Len()-1; i >= 0; --i)
	{
		eWallRim *wall = list[i];
		eCoord begin = wall->EndPoint(0), end = wall->EndPoint(1);
		svgFile << "  <line x1=\"" << -begin.x << "\" y1=\"" << begin.y
				<< "\" x2=\"" << -end.x << "\" y2=\"" << end.y 
				<< "\" stroke=\"#FFFFFF\" stroke-width=\"1\" stroke-linecap=\"round\" />\n";
	}
}

void SvgOutput::DrawWalls(tList<gNetPlayerWall> &list) {
    unsigned i, len=list.Len();
    double currentTime = se_GameTime();
    bool limitedLength = gCycle::WallsLength() > 0;
    double wallsStayUpDelay = gCycle::WallsStayUpDelay();
    for(i=0; i<len; i++) {
        gNetPlayerWall *wall = list[i];
        gCycle *cycle = wall->Cycle();
        if(!cycle) continue;
        double wallsLength = cycle->ThisWallsLength();
        double alpha = 1;
        if(!cycle->Alive() && wallsStayUpDelay >= 0) {
            alpha -= 2 * (currentTime - cycle->DeathTime() - wallsStayUpDelay);
            if(alpha <= 0) continue;
        }
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
				eCoord v = endPos - begPos, begin = begPos + v * prevDist, end = begPos + v * curDist;
				svgFile << "  <line x1=\"" << -begin.x << "\" y1=\"" << begin.y << "\" x2=\"" << -end.x
						<< "\" y2=\"" << end.y << "\" stroke=\"rgb(" << cycle->color_.r*100 << "%," << cycle->color_.g*100 
						<< "%," << cycle->color_.b*100 << "%)\" stroke-width=\"1\" stroke-linecap=\"round\" opacity=\"" 
						<< alpha << "\"/>\n";
            }
            prevDangerous = curDangerous;
            prevDist = curDist;
        }
    }
}

void SvgOutput::DrawObjects() {
    tList<eGameObject> const &gameObjects = eGrid::CurrentGrid()->GameObjects();
    size_t len = gameObjects.Len();
    for(size_t i = 0; i < len; ++i) {
        eGameObject *obj = gameObjects(i);
        tASSERT(obj);
        obj->DrawSvg(svgFile);
    }
}

void SvgOutput::Create() {
	// open file as a new one
	svgFile.clear();
	if ( !tDirectories::Var().Open(svgFile, "map.svg", std::ios::trunc) ) return;
	// get map limits
	const eRectangle &bounds = eWallRim::GetBounds();
    lx = ((long)bounds.GetLow().x) - 5;
    ly = ((long)bounds.GetLow().y) - 5;
    hx = ((long)bounds.GetHigh().x) + 5;
    hy = ((long)bounds.GetHigh().y) + 5;
	// add header, rim walls
	WriteSvgHeader();
	svgFile << "<!-- Rim Walls -->\n";
	DrawRimWalls(se_rimWalls);
	svgFile << "<!-- End of Rim Walls -->\n\n";
	// keep the current file offset
	afterRimWallsPos = svgFile.tellp();
	// add player walls and other game objects 
	svgFile << "<!-- Cycle's Walls -->\n";
	DrawWalls(sg_netPlayerWallsGridded);
	DrawWalls(sg_netPlayerWalls);
	svgFile << "\n<!-- Other objects -->\n";
    DrawObjects();
	// add the footer and close the filepp
	WriteSvgFooter();
	svgFile.close();
	//con << "Svg file created\n";
}

SvgOutput::SvgOutput() {
}

SvgOutput::~SvgOutput() {
}
