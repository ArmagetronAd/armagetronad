// Cycle assembly, ground illumination and optical effects. GPL version 2 or later.
#include "gLightcycleEffects.h"
#include "gLightcycleModel.h"
#define DONTDOIT
#include "aa_config.h"
#ifndef DEDICATED
#include "rGL.h"
#include "rRender.h"
#include <algorithm>
#include <cmath>

namespace gLightcycleEffects
{
void Render(float r, float g, float b, float seconds, float speed,
            bool braking, float bank, float birthAge)
{
    if (!std::isfinite(seconds) || !std::isfinite(speed) || !std::isfinite(bank))
        return;
    RenderEnd();
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT |
                 GL_DEPTH_BUFFER_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_POLYGON_BIT);
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
    glDisable(GL_POLYGON_OFFSET_FILL);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glShadeModel(GL_SMOOTH);
    float const pi = 3.14159265358979323846f;
    float drive = std::max(0.f, std::min(1.f, (speed - 4.f) / 40.f));
    float clock = std::fmod(seconds, 2048.f);
    auto colour = [&](float alpha, float hot = 0.f)
    {
        glColor4f(r + (1 - r) * hot, g + (1 - g) * hot, b + (1 - b) * hot, alpha);
    };

    // Short wisps peel off the emitter. Their longitudinal travel follows time,
    // while wheel registers follow distance; a stopped cycle has no speed wake.
    for (int i = 0; i < 7 && drive > 0; ++i)
    {
        float phase = clock * (1.2f + .05f * i) + i * .6180339f;
        phase -= std::floor(phase);
        float side = i % 2 ? -1.f : 1.f;
        float x = -1.88f - phase * (.9f + drive);
        float y = side * (.07f + .13f * phase);
        float z = .20f + .10f * (i % 4) + .025f * std::sin(clock * 3 + i);
        float width = .009f + .009f * (1 - phase);
        float length = .15f + .27f * drive;
        float fade = std::sin(phase * pi) * drive * .30f;
        BeginQuads();
        colour(0);
        glVertex3f(x - length, y, z - width);
        colour(fade, .25f);
        glVertex3f(x, y, z - width);
        colour(fade, .25f);
        glVertex3f(x, y, z + width);
        colour(0);
        glVertex3f(x - length, y, z + width);
    }
    RenderEnd();

    // A brief curved contact glint follows visual banking. It is illumination
    // on the glass, never a second trail or a change to the collision corner.
    float corner = std::min(1.f, std::fabs(bank) * 5.f);
    if (corner > .015f || braking)
    {
        float side = bank < 0 ? -1.f : 1.f;
        for (int wheel = 0; wheel < 2; ++wheel)
        {
            float axle = wheel ? 1.55f : -1.45f;
            BeginTriangleStrip();
            for (int i = 0; i <= 24; ++i)
            {
                float u = i / 24.f, a = pi * (.12f + .75f * u);
                float fade = std::sin(u * pi) * (corner * .28f + (braking ? .10f : 0.f));
                float x = axle - .80f * std::sin(a);
                float y = side * .76f * std::cos(a);
                colour(fade, .12f);
                glVertex3f(x, y, .024f);
                colour(0);
                glVertex3f(x - .12f, y + side * .16f, .024f);
            }
            RenderEnd();
        }
    }

    // Wheel hoops establish the vehicle first. The chassis grows between them,
    // then these temporary construction lines resolve into the actual shell.
    // Absolute birth age keeps the reflection and main image in the same stage.
    if (std::isfinite(birthAge) && birthAge >= 0 && birthAge < 1.25f)
    {
        float u = birthAge / 1.25f;
        auto smooth = [](float a, float b, float x)
        {
            x = std::max(0.f, std::min(1.f, (x - a) / (b - a)));
            return x * x * (3 - 2 * x);
        };
        float resolve = 1 - smooth(.64f, 1.f, u);
        for (int wheel = 0; wheel < 2; ++wheel)
        {
            float progress = smooth(.015f + .04f * wheel, .50f + .04f * wheel, u);
            float axle = wheel ? 1.55f : -1.45f;
            for (int ring = 0; ring < 3; ++ring)
            {
                float side = (ring - 1) * .44f;
                float radius = .72f + (.20f + .06f * ring) * (1 - progress);
                BeginTriangleStrip();
                for (int i = 0; i <= 96; ++i)
                {
                    float f = i / 96.f;
                    float a = 2 * pi * (f * progress + .10f * ring + (wheel ? u * .25f : -u * .25f));
                    float edge = std::min(1.f, (1 - f) * 12.f);
                    colour(resolve * edge * .75f, .52f);
                    glVertex3f(axle + (radius - .010f) * std::cos(a), side, .72f + (radius - .010f) * std::sin(a));
                    glVertex3f(axle + (radius + .010f) * std::cos(a), side, .72f + (radius + .010f) * std::sin(a));
                }
                RenderEnd();
            }
        }
        float chassis = smooth(.18f, .67f, u);
        for (int rail = 0; rail < 12; ++rail)
        {
            float a = 2 * pi * rail / 12.f;
            float y = .40f * std::cos(a), z = .80f + .28f * std::sin(a);
            BeginTriangleStrip();
            for (int i = 0; i <= 24; ++i)
            {
                float f = i / 24.f, x = -1.45f + 3.00f * f * chassis;
                float bow = std::sin(f * pi) * .12f;
                float alpha = resolve * std::min(1.f, chassis * 5.f) * (.25f + .35f * (1 - f));
                colour(alpha, .25f);
                glVertex3f(x, y - bow * .4f, z + bow - .006f);
                glVertex3f(x, y - bow * .4f, z + bow + .006f);
            }
            RenderEnd();
        }
    }

