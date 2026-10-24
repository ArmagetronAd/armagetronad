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

#include "gSparks.h"
#include "gStuff.h"
#include "eTimer.h"
#include "rRender.h"
#include "tRandom.h"
#include <algorithm>
#include <cmath>

#ifdef USEPARTICLES
#include "papi.h"
#endif

bool white_sparks=false;

gSpark::gSpark(eGrid *grid, const eCoord &pos,const eCoord &dir,REAL time,REAL ocolor_r,REAL ocolor_g,REAL ocolor_b,REAL ecolor_r,REAL ecolor_g,REAL ecolor_b)
        :eReferencableGameObject(grid, pos, dir , NULL, true),
        //   sound(scrap),
createTime(time){
    lastTime=createTime;

#ifndef USEPARTICLES
    sparkowncolor_r=ocolor_r;
    sparkowncolor_g=ocolor_g;
    sparkowncolor_b=ocolor_b;

    sparkenemycolor_r=ecolor_r;
    sparkenemycolor_g=ecolor_g;
    sparkenemycolor_b=ecolor_b;

    for (int i=SPARKS-1;i>=0;i--){
        lastX[i]=preLastX[i]=x[i]=Vec3(pos.x,pos.y,.5);

        static const REAL fak=4;

        tRandomizer & randomizer = tRandomizer::GetInstance();
        REAL a=fak*( randomizer.Get() - .5f );
        REAL b=fak*( randomizer.Get() - .5f );
        //      REAL a=fak*(rand()/static_cast<REAL>(RAND_MAX)-.5f);
        //      REAL b=fak*(rand()/static_cast<REAL>(RAND_MAX)-.5f);
        REAL c=1;

        eCoord xy(eCoord(c,b).Turn(dir));

        xDot[i]=Vec3(xy.x,xy.y,a);
        xDot[i]=xDot[i]*(1/xDot[i].Norm());
        xDot[i].x[2]+=1;

        heat[i]=2+randomizer.Get();
        //      heat[i]=2+rand()/REAL(RAND_MAX);
        lastBreak[i]=createTime;
    }
#else
#ifndef DEDICATED
    particle_handle = pGenParticleGroups(1, SPARKS);

    pCurrentGroup(particle_handle);
    // Generate particles along a very small line in the nozzle.
    pSource(SPARKS, PDLine(pVec(Position().x, Position().y, 0.3), pVec(Position().x, Position().y, 0.8)));
#endif
#endif
    // add to game grid
    this->AddToList();
}

gSpark::~gSpark(){  }

// virtual eGameObject_type type();

bool gSpark::Timestep(REAL currentTime){
#ifndef USEPARTICLES
    REAL ts=currentTime-lastTime;
    lastTime=currentTime;

    for (int i=SPARKS-1;i>=0;i--){
        x[i]+=xDot[i]*ts;
        xDot[i].x[2]-=5*ts;
        heat[i]-=ts;

        if (x[i].x[2]<0){
            x[i].x[2]*=-1;
            xDot[i].x[2]*=-.5;
            lastBreak[i]=currentTime;
        }
    }

    if (currentTime>createTime+4)
        return true;
    else
        return false;
#else
#ifndef DEDICATED
    pCurrentGroup(particle_handle);
    // Set up the state.
    pVelocityD(PDCylinder(pVec(0.0, 0.0, 0.0), pVec(0.0, 0.0, 0.01), 0.01, 0.007));
    pColorD(PDLine(pVec(1.0, 1.0, 1.0), pVec(1.0, 1.0, 1.0)));
    pSize(40.0);
    //pStartingAge(0);

    //pRandomAccel(PDSphere(pVec(Position().x, Position().y, 0.0), 100.0));

    // Gravity.
    pGravity(pVec(0.0, 0.0, -0.001));

    // Bounce particles off a disc of radius 5.
    pBounce(-0.05, 1.0, 0.1, PDDisc(pVec(0, 0, 0), pVec(0, 0, 1), 5000000));

    // Kill particles below Z=0.
    pSink(false, PDPlane(pVec(0,0,0), pVec(0,0,1)));

    pKillOld(40);

    // Move particles to their new positions.
    pMove();

    // Finished when there are less than 1/10 of the original particles
    if (pGetGroupCount() < SPARKS/10) {
        pDeleteParticleGroups(particle_handle, particle_handle);
        return true;
    }
#endif // dedicated
#endif // particles

    lastTime=currentTime;

    return false;
}

