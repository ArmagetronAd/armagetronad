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
#include "cockpit/cCamview.h"
#include "cockpit/cCockpit.h"
#include "nConfig.h"
#include "tCoord.h"

#ifndef DEDICATED

#include "rRender.h"
#include "rScreen.h"
#include "ePlayer.h"
#include "rViewport.h"
#include "eGrid.h"
#include "eCamera.h"
#include "gCycle.h"

#endif

static bool stc_forbidHudCamera = false;
static nSettingItem<bool> fcs("FORBID_HUD_CAMERA", stc_forbidHudCamera);

#ifndef DEDICATED

namespace cWidget {

bool Camview::Process(tXmlParser::node cur) {
    if (
        WithCoordinates ::Process(cur) ||
        WithForeground ::Process(cur) ||
        WithBackground ::Process(cur))
        return true;
    if(cur.IsOfType("Camview")) {
		view = cur.GetProp("view");
		if (view=="custom") {
			camMode = CAMERA_CUSTOM;
		} else if (view=="follow") {
			camMode = CAMERA_FOLLOW;
		} else if (view=="free") {
			camMode = CAMERA_FREE;
		} else if (view=="in") {
			camMode = CAMERA_IN;
		} else if (view=="server_custom") {
			camMode = CAMERA_SERVER_CUSTOM;
		} else if (view=="smart") {
			camMode = CAMERA_SMART;
		} else if (view=="mer") {
			camMode = CAMERA_MER;
		} else if (view=="main") {
			camMode = CAMERA_SMART;
			mainCameraFlag = true;
		} else {
			view = tString("");
			tERR_WARN("Empty view string");
			DisplayError(cur);
			return false;
		}
//     	con << "Set view " << view << "\n";
        return true;
    } else if(cur.IsOfType("Translation")) {
        cur.GetProp("x", posTranslation.x);
        cur.GetProp("y", posTranslation.y);
        cur.GetProp("z", z);
        posTranslationFlag = true;
//     	con << "Set translation " << posTranslation.x << " " << posTranslation.y << " " << z << "\n";
        return true;
    } else if(cur.IsOfType("Mirror")) {
        mirror = cur.GetPropBool("value");
//     	con << "Set mirror " << mirror << "\n";
        return true;
    } else if(cur.IsOfType("Rise")) {
        cur.GetProp("value", rise);
        riseFlag = true;
//        con << "Set rise " << rise << "\n";
        return true;
    } else if(cur.IsOfType("MainCameraDir")) {
        mainCameraDirFlag = cur.GetPropBool("value");
//     	con << "Set MainCameraDir " << mainCameraDirFlag << "\n";
        return true;
    } else if(cur.IsOfType("Rotation")) {
        cur.GetProp("value",dirAngle);
        dirAngle *= M_PI / 180.;
        dirCos = cos(dirAngle);
        dirSin = sin(dirAngle);
        dirAngleFlag = true;
//     	con << "Set rotation " << dirAngle << "\n";
        return true;
    } else if(cur.IsOfType("FOV")) {
        cur.GetProp("value", fov);
        fovFlag = true;
		if (fov>120) fov=120;
		if (fov<30) fov=30;
//     	con << "Set fov " << fov << "\n";
        return true;
    }

    DisplayError(cur);
    return false;
}

void Camview::Render() {
    // I haven't checked possible initial matrix state, so init to identity and modelview
    if(stc_forbidHudCamera) return; // the server doesn't want us to do that
	glClear(GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

	// Rendering cameras
    rViewportConfiguration* viewportConfiguration = rViewportConfiguration::CurrentViewportConfiguration();

    for ( int viewport = viewportConfiguration->num_viewports-1; viewport >= 0; --viewport )
    {	
    	//con << "render subviewport\n";
		eCamera *mc = eGrid::CurrentGrid()->Cameras()(viewport);
		if (mc) {
			if (cam[viewport]) {
				// force mode and FOV as it is lost between rounds ...
				if (fovFlag) cam[viewport]->SetCameraFOV(fov);
				if (cam[viewport]->GetCamMode()!=camMode) cam[viewport]->SetCamMode(camMode);
				// keep original settings
				tCoord posOrg = cam[viewport]->CameraPos(), dirOrg = cam[viewport]->CameraDir();
				REAL zOrg = cam[viewport]->CameraZ(), riseOrg = cam[viewport]->CameraRise();
				// apply specific settings
				if (posTranslationFlag) {
					tCoord d=cam[viewport]->CameraDir();
					tCoord o(-d.y, d.x);
					tCoord newPos = posOrg + o*posTranslation.x + d*posTranslation.y;
					cam[viewport]->SetCameraPos(newPos);
					cam[viewport]->SetCameraZ(zOrg + z);
				}
				if (dirAngleFlag) {
					tCoord dirNew;
					if (mainCameraDirFlag) {
						dirNew = tCoord(mc->CameraDir().x*dirCos-mc->CameraDir().y*dirSin, 
										mc->CameraDir().x*dirSin+mc->CameraDir().y*dirCos);
					} else {
						dirNew = tCoord(cam[viewport]->CameraDir().x*dirCos-cam[viewport]->CameraDir().y*dirSin, 
											   cam[viewport]->CameraDir().x*dirSin+cam[viewport]->CameraDir().y*dirCos);
					}
					cam[viewport]->SetCameraDir( dirNew );
				}
				if (riseFlag) cam[viewport]->SetCameraRise(rise);
				// render
				viewports[viewport]->Select();
				cam[viewport]->Render();
				cam[viewport]->SetRenderInCockpit(true);    		
				// restore original settings
				cam[viewport]->SetCameraPos(posOrg);
				cam[viewport]->SetCameraDir(dirOrg);
				cam[viewport]->SetCameraZ(zOrg);
				cam[viewport]->SetCameraRise(riseOrg);
				// check if camera center must be adjusted
				if (cam[viewport]->Center()!=mc->Center()) {
					cam[viewport]->SetCenter( mc->Center() );
				}
			} else if (mainCameraFlag) {
				// render
				viewports[viewport]->Select();
				mc->Render();
				mc->SetRenderInCockpit(true);    		
			}
		}
	}
		
	//restore gl context
    glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
    glMatrixMode(GL_PROJECTION);
	glPopMatrix();
    
    // Restore the root viewport
    sr_ResetRenderState(true);
    glViewport (GLsizei(0),
                GLsizei(0),
                GLsizei(sr_screenWidth),
                GLsizei(sr_screenWidth));
}

void Camview::PostParsingProcess()
{
    rViewportConfiguration* viewportConfiguration = rViewportConfiguration::CurrentViewportConfiguration();
    rViewport *vpmod = new rViewport((m_position.x+1)/2,(m_position.y+1)/2,m_size.x,m_size.y*sr_screenWidth/sr_screenHeight);
	// eGrid *grid = eGrid::CurrentGrid();
    for ( int viewport = viewportConfiguration->num_viewports-1; viewport >= 0; --viewport )
    {
        // get the viewport
        rViewport *port = viewportConfiguration->Port( viewport );
		// create the subviewport
    	viewports[viewport] = new rViewport(*port, *vpmod);
    	viewports[viewport]->SetRootViewport(port);
    	// create the camera (if not the main cam to render)
		if (!mainCameraFlag) {
			CreateCamera(viewport);
		}
//    	con << "create subviewport " << m_position.x << " " << m_position.y << " " << m_size.x << " " << m_size.y << "\n";
	}
	ReadyFlag = true;
	delete vpmod;
}

// recreate viewports and cams if needed ...
void Camview::BeforeRoundProcess() {
	if (!ReadyFlag)
		PostParsingProcess();
}

// delete viewports and cams ...
void Camview::AfterRoundProcess() {
    rViewportConfiguration* viewportConfiguration = rViewportConfiguration::CurrentViewportConfiguration();
    for ( int viewport = viewportConfiguration->num_viewports-1; viewport >= 0; --viewport )
    {
		if (!mainCameraFlag) {
			delete cam[viewport];
		}
		cam[viewport]=0;
    	delete viewports[viewport];
		viewports[viewport]=0;
	}
	ReadyFlag = false;
}

void Camview::CreateCamera(int vp) {
	eGrid *grid = eGrid::CurrentGrid();
    // const tList<eCamera>& cameras = grid->Cameras();
	int playerID = sr_viewportBelongsToPlayer[ vp ];
	ePlayer* player = ePlayer::PlayerConfig( playerID );
	cam[vp] = new eCamera(grid,
						  viewports[vp],
						  player->netPlayer,
						  player,
						  camMode,
						  false);
	cam[vp]->SetMirrorView(mirror);
	// get main cam
	if (fovFlag) cam[vp]->SetCameraFOV(fov);
}

Camview::Camview():
		view(tString("")), camMode(CAMERA_IN),
        dirAngle(0), dirCos(0), dirSin(0), 
        posTranslation(tCoord(0,0)),
        z(0), rise(0), fov(0),
        mirror(false),
        mainCameraFlag(false), 
        mainCameraDirFlag(false), 
        dirAngleFlag(false),
        posTranslationFlag(false), 
        riseFlag(false), fovFlag(false),ReadyFlag(false)
{
    for ( int viewport = MAX_VIEWPORTS-1; viewport >= 0; --viewport )
    {
    	viewports[viewport] = 0;
    	cam[viewport] = 0;
	}
}

Camview::~Camview()
{
    rViewportConfiguration* viewportConfiguration = rViewportConfiguration::CurrentViewportConfiguration();
    for ( int viewport = viewportConfiguration->num_viewports-1; viewport >= 0; --viewport )
    {
    	// suppress references
        if( viewports[viewport] )
        {
            viewports[viewport]->SetRootViewport(0);
            // suppress widget cams and viewports
            delete cam[viewport];
            delete viewports[viewport];
            cam[viewport] = 0;
            viewports[viewport] = 0;
        }
	}
}

}
#endif