    glPopAttrib();
#ifdef HAVE_GLEW
    if (GLEW_VERSION_2_0)
        glUseProgram(program);
    if (GLEW_VERSION_1_3)
        glActiveTexture(texture);
#endif
}
void GroundLight(float r, float g, float b, float assembly)
{
    RenderEnd();
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                 GL_CURRENT_BIT | GL_TEXTURE_BIT | GL_POLYGON_BIT | GL_LIGHTING_BIT);
#ifdef HAVE_GLEW
    GLint oldProgram = 0;
    if (GLEW_VERSION_2_0)
    {
        glGetIntegerv(GL_CURRENT_PROGRAM, &oldProgram);
        glUseProgram(0);
    }
#endif
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    glShadeModel(GL_SMOOTH);

    // Fit contact occlusion to the actual axles and chassis. This replaces the
    // old bike's rectangular shadow; it follows visual yaw without banking off
    // the floor. Fade it with assembly so no full-bike shadow precedes the bike.
    auto contact = [&](float x, float radiusX, float radiusY, float opacity)
    {
        for (int band = 0; band < 6; ++band)
        {
            float inner = band / 6.f, outer = (band + 1) / 6.f;
            glBegin(GL_QUAD_STRIP);
            for (int i = 0; i <= 40; ++i)
            {
                float a = i * (2.f * M_PI / 40.f), c = std::cos(a), s = std::sin(a);
                auto vertex = [&](float radius)
                {
                    float fade = 1.f - radius * radius;
                    glColor4f(0, 0, 0, opacity * assembly * fade * fade);
                    glVertex3f(x + radiusX * radius * c, radiusY * radius * s, .010f);
                };
                vertex(inner);
                vertex(outer);
            }
            glEnd();
        }
    };
    contact(.05f, 1.65f, .58f, .25f);
    contact(gLightcycleModel::RearAxle, .52f, .58f, .64f);
    contact(gLightcycleModel::FrontAxle, .52f, .58f, .64f);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    // Smooth local pools, strongest at the wheels and falling to zero before
    // they reach the next cycle. They follow heading but stay on the ground
    // when the visual bike leans into a turn.
    auto pool = [&](float x, float radiusX, float radiusY, float power)
    {
        auto energy = [power, assembly](float r)
        { return power * assembly * std::exp(-4.f * r * r) * (1.f - r * r); };
        for (int band = 0; band < 6; ++band)
        {
            float inner = band / 6.f, outer = (band + 1) / 6.f;
            glBegin(GL_QUAD_STRIP);
            for (int i = 0; i <= 48; ++i)
            {
                float a = i * (2.f * M_PI / 48.f), c = std::cos(a), s = std::sin(a);
                glColor4f(r, g, b, energy(inner));
                glVertex3f(x + radiusX * inner * c, radiusY * inner * s, .014f);
                glColor4f(r, g, b, energy(outer));
                glVertex3f(x + radiusX * outer * c, radiusY * outer * s, .014f);
            }
            glEnd();
        }
    };
    pool(gLightcycleModel::RearAxle, 1.25f, .95f, .055f);
    pool(gLightcycleModel::FrontAxle, 1.25f, .95f, .055f);
    pool(.15f, 1.55f, .78f, .015f);

    // The white nose lamps project a short, soft cone onto the polished floor.
    // This is local illumination; it does not blur the scene or HUD.
    auto beamVertex = [assembly](float u, float v)
    {
        float width = .24f + u * .90f;
        float fade = (1.f - u) * (1.f - u) * std::exp(-2.5f * u);
        fade *= std::min(1.f, u * 18.f) * (1.f - v * v) * (1.f - v * v);
        glColor4f(.68f, .88f, 1.f, .12f * fade * assembly);
        glVertex3f(gLightcycleModel::FrontAxle + .62f + u * 6.f, v * width, .016f);
    };
    for (int row = 0; row < 20; ++row)
    {
        float u0 = row / 20.f, u1 = (row + 1) / 20.f;
        glBegin(GL_QUAD_STRIP);
        for (int col = 0; col <= 16; ++col)
        {
            float v = -1.f + col / 8.f;
            beamVertex(u0, v);
            beamVertex(u1, v);
        }
        glEnd();
    }
#ifdef HAVE_GLEW
    if (GLEW_VERSION_2_0)
        glUseProgram(oldProgram);
#endif
    glPopAttrib();
}

} // namespace gLightcycleEffects

#endif