void gSpark::InteractWith(eGameObject *,REAL ,int){}
eGameObject::ePassEdgeResult gSpark::PassEdge(const eWall *,REAL ,REAL ,int){return eContinue;}

void gSpark::Kill(){createTime=lastTime-100000;}


#ifndef DEDICATED
void gSpark::Render(const eCamera *cam){
    if (sg_blacklightVisuals && !sg_MoviePack())
    {
        REAL age = lastTime - createTime;
        if (!std::isfinite(age) || age < 0 || age >= .65f)
            return;
        REAL fade = 1 - age / .65f;
        fade *= fade;
        RenderEnd();
        glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT |
                     GL_DEPTH_BUFFER_BIT | GL_TEXTURE_BIT | GL_LINE_BIT |
                     GL_POLYGON_BIT | GL_LIGHTING_BIT);
#ifdef HAVE_GLEW
        GLint program = 0, texture = GL_TEXTURE0;
        if (GLEW_VERSION_2_0)
        {
            glGetIntegerv(GL_CURRENT_PROGRAM, &program);
            glUseProgram(0);
        }
        if (GLEW_VERSION_1_3)
        {
            glGetIntegerv(GL_ACTIVE_TEXTURE, &texture);
            GLint units = 1;
            glGetIntegerv(GL_MAX_TEXTURE_UNITS, &units);
            for (GLint unit = 0; unit < units; ++unit)
            {
                glActiveTexture(GL_TEXTURE0 + unit);
                glDisable(GL_TEXTURE_1D);
                glDisable(GL_TEXTURE_2D);
                glDisable(GL_TEXTURE_3D);
                glDisable(GL_TEXTURE_CUBE_MAP);
            }
            glActiveTexture(GL_TEXTURE0);
        }
        if (GLEW_VERSION_1_4)
            glBlendEquation(GL_FUNC_ADD);
#endif
        glDisable(GL_TEXTURE_1D);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_LIGHTING);
        glDisable(GL_ALPHA_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_LINE_SMOOTH);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glShadeModel(GL_SMOOTH);
        glLineWidth(1.25f);
#ifndef USEPARTICLES
        BeginLines();
        for (int i = 0; i < SPARKS; ++i)
        {
            REAL ago = std::max(REAL(0), std::min(REAL(.05f), lastTime - lastBreak[i]));
            REAL r = i % 2 ? sparkowncolor_r : sparkenemycolor_r;
            REAL g = i % 2 ? sparkowncolor_g : sparkenemycolor_g;
            REAL b = i % 2 ? sparkowncolor_b : sparkenemycolor_b;
            if (white_sparks)
            {
                r = 1;
                g = .88f;
                b = .62f;
            }
            glColor4f(r, g, b, .95f * fade);
            glVertex3f(x[i].x[0], x[i].x[1], x[i].x[2]);
            glColor4f(r, g, b, 0);
            glVertex3f(x[i].x[0] - xDot[i].x[0] * ago * .8f,
                       x[i].x[1] - xDot[i].x[1] * ago * .8f,
                       std::max(REAL(.006f), REAL(x[i].x[2] - xDot[i].x[2] * ago * .8f)));
        }
