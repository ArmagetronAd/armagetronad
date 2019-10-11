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

#include "tMemManager.h"
#include "gCycle.h"
#include "gCamera.h"
#include "gGame.h"
#include "ePlayer.h"
#include "rFont.h"
#include "eSoundMixer.h"

void gCamera::MyInit(){
    if (mode == CAMERA_SMART)
        if (sg_currentSettings->finishType==gFINISH_EXPRESS && sn_GetNetState() != nCLIENT )
            pos=CenterPos()+dir.Turn(eCoord(-2,-10)) ;

    lastCenter=NULL;
}

gCamera::gCamera(eGrid *grid, rViewport *view,ePlayerNetID *p,ePlayer *lp,eCamMode m)
        :eCamera(grid, view,p,lp,m), lastCenter(NULL){
    MyInit();
}


gCamera::~gCamera(){
#ifdef DEBUG
    int x;
    x =0;
#endif
}

eCoord gCamera::CenterCycleDir() const {
    gCycle *c = dynamic_cast<gCycle *>( Center());
    if (c)
        return c->CamDir();
    else
        return CenterDir();
}

REAL gCamera::SpeedMultiplier() const
{
    return gCycle::SpeedMultiplier();
}

void gCamera::Timestep(REAL ts){
    eCamera::Timestep(ts);
    if (Center()!=lastCenter){
        if (!netPlayer || !netPlayer->Object() ||
                netPlayer->Object()!=center){
            if (dynamic_cast<gCycle *>(Center())){
                eNetGameObject *x=dynamic_cast<eNetGameObject *>(Center());
                if (x){
                    const ePlayerNetID *p=x->Player();
                    if (p==NULL)
                        con << tOutput("$camera_watching_ai");
                    else{
                        tOutput o;
                        o.SetTemplateParameter(1, p->GetName());
                        o << "$camera_watching_player";
                        con << o;
                    }
                }
            }
        }

        lastCenter=Center();

        eSoundMixer* mixer = eSoundMixer::GetMixer();
        mixer->SetMicrophoneOwner(Center());
    }
}






