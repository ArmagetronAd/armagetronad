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

// Classic/moviepack loading extracted from gCycle.cpp. GPL version 2 or later.
#include "gCycleVisuals.h"
#include "rModel.h"
#include "rScreen.h"
#include "tMemManager.h"
#include <memory>
#include <utility>

void gTextureCycle::SetColor(rColor const& color)
{
    if (color_ != color)
    {
        color_ = color;
        Unload();
    }
}

gTextureCycle::gTextureCycle(rSurface const& surface, const rColor& color, bool repx, bool repy, bool w)
    : rSurfaceTexture(rTextureGroups::TEX_OBJ, surface, repx, repy),
      color_(color), wheel(w)
{
    Select();
}

void gTextureCycle::ProcessImage(SDL_Surface* im)
{
#ifndef DEDICATED
    // blend transparent texture parts with cycle color
    tVERIFY(im->format->BytesPerPixel == 4);
    GLubyte R = int(color_.r_ * 255);
    GLubyte G = int(color_.g_ * 255);
    GLubyte B = int(color_.b_ * 255);

    GLubyte* pixels = reinterpret_cast<GLubyte*>(im->pixels);

    for (int i = im->w * im->h - 1; i >= 0; i--)
    {
        GLubyte alpha = pixels[4 * i + 3];
        pixels[4 * i] = (alpha * pixels[4 * i] + (255 - alpha) * R) >> 8;
        pixels[4 * i + 1] = (alpha * pixels[4 * i + 1] + (255 - alpha) * G) >> 8;
        pixels[4 * i + 2] = (alpha * pixels[4 * i + 2] + (255 - alpha) * B) >> 8;
        pixels[4 * i + 3] = 255;
    }
#endif
}

void gTextureCycle::OnSelect(bool enforce)
{
#ifndef DEDICATED
    rISurfaceTexture::OnSelect(enforce);

    if (rTextureGroups::TextureMode[rTextureGroups::TEX_OBJ] < 0)
    {
        REAL R = color_.r_, G = color_.g_, B = color_.b_;
        if (wheel)
        {
            R *= .7;
            G *= .7;
            B *= .7;
        }
        glColor3f(R, G, B);
        GLfloat color[4] = {R, G, B, 1};

        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, color);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, color);
    }
#endif
}

#ifndef DEDICATED
gCycleVisuals::gCycleVisuals(rColor const& a_color)
{
    customModel = bodyModel = frontModel = rearModel = 0;
    customTexture = bodyTexture = wheelTexture = 0;

    color = a_color;

    mpType = false;
    mpPreference = 0;
}

gCycleVisuals::~gCycleVisuals()
{
    delete customTexture;
    delete bodyTexture;
    delete wheelTexture;
}

// loads a specific texture from a specific folder
rSurface* gCycleVisuals::LoadTextureSafe2(Slot slot, int mp)
{
    static std::unique_ptr<rSurface> cache[SLOT_MAX][2];
    std::unique_ptr<rSurface>& surface = cache[slot][mp];
    if (surface.get() == NULL)
    {
        static char const* names[SLOT_MAX] = {"bike.png", "cycle_body.png", "cycle_wheel.png"};
        char const* name = names[slot];

        char const* folder = mp ? "moviepack" : "textures";
        tString file = tString(folder) + tString("/") + tString(name);

        surface.reset(tNEW(rSurface(file)));
    }

    if (surface->GetSurface())
        return surface.get();
    else
        return NULL;
}

// load one texture from the prefered folder (or the other one as fallback)
gTextureCycle* gCycleVisuals::LoadTextureSafe(Slot slot, bool wheel)
{
    rSurface* surface = LoadTextureSafe2(slot, mpPreference);
    if (!surface)
        surface = LoadTextureSafe2(slot, 1 - mpPreference);

    if (surface)
        return tNEW(gTextureCycle)(*surface, color, 0, 0, wheel);

    return NULL;
}

// load textures from the specified folder (or the other one) and the format the model has just been read for
bool gCycleVisuals::LoadTextures()
{
    if (mpType)
    {
        if (!customTexture)
            customTexture = LoadTextureSafe(SLOT_CUSTOM, false);

        return customTexture;
    }
    else
    {
        if (!bodyTexture)
            bodyTexture = LoadTextureSafe(SLOT_BODY, false);
        if (!wheelTexture)
            wheelTexture = LoadTextureSafe(SLOT_WHEEL, true);

        return bodyTexture && wheelTexture;
    }
}

// loads a model, checking before if the file exists
rModel* gCycleVisuals::LoadModelSafe(char const* filename)
{
    return rModel::GetModel(filename);
}

// load a model of specified type from a specified directory
bool gCycleVisuals::LoadModel(bool a_mpType, bool mpFolder)
{
    mpType = a_mpType;
    char const* folder = mpFolder ? "moviepack" : "models";

    if (mpType)
    {
        tString base = tString(folder);
        base << "/cycle";
        if (!customModel)
            customModel = LoadModelSafe(base + ".ASE");
        if (!customModel)
            customModel = LoadModelSafe(base + ".ase");

        return customModel && LoadTextures();
    }
    else
    {
        tString base = tString(folder) + "/cycle_";

        if (!bodyModel)
            bodyModel = LoadModelSafe(base + "body.mod");
        if (!frontModel)
            frontModel = LoadModelSafe(base + "front.mod");
        if (!rearModel)
            rearModel = LoadModelSafe(base + "rear.mod");

        return bodyModel && frontModel && rearModel && LoadTextures();
    }
}

// tries to load everything from the given data folder
bool gCycleVisuals::LoadModel2(bool mp)
{
    // first, try the right type, then the 'unnatural' choice
    return LoadModel(mp, mp) || LoadModel(!mp, mp);
}

// top level load function: tries to load all variations, starting with passed moviepack folder flag
bool gCycleVisuals::LoadModel(bool mp)
{
    mpPreference = mp ? 1 : 0;

    // delegate to try loading both formats from both directories
    return LoadModel2(mp) || LoadModel2(!mp);
}

void gCycleVisuals::Commit(rModel*& body, rModel*& front, rModel*& rear, rModel*& custom,
                           gTextureCycle*& bodyTex, gTextureCycle*& wheelTex, gTextureCycle*& customTex,
                           bool& moviepack)
{
    body = bodyModel;
    front = frontModel;
    rear = rearModel;
    custom = customModel;
    std::swap(bodyTex, bodyTexture);
    std::swap(wheelTex, wheelTexture);
    std::swap(customTex, customTexture);
    moviepack = mpType;
}
#endif
