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
//! @brief Contains the class for rendering the camera widget

#ifndef ARMAGETRON_CCAMVIEW_H
#define ARMAGETRON_CCAMVIEW_H

#include "cockpit/cWidgetBase.h"

#ifndef DEDICATED

#include "ePlayer.h"
#include "eCamera.h"

namespace cWidget {

//! Processes and renders a camera "subview" of the grid
class Camview : public WithCoordinates, public WithBackground, public WithForeground {
	
	// the subviewport and related camera ... 
 	rViewport 				*viewports[MAX_VIEWPORTS];
 	tCHECKED_PTR(eCamera)	cam[MAX_VIEWPORTS];

	// widget settings
	tString view;
	eCamMode camMode;
	REAL dirAngle, dirCos, dirSin;	//! Angle, cos and sin of the rotation to apply on camera direction (lookat)
	tCoord posTranslation;			//! translation to be apply on camera position
	REAL z,rise;    				//! height above the floor and whether we look up or down
    REAL fov;       				//! field of vision;
	bool mirror;					//! horizontal reflection (mirror view)
	bool mainCameraFlag;			//! render main player camera in the widget
	bool mainCameraDirFlag;			//! use main camera direction to deduce view direction
 	bool dirAngleFlag, posTranslationFlag, riseFlag, fovFlag; //! flag settings to be overwritten ...
 	bool ReadyFlag;						//! flag to indicate whether the widget is ready or need to be reinit (cam and viewport) 
public:
    void Render();
    void CreateCamera(int vp); 
    virtual bool Process(tXmlParser::node cur); //!< Passes on to all Process() functions of the base classes and calls Base::DisplayError() on failure
	virtual void PostParsingProcess();
    virtual void BeforeRoundProcess();
    virtual void AfterRoundProcess();
    Camview(); 								//!< default constructor
    ~Camview(); 							//!< default destructor
};

}

#endif
#endif
