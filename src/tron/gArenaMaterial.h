#ifndef ARMAGETRON_ARENA_MATERIAL_H
#define ARMAGETRON_ARENA_MATERIAL_H

#include "gStuff.h"

#ifndef DEDICATED
#include "rTexture.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

// Procedural materials use the normal texture cache and context-loss handling.
class gArenaMaterial : public rISurfaceTexture
{
public:
    enum Kind
    {
        Floor,
        Trail,
        Rim
    };
    explicit gArenaMaterial(Kind kind)
        : rISurfaceTexture(kind == Floor ? rTextureGroups::TEX_FLOOR : rTextureGroups::TEX_WALL,
                           true, kind == Floor, true),
          kind_(kind) {}

private:
    Kind kind_;
    class Surface : public rSurface
    {
    public:
        explicit Surface(SDL_Surface* surface) { Create(surface); }
    };

    static float Line(float distance, float width)
    {
        return std::max(0.f, 1.f - distance / width);
    }

    void OnSelectCore() override
    {
        // Preserve narrow panel edges and rounded corners at Retina resolution.
        int const size = kind_ == Floor ? 4096 : 2048;
        SDL_Surface* raw = SDL_CreateRGBSurface(SDL_SWSURFACE, size, size, 32,
                                                0x000000ff, 0x0000ff00,
                                                0x00ff0000, 0xff000000);
        if (!raw)
            throw std::runtime_error("Could not allocate arena material");
        Surface surface(raw);
        for (int y = 0; y < size; ++y)
        {
            Uint32* row = reinterpret_cast<Uint32*>(static_cast<Uint8*>(raw->pixels) + y * raw->pitch);
            for (int x = 0; x < size; ++x)
            {
                float intensity;
                Uint8 alpha = 255;
                if (kind_ == Floor)
                {
                    // Smoked glass with recessed joints, a polished bevel and
                    // fine etched registration marks. Circuit inlays are drawn
                    // in world space by the floor material, above this quiet grain.
                    float const tiles = 4.f, tile = 512.f;
                    float fx = float(x) * tiles / size, fy = float(y) * tiles / size;
                    int ix = int(fx), iy = int(fy);
                    float lx = (fx - ix) * tile, ly = (fy - iy) * tile;
                    float dx = std::min(lx, tile - lx), dy = std::min(ly, tile - ly);
                    float edge = std::min(dx, dy);
                    float seam = Line(edge, 1.4f);
                    float shoulder = Line(edge, 5.f);
                    float bevel = Line(std::fabs(edge - 7.f), 1.8f);
                    float stud = Line(dx, 4.f) * Line(dy, 4.f);
                    unsigned hash = unsigned(ix * 73856093) ^ unsigned(iy * 19349663);
                    hash = (hash ^ (hash >> 13)) * 1274126177u;
                    float shade = float(hash & 255u) / 255.f;
                    float grain = .5f + .5f * std::sin(lx * 1.7f + std::sin(ly * .047f));
                    float tick = Line(std::fabs(dx - 21.f), 1.2f) * Line(dy, 7.f) + Line(std::fabs(dy - 21.f), 1.2f) * Line(dx, 7.f);
                    intensity = .085f + .012f * shade + .004f * grain + .20f * seam + .025f * shoulder + .055f * bevel + .24f * stud + .11f * tick;
                }
                else if (kind_ == Trail)
                {
                    // A transparent energy sheet between two narrow optical
                    // edges. Greyscale modulation preserves every player's hue;
                    // the field adds light without painting an opaque panel.
                    float v = float(y) / (size - 1);
                    float u = float(x) / size;
                    float top = std::exp(-v * 85.f);
                    float bottom = std::exp(-(1.f - v) * 60.f);
                    float shoulder = std::exp(-v * 14.f);
                    float foot = std::exp(-(1.f - v) * 10.f);
                    float weave = .5f + .5f * std::sin(v * 201.06193f + .18f * std::sin(u * 6.2831853f));
                    float edge = std::max(top, bottom);
                    intensity = .70f + .012f * weave + .11f * shoulder + .07f * foot + .30f * edge;
                    float opacity = .34f + .008f * weave + .16f * shoulder + .10f * foot + .64f * edge;
                    alpha = Uint8(std::min(1.f, opacity) * 255.f + .5f);
                }
                else
                {
                    // Architectural panels with a bright lower rail and a
                    // restrained second seam above it.
                    float v = float(y) / (size - 1);
                    float panel = std::fmod(float(x) * 512.f / size, 128.f);
                    float seam = Line(std::min(panel, 128.f - panel), 2.f);
                    float rail = std::fabs(v - .985f);
                    float upper = std::fabs(v - .93f);
                    intensity = .035f + .02f * v + .025f * seam + .94f * Line(rail, .0035f) + .16f * Line(rail, .016f) + .19f * Line(upper, .0025f);
                }
                Uint8 light = Uint8(std::min(1.f, intensity) * 255.f + .5f);
                row[x] = SDL_MapRGBA(raw->format, light, light, light, alpha);
            }
        }
        Upload(surface);
    }
};
#endif
#endif
