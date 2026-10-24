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

// Cached Classic/moviepack resources. GPL version 2 or later.
#ifndef ArmageTron_CYCLE_VISUALS_H
#define ArmageTron_CYCLE_VISUALS_H
#include "rTexture.h"
#include "rColor.h"

class rModel;

class gTextureCycle : public rSurfaceTexture
{
    rColor color_;
    bool wheel;

public:
    gTextureCycle(rSurface const& surface, rColor const& color, bool repx = false, bool repy = false, bool wheel = false);
    void ProcessImage(SDL_Surface* image) override;
    void OnSelect(bool enforce) override;
    using rSurfaceTexture::OnSelect;
    void SetColor(rColor const& color);
};

#ifndef DEDICATED
// Models are borrowed from rModel's cache; this object owns its textures until
// Commit swaps them into a bike. A failed load leaves that bike untouched.
struct gCycleVisuals
{
    rModel *customModel, *bodyModel, *frontModel, *rearModel;
    gTextureCycle *customTexture, *bodyTexture, *wheelTexture;
    rColor color;
    bool mpType;
    int mpPreference;
    explicit gCycleVisuals(rColor const& color);
    ~gCycleVisuals();
    gCycleVisuals(gCycleVisuals const&) = delete;
    gCycleVisuals& operator=(gCycleVisuals const&) = delete;
    bool LoadModel(bool moviepack);
    void Commit(rModel*& body, rModel*& front, rModel*& rear, rModel*& custom,
                gTextureCycle*& bodyTex, gTextureCycle*& wheelTex, gTextureCycle*& customTex,
                bool& moviepack);

private:
    enum Slot
    {
        SLOT_CUSTOM,
        SLOT_BODY,
        SLOT_WHEEL,
        SLOT_MAX
    };
    static rSurface* LoadTextureSafe2(Slot slot, int moviepack);
    gTextureCycle* LoadTextureSafe(Slot slot, bool wheel);
    bool LoadTextures();
    static rModel* LoadModelSafe(char const* filename);
    bool LoadModel(bool format, bool moviepackFolder);
    bool LoadModel2(bool moviepack);
};
#endif
#endif