#else
        // Keep optional particle simulation untouched; only a small, bounded
        // read-only subset supplies its short visual streaks.
        pCurrentGroup(particle_handle);
        float* ptr = NULL;
        size_t stride, pos3, posB3, size3, velocity3, velocityB3, colour3, alpha1, age1;
        int count = pGetParticlePointer(ptr, stride, pos3, posB3, size3, velocity3, velocityB3, colour3, alpha1, age1);
        BeginLines();
        for (int i = 0; i < std::min(count, 16); ++i)
        {
            float const* particle = ptr + i * stride;
            float const* p = particle + pos3;
            float const* velocity = particle + velocity3;
            float const* colour = particle + colour3;
            if (!std::isfinite(p[0]) || !std::isfinite(p[1]) || !std::isfinite(p[2]) ||
                !std::isfinite(velocity[0]) || !std::isfinite(velocity[1]) || !std::isfinite(velocity[2]) ||
                (p[0] - pos.x) * (p[0] - pos.x) + (p[1] - pos.y) * (p[1] - pos.y) > 4 || p[2] < 0 || p[2] > 2)
                continue;
            float speed = std::sqrt(velocity[0] * velocity[0] + velocity[1] * velocity[1] + velocity[2] * velocity[2]);
            float tail = std::min(.05f, .12f / std::max(.001f, speed));
            glColor4f(colour[0], colour[1], colour[2], fade);
            glVertex3f(p[0], p[1], p[2]);
            glColor4f(colour[0], colour[1], colour[2], 0);
            glVertex3f(p[0] - velocity[0] * tail, p[1] - velocity[1] * tail,
                       std::max(.006f, p[2] - velocity[2] * tail));
        }
#endif
        RenderEnd();
        glPopAttrib();
#ifdef HAVE_GLEW
        if (GLEW_VERSION_2_0)
            glUseProgram(program);
        if (GLEW_VERSION_1_3)
            glActiveTexture(texture);
#endif
        return;
    }
#ifndef USEPARTICLES
    glBlendFunc(GL_SRC_ALPHA,GL_ONE);

    //glMatrixMode(GL_MODELVIEW);
    //glPushMatrix();
    //glLoadIdentity();

    //glDisable(GL_TEXTURE);
    glDisable(GL_TEXTURE_2D);

    BeginLines();
    for (int i=SPARKS-1;i>=0;i--){
    #define rmax 1.2
    #define gmax 1.1
        REAL ago=.2;
        if (ago>se_GameTime()-lastBreak[i])
            ago=se_GameTime()-  lastBreak[i];

        REAL a=heat[i]+1.5;
        if (a>1) a=1;
        if (a<0) a=0;

        if(!white_sparks) {
            if(i%2)
                glColor4f(sparkowncolor_r,sparkowncolor_g,sparkowncolor_b,a);
            else
                glColor4f(sparkenemycolor_r,sparkenemycolor_g,sparkenemycolor_b,a);
        }
        else {
            REAL r=heat[i]+1;
            if (r>rmax) r=rmax;
            if (r>1) r=2-r;
            if (r<0) r=0;
            REAL g=heat[i]+.5;
            if (g>gmax) g=gmax;
            if (g>1) g=2-g;
            if (g<0) g=0;
            REAL b=heat[i];
            if (b>1) b=1;
            if (b<0) b=0;

            glColor4f(r,g,b,a);
        }

        x[i].RenderVertex();
        preLastX[i]=x[i];
        preLastX[i]+=xDot[i]*(-ago*.8);

        preLastX[i].RenderVertex();
        preLastX[i]=lastX[i];
        lastX[i]=x[i];
    }
    RenderEnd();
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    //glPopMatrix();
#else
    pCurrentGroup(particle_handle);
    int cnt = (int)pGetGroupCount();
    if(cnt < 1) return;

    float *ptr;
    size_t flstride, pos3Ofs, posB3Ofs, size3Ofs, vel3Ofs, velB3Ofs, color3Ofs, alpha1Ofs, age1Ofs;

    cnt = (int)pGetParticlePointer(ptr, flstride, pos3Ofs, posB3Ofs,
                                   size3Ofs, vel3Ofs, velB3Ofs, color3Ofs, alpha1Ofs, age1Ofs);
    if(cnt < 1) return;

    glEnableClientState(GL_COLOR_ARRAY);
    glColorPointer(4, GL_FLOAT, int(flstride) * sizeof(float), ptr + color3Ofs);

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, int(flstride) * sizeof(float), ptr + pos3Ofs);

    glDrawArrays(GL_POINTS, 0, cnt);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
#endif
}

/*
void gSpark::SoundMix(Sint16 *dest,unsigned int len,
                      int viewer,REAL rvol,REAL lvol, REAL dopplerPitch){
    //  sound.Mix(dest,len,viewer,rvol*.5,lvol*.5,4);
}*/
#endif
