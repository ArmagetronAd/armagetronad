// Retained stadium geometry and materials. GPL version 2 or later.
#include "gArenaBackdrop.h"
#define DONTDOIT
#include "rRender.h"
#include "rScreen.h"
#include "tConsole.h"

#ifndef DEDICATED
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <map>
#include <utility>
#include <vector>

// A visual stadium, wholly outside the physical rim. Geometry is retained in CPU
// memory so it survives context recreation and can be used by the mirrored pass.
// Coordinates remain in world space: the caller owns all model/view transforms.
class ArenaBackdropImplementation
{
public:
    static void RenderSegment(float x1, float y1, float x2, float y2,
                              float outwardX, float outwardY,
                              float r, float g, float b,
                              bool miterStart = false, bool miterEnd = false, float seconds = 0)
    {
        Key key = {{x1, y1, x2, y2, outwardX, outwardY, r, g, b,
                    float(miterStart), float(miterEnd)}};
        for (float value : key)
            if (!std::isfinite(value))
                return;
        float dx = x2 - x1, dy = y2 - y1;
        float length = std::hypot(dx, dy);
        if (!std::isfinite(length) || length < .001f)
            return;
        // Stable stationing makes ribs and lamps meet across collinear rim splits.
        if (dx < 0 || (dx == 0 && dy < 0))
        {
            std::swap(x1, x2);
            std::swap(y1, y2);
            std::swap(miterStart, miterEnd);
            dx = -dx;
            dy = -dy;
        }
        float tx = dx / length, ty = dy / length;
        // Accept an approximate normal, but never let the architecture drift inward.
        float side = tx * outwardY - ty * outwardX;
        if (std::fabs(side) < .00001f)
            return;
        float ox = side > 0 ? -ty : ty, oy = side > 0 ? tx : -tx;
        r = Clamp(r);
        g = Clamp(g);
        b = Clamp(b);
        key = {{x1, y1, x2, y2, ox, oy, r, g, b, float(miterStart), float(miterEnd)}};
        Cache& cache = Storage();
        auto found = cache.meshes.find(key);
        if (found == cache.meshes.end())
        {
            Mesh mesh;
            Build(mesh, x1, y1, tx, ty, ox, oy, length, r, g, b, miterStart, miterEnd);
            cache.vertices += mesh.Count();
            found = cache.meshes.emplace(key, std::move(mesh)).first;
            // Rim tessellation and custom-map changes cannot grow the cache forever.
            // Eviction happens only when a new segment is first encountered.
            while ((cache.meshes.size() > 512 || cache.vertices > 1500000) && cache.meshes.size() > 1)
            {
                auto oldest = cache.meshes.end();
                for (auto it = cache.meshes.begin(); it != cache.meshes.end(); ++it)
                    if (it != found && (oldest == cache.meshes.end() || it->second.used < oldest->second.used))
                        oldest = it;
                cache.vertices -= oldest->second.Count();
                cache.meshes.erase(oldest);
            }
        }
        Mesh& mesh = found->second;
        mesh.used = ++cache.clock;
        RenderEnd();
        State state;
        Draw(mesh.atmosphere);
#ifdef HAVE_GLEW
        bool material = StructureMaterial::Use(x1, y1, tx, ty, ox, oy,
                                               14.f * std::max(1.f, std::ceil(length / (14.f * 128.f))));
#endif
        Draw(mesh.structure);
#ifdef HAVE_GLEW
        if (material)
            glUseProgram(0);
#endif
        Draw(mesh.lights);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);
        Draw(mesh.halos);
        // A slow optical sweep along the lower fascia gives the stadium depth
        // in motion. It is evaluated outside the retained architectural mesh.
        if (std::isfinite(seconds))
        {
            float origin = x1 * tx + y1 * ty;
            float phase = std::fmod(seconds * 7.f, 84.f);
            float stride = 84.f * std::max(1.f, std::ceil(length / (84.f * 128.f)));
            float first = std::floor((origin - phase - 8.f) / stride) * stride + phase - origin;
            BeginQuads();
            for (float station = first; station < length; station += stride)
            {
                float a = std::max(0.f, station), z = std::min(length, station + 8.f);
                if (a >= z)
                    continue;
                glColor4f(.32f, .82f, 1.f, .04f);
                glVertex3f(x1 + tx * a + ox * 3.005f, y1 + ty * a + oy * 3.005f, 3.84f);
                glVertex3f(x1 + tx * z + ox * 3.005f, y1 + ty * z + oy * 3.005f, 3.84f);
                glColor4f(.52f, .94f, 1.f, .32f);
                glVertex3f(x1 + tx * z + ox * 3.005f, y1 + ty * z + oy * 3.005f, 4.01f);
                glVertex3f(x1 + tx * a + ox * 3.005f, y1 + ty * a + oy * 3.005f, 4.01f);
            }
            RenderEnd();
        }
    }

private:
    struct Point
    {
        float s, d, z;
    };
    struct Colour
    {
        float r, g, b, a;
    };
    struct Vertex
    {
        float x, y, z, r, g, b, a;
    };
    using Batch = std::vector<Vertex>;
    using Key = std::array<float, 11>;
    struct Mesh
    {
        Batch atmosphere, structure, lights, halos;
        std::uint64_t used = 0;
        std::size_t Count() const { return atmosphere.size() + structure.size() + lights.size() + halos.size(); }
    };
    struct Cache
    {
        std::map<Key, Mesh> meshes;
        std::size_t vertices = 0;
        std::uint64_t clock = 0;
    };
    struct Frame
    {
        float x, y, tx, ty, ox, oy, length;
        bool miterStart, miterEnd;
        Vertex Make(Point p, Colour c) const
        {
            float along = p.s;
            // At a true rectangular corner the adjoining profiles meet on the
            // outward 45-degree bisector. Other segment ends remain untouched.
            // Only boundary vertices move; world-stationed details use a plain
            // frame so there is no stretch or duplicate lamp population.
            if (miterStart && p.s == 0)
                along -= p.d;
            if (miterEnd && p.s == length)
                along += p.d;
            return {x + tx * along + ox * p.d, y + ty * along + oy * p.d,
                    p.z, c.r, c.g, c.b, c.a};
        }
    };

    static Cache& Storage()
    {
        static Cache cache;
        return cache;
    }
    static float Clamp(float v) { return std::max(0.f, std::min(1.f, v)); }
    static Colour Scale(Colour c, float gain)
    {
        return {c.r * gain, c.g * gain, c.b * gain, c.a};
    }
    static void Quad(Batch& batch, Frame const& f, Point a, Point b, Point c, Point d,
                     Colour lower, Colour upper)
    {
        Vertex va = f.Make(a, lower), vb = f.Make(b, lower);
        Vertex vc = f.Make(c, upper), vd = f.Make(d, upper);
        batch.insert(batch.end(), {va, vb, vc, va, vc, vd});
    }
    static void Box(Batch& batch, Frame const& f, float s0, float s1,
                    float d0, float d1, float z0, float z1, Colour colour)
    {
        s0 = std::max(0.f, s0);
        s1 = std::min(f.length, s1);
        if (s0 >= s1)
            return;
        Colour shade = Scale(colour, .64f), top = Scale(colour, 1.28f);
        Quad(batch, f, {s0, d0, z0}, {s1, d0, z0}, {s1, d0, z1}, {s0, d0, z1}, shade, colour);
        Quad(batch, f, {s1, d1, z0}, {s0, d1, z0}, {s0, d1, z1}, {s1, d1, z1}, shade, shade);
        if (!(f.miterStart && s0 == 0))
            Quad(batch, f, {s0, d1, z0}, {s0, d0, z0}, {s0, d0, z1}, {s0, d1, z1}, shade, colour);
        if (!(f.miterEnd && s1 == f.length))
            Quad(batch, f, {s1, d0, z0}, {s1, d1, z0}, {s1, d1, z1}, {s1, d0, z1}, shade, colour);
        Quad(batch, f, {s0, d0, z1}, {s1, d0, z1}, {s1, d1, z1}, {s0, d1, z1}, top, colour);
        Quad(batch, f, {s0, d1, z0}, {s1, d1, z0}, {s1, d0, z0}, {s0, d0, z0}, shade, shade);
    }
    // An extruded cross-section: its outward-monotone upper profile gives closed
    // end caps without a polygon tessellator or assumptions about arena shape.
    static void Shell(Batch& batch, Frame const& f, std::vector<Point> const& profile, Colour colour)
    {
        for (std::size_t i = 1; i < profile.size(); ++i)
        {
            Point a = profile[i - 1], b = profile[i];
            bool rising = b.z > a.z + .01f;
            Colour face = Scale(colour, rising ? .77f : 1.24f);
            Quad(batch, f, {0, a.d, a.z}, {f.length, a.d, a.z},
                 {f.length, b.d, b.z}, {0, b.d, b.z}, Scale(face, .83f), face);
            if (b.d > a.d && (a.z > 0 || b.z > 0))
            {
                if (!f.miterStart)
                    Quad(batch, f, {0, a.d, 0}, {0, b.d, 0}, {0, b.d, b.z}, {0, a.d, a.z},
                         Scale(colour, .64f), Scale(colour, .91f));
                if (!f.miterEnd)
                    Quad(batch, f, {f.length, b.d, 0}, {f.length, a.d, 0},
                         {f.length, a.d, a.z}, {f.length, b.d, b.z},
                         Scale(colour, .64f), Scale(colour, .91f));
            }
        }
    }
    static void Beam(Batch& batch, Frame const& f, float s, float halfWidth,
                     float d0, float z0, float d1, float z1, float thickness, Colour colour)
    {
        float dd = d1 - d0, dz = z1 - z0, inverse = thickness / std::hypot(dd, dz);
        float nd = -dz * inverse, nz = dd * inverse;
        float s0 = std::max(0.f, s - halfWidth), s1 = std::min(f.length, s + halfWidth);
        if (s0 >= s1)
            return;
        Point p[] = {{0, d0 - nd, z0 - nz}, {0, d1 - nd, z1 - nz}, {0, d1 + nd, z1 + nz}, {0, d0 + nd, z0 + nz}};
        for (int i = 0; i < 4; ++i)
        {
            Point a = p[i], b = p[(i + 1) % 4];
            Quad(batch, f, {s0, a.d, a.z}, {s1, a.d, a.z}, {s1, b.d, b.z}, {s0, b.d, b.z},
                 Scale(colour, .72f), colour);
        }
        Quad(batch, f, {s0, p[0].d, p[0].z}, {s0, p[1].d, p[1].z},
             {s0, p[2].d, p[2].z}, {s0, p[3].d, p[3].z}, colour, Scale(colour, .7f));
        Quad(batch, f, {s1, p[3].d, p[3].z}, {s1, p[2].d, p[2].z},
             {s1, p[1].d, p[1].z}, {s1, p[0].d, p[0].z}, colour, Scale(colour, .7f));
    }
    static void Light(Mesh& mesh, Frame const& f, float s0, float s1,
                      float depth, float height, float thickness, Colour colour, float halo)
    {
        s0 = std::max(0.f, s0);
        s1 = std::min(f.length, s1);
        if (s0 >= s1)
            return;
        Quad(mesh.lights, f, {s0, depth, height - thickness}, {s1, depth, height - thickness},
             {s1, depth, height + thickness}, {s0, depth, height + thickness}, colour, colour);
        if (halo <= 0)
            return;
        // Feather both axes. A constant-alpha rectangle here looked like
        // duplicate lamp outlines when inspected at high resolution.
        float radius = thickness * 5;
        float along[] = {std::max(0.f, s0 - radius), s0, s1, std::min(f.length, s1 + radius)};
        float heights[] = {height - radius, height - thickness, height + thickness, height + radius};
        float weights[] = {0, 1, 1, 0};
        for (int row = 0; row < 3; ++row)
            for (int column = 0; column < 3; ++column)
            {
                if (along[column] >= along[column + 1])
                    continue;
                auto corner = [&](int x, int y)
                {
                    Colour soft = colour;
                    soft.a = halo * weights[x] * weights[y];
                    return f.Make({along[x], depth - .015f, heights[y]}, soft);
                };
                Vertex a = corner(column, row), b = corner(column + 1, row);
                Vertex c = corner(column + 1, row + 1), d = corner(column, row + 1);
                mesh.halos.insert(mesh.halos.end(), {a, b, c, a, c, d});
            }
    }
    static void Terrace(std::vector<Point>& profile, float startDepth, float startHeight,
                        float tread, float rise, int rows)
    {
        profile.push_back({0, startDepth, startHeight});
        for (int row = 0; row < rows; ++row)
        {
            float d = startDepth + row * tread, z = startHeight + row * rise;
            // Small chamfers keep the silhouette architectural rather than serrated.
            profile.push_back({0, d + .12f, z + rise * .9f});
            profile.push_back({0, d + tread - .10f, z + rise * .9f});
            profile.push_back({0, d + tread, z + rise});
        }
    }
    static void Build(Mesh& mesh, float x, float y, float tx, float ty, float ox, float oy,
                      float length, float r, float g, float b, bool miterStart, bool miterEnd)
    {
        Frame f = {x, y, tx, ty, ox, oy, length, miterStart, miterEnd};
        Frame details = {x, y, tx, ty, ox, oy, length, false, false};
        // These are visible material midtones, not emissive edge colours. The
        // shader shades the solid mass, while this baked face gradient remains
        // readable on the compatibility renderer as well.
        Colour graphite = {.042f + .004f * r, .058f + .005f * g, .071f + .006f * b, 1};
        Colour steel = {.080f + .005f * r, .103f + .006f * g, .123f + .008f * b, 1};
        Colour cyan = {.23f + .07f * r, .78f + .12f * g, .94f + .06f * b, 1};
        Colour dim = {.08f, .25f + .07f * g, .33f + .07f * b, 1};
        Colour white = {.82f, .91f, .96f, 1};
        std::size_t bays = static_cast<std::size_t>(std::min(130.f, std::ceil(length / 14.f) + 2));
        mesh.atmosphere.reserve(6);
        mesh.structure.reserve(1536 + 180 * bays);
        mesh.lights.reserve(42 + 48 * bays);
        mesh.halos.reserve(24 + 6 * bays);

        // A distant blue-black falloff behind the tiers supplies the film's
        // atmospheric depth. It is geometry outside the rim, not a screen tint.
        Quad(mesh.atmosphere, f, {0, 39.f, 0}, {length, 39.f, 0}, {length, 39.f, 110.f}, {0, 39.f, 110.f},
             {.028f, .050f, .061f, 1}, {.004f, .010f, .015f, 1});

        std::vector<Point> bowl = {{0, 3.15f, 0}, {0, 3.15f, 3.45f}, {0, 5.f, 3.45f}};
        Terrace(bowl, 5.f, 3.45f, 1.35f, .94f, 6);
        bowl.push_back({0, 15.9f, 9.09f});
        Terrace(bowl, 16.f, 9.65f, 1.35f, 1.14f, 6);
        bowl.push_back({0, 26.9f, 16.49f});
        Terrace(bowl, 27.f, 17.1f, 1.4f, 1.14f, 5);
        bowl.push_back({0, 34.2f, 25.25f});
        bowl.push_back({0, 35.5f, 25.55f});
        bowl.push_back({0, 36.2f, 24.9f});
        bowl.push_back({0, 37.f, 0});
        bowl.push_back(bowl.front());
        Shell(mesh.structure, f, bowl, graphite);

        // Recessed walkways, with only the edge rail exposed to the playing field.
        float railDepth[] = {3.09f, 13.30f, 24.30f};
        float railHeight[] = {3.95f, 9.65f, 17.05f};
        for (int tier = 0; tier < 3; ++tier)
        {
            float d = railDepth[tier], z = railHeight[tier];
            // A solid fascia and recessed soffit give each tier a readable
            // thickness below its rail, instead of a luminous floating outline.
            Box(mesh.structure, f, 0, length, d + .08f, d + 1.10f, z - .64f, z + .10f, Scale(graphite, .91f));
            Box(mesh.structure, f, 0, length, d + .04f, d + .40f, z - .15f, z + .15f, steel);
            Light(mesh, f, 0, length, d, z, .035f, Scale(cyan, tier == 0 ? .82f : .62f), .10f);
            Light(mesh, f, 0, length, d + .02f, z - .19f, .012f, dim, 0);
        }

        // A thin swept canopy breaks the skyline without enclosing the arena.
        std::vector<Point> canopy = {{0, 28.65f, 26.9f}, {0, 28.8f, 27.3f}, {0, 36.8f, 27.85f}, {0, 38.4f, 27.55f}, {0, 38.4f, 27.20f}, {0, 35.f, 27.4f}, {0, 29.f, 26.9f}, {0, 28.65f, 26.9f}};
        // Only the canopy skin is extruded: its underside must stay open above
        // the tiers rather than acquire the bowl's ground-reaching end caps.
        for (std::size_t i = 1; i < canopy.size(); ++i)
        {
            Point a = canopy[i - 1], c = canopy[i];
            Quad(mesh.structure, f, {0, a.d, a.z}, {length, a.d, a.z}, {length, c.d, c.z}, {0, c.d, c.z},
                 Scale(steel, .62f), Scale(steel, .9f));
        }
        Box(mesh.structure, f, 0, length, 28.63f, 29.25f, 26.40f, 27.25f, Scale(steel, .90f));
        Light(mesh, f, 0, length, 28.62f, 27.04f, .035f, Scale(cyan, .75f), .12f);

        // Station spacing is fixed on ordinary maps and grows only for extreme
        // custom segments, bounding mesh size even with kilometre-long rims.
        float stride = 14.f * std::max(1.f, std::ceil(length / (14.f * 128.f)));
        float origin = x * tx + y * ty;
        double first = std::ceil((double(origin) - 1.) / stride) * stride - origin;
        for (double station = first; station < double(length) + 1.; station += stride)
        {
            float s = float(station);
            Box(mesh.structure, details, s - .32f, s + .32f, 3.035f, 4.2f, .05f, 4.1f, steel);
            Box(mesh.structure, details, s - .20f, s + .20f, 14.2f, 15.4f, .3f, 10.7f, graphite);
            // Support the canopy at its rear instead of running bright diagonal
            // ribs across the entire bowl and its floor reflection.
            Beam(mesh.structure, details, s, .23f, 30.8f, 23.9f, 34.0f, 27.25f, .16f, graphite);
            Box(mesh.structure, details, s - .25f, s + .25f, 33.8f, 35.f, 22.5f, 27.4f, graphite);
            // Warm-white lamps are orderly architectural fixtures, not a field
            // of randomly coloured neon or animated spectator lights.
            Box(mesh.structure, details, s - 2.45f, s + 2.45f, 28.53f, 29.f, 26.38f, 27.40f, graphite);
            Light(mesh, details, s - 2.15f, s + 2.15f, 28.49f, 26.89f, .32f, white, .08f);
            Light(mesh, details, s - .32f, s + .32f, 3.015f, 1.55f, .055f, Scale(cyan, .72f), 0);
            // Recessed banks of light and edge-lit structural blades make the
            // tiers legible against the sky, as in the lightcycle stadium.
            Box(mesh.structure, details, s - 2.1f, s + 2.1f, 3.02f, 3.25f, 4.9f, 5.6f, graphite);
            Light(mesh, details, s - 1.75f, s + 1.75f, 2.995f, 5.25f, .115f, Scale(white, .72f), .10f);
            Light(mesh, details, s - .19f, s + .19f, 14.16f, 7.4f, .72f, Scale(cyan, .46f), .035f);
            Quad(mesh.lights, details, {s - .025f, 30.79f, 23.9f}, {s + .025f, 30.79f, 23.9f},
                 {s + .025f, 33.99f, 27.2f}, {s - .025f, 33.99f, 27.2f}, Scale(dim, .65f), Scale(dim, .25f));
        }

        // Sparse aisle markers give the dark seating scale. They follow a quiet,
        // repeated arrangement and sit above the actual terrace surfaces.
        float tierDepth[] = {5.f, 16.f, 27.f};
        float tierHeight[] = {3.45f, 9.65f, 17.1f};
        float rise[] = {.94f, 1.14f, 1.14f};
        for (int tier = 0; tier < 3; ++tier)
            for (int row = 1; row < 5; row += 2)
            {
                float tread = tier == 2 ? 1.4f : 1.35f;
                float d = tierDepth[tier] + row * tread + .10f;
                float z = tierHeight[tier] + (row + .9f) * rise[tier] + .035f;
                float offset = (tier + row) % 2 ? stride * .5f : 0;
                double start = std::ceil((double(origin) - offset - .075) / stride) * stride + offset - origin;
                for (double station = start; station < double(length) + .075; station += stride)
                    Light(mesh, details, float(station) - .075f, float(station) + .075f, d, z, .035f,
                          Scale(dim, .78f), 0);
            }
    }

#ifdef HAVE_GLEW
    // A small opaque material pass gives the retained stadium actual face and
    // metal response. Fixtures and halos are still drawn independently, so a
    // diffuse roof cannot accidentally become an enormous glowing light source.
    class StructureMaterial
    {
        struct Bindings
        {
            GLint frame = -1, origin = -1, spacing = -1;
        };
        static GLuint& Program()
        {
            static GLuint value = 0;
            return value;
        }
        static bool& Failed()
        {
            static bool value = false;
            return value;
        }
        static Bindings& Uniforms()
        {
            static Bindings value;
            return value;
        }
        static void Unload()
        {
            if (Program())
                glDeleteProgram(Program());
            Program() = 0;
            Failed() = false;
            Uniforms() = Bindings();
        }
        static GLuint Compile(GLenum type, char const* source)
        {
            GLuint shader = glCreateShader(type);
            glShaderSource(shader, 1, &source, NULL);
            glCompileShader(shader);
            GLint success = GL_FALSE;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                char message[1024] = "";
                glGetShaderInfoLog(shader, sizeof(message) - 1, NULL, message);
                con << "Stadium material: " << message << "\n";
                glDeleteShader(shader);
                return 0;
            }
            return shader;
        }
        static bool Prepare()
        {
            static rCallbackBeforeScreenModeChange cleanup(&Unload);
            if (Program())
                return true;
            if (Failed() || !GLEW_VERSION_2_0)
                return false;
            char const* vertex =
                "#version 120\n"
                "uniform vec4 arenaFrame;\n"
                "uniform vec2 arenaOrigin;\n"
                "varying vec3 viewPosition;\n"
                "varying vec3 architecturalPosition;\n"
                "varying vec3 alongView, outwardView, upView;\n"
                "varying vec3 keyView, fillView;\n"
                "varying vec4 materialColour;\n"
                "void main() {\n"
                "    vec4 position=gl_ModelViewMatrix*gl_Vertex;\n"
                "    viewPosition=position.xyz;\n"
                "    architecturalPosition=vec3(dot(gl_Vertex.xy,arenaFrame.xy),dot(gl_Vertex.xy-arenaOrigin,arenaFrame.zw),gl_Vertex.z);\n"
                "    alongView=(gl_ModelViewMatrix*vec4(arenaFrame.xy,0.0,0.0)).xyz;\n"
                "    outwardView=(gl_ModelViewMatrix*vec4(arenaFrame.zw,0.0,0.0)).xyz;\n"
                "    upView=(gl_ModelViewMatrix*vec4(0.0,0.0,1.0,0.0)).xyz;\n"
                "    keyView=normalize((gl_ModelViewMatrix*vec4(0.36,-0.48,0.80,0.0)).xyz);\n"
                "    fillView=normalize(-outwardView+upView*0.45);\n"
                "    materialColour=gl_Color;\n"
                "    gl_ClipVertex=position;\n"
                "    gl_Position=ftransform();\n"
                "}\n";
            char const* fragment =
                "#version 120\n"
                "uniform float fixtureSpacing;\n"
                "varying vec3 viewPosition;\n"
                "varying vec3 architecturalPosition;\n"
                "varying vec3 alongView, outwardView, upView;\n"
                "varying vec3 keyView, fillView;\n"
                "varying vec4 materialColour;\n"
                "float seam(float coordinate,float halfWidth) {\n"
                "    float distance=abs(fract(coordinate+0.5)-0.5);\n"
                "    float footprint=max(fwidth(coordinate),0.00001);\n"
                "    return (1.0-smoothstep(halfWidth,halfWidth+footprint,distance))*halfWidth/(halfWidth+footprint);\n"
                "}\n"
                "float lamp(vec3 source,vec3 n,float reach) {\n"
                "    vec3 local=source-architecturalPosition;\n"
                "    float distance2=dot(local,local);\n"
                "    vec3 delta=alongView*local.x+outwardView*local.y+upView*local.z;\n"
                "    float diffuse=max(dot(n,normalize(delta)),0.0);\n"
                "    return diffuse/(1.0+distance2/(reach*reach))*(1.0-smoothstep(reach*1.4,reach*2.8,sqrt(distance2)));\n"
                "}\n"
                "float seatHash(vec2 p) {\n"
                "    vec3 q=fract(vec3(p.xyx)*0.1031);\n"
                "    q+=dot(q,q.yzx+33.33);\n"
                "    return fract((q.x+q.y)*q.z);\n"
                "}\n"
                "void main() {\n"
                // Derivative normals retain the actual flat architectural faces
                // without duplicating the cached mesh or uploading normal arrays.
                "    vec3 n=normalize(cross(dFdx(viewPosition),dFdy(viewPosition)));\n"
                "    vec3 eye=normalize(-viewPosition);\n"
                "    if (dot(n,eye)<0.0) n=-n;\n"
                "    vec3 key=normalize(keyView), fill=normalize(fillView);\n"
                "    float vertical=1.0-abs(dot(n,normalize(upView)));\n"
                "    float metal=smoothstep(0.075,0.18,materialColour.r);\n"
                "    float panel=seam(architecturalPosition.x/3.5,0.004);\n"
                "    panel+=seam(architecturalPosition.z/2.4,0.005)*vertical*0.4;\n"
                "    float finish=1.0-0.15*panel;\n"
                "    float grainPhase=architecturalPosition.x*240.0+architecturalPosition.z*39.0;\n"
                "    finish+=sin(grainPhase)*(1.0-smoothstep(0.65,3.0,fwidth(grainPhase)))*0.018*metal;\n"
                "    float light=0.59+0.66*max(dot(n,key),0.0)+0.37*max(dot(n,fill),0.0);\n"
                "    vec3 body=materialColour.rgb*light*finish;\n"
                "    float keyGloss=pow(max(dot(n,normalize(eye+key)),0.0),mix(12.0,38.0,metal));\n"
                "    float fillGloss=pow(max(dot(n,normalize(eye+fill)),0.0),9.0);\n"
                "    float fresnel=pow(1.0-max(dot(n,eye),0.0),4.0);\n"
                "    body+=vec3(0.85,0.91,0.96)*keyGloss*(0.018+0.11*metal);\n"
                "    body+=vec3(0.40,0.55,0.61)*fillGloss*(0.012+0.050*metal+0.025*fresnel);\n"
                // The light pools coincide with existing lower and canopy lamps.
                // They expose recess depth without introducing extra neon objects.
                "    float station=floor(architecturalPosition.x/fixtureSpacing+0.5)*fixtureSpacing;\n"
                "    float lower=lamp(vec3(station,2.995,5.25),n,4.5);\n"
                "    float upper=lamp(vec3(station,28.49,26.89),n,6.0);\n"
                "    body+=vec3(0.075,0.105,0.115)*(lower+upper)*mix(0.55,1.0,metal);\n"
                "    vec2 local=architecturalPosition.yz;\n"
                "    float rail=exp(-dot(local-vec2(3.09,3.95),local-vec2(3.09,3.95))*0.45);\n"
                "    rail+=exp(-dot(local-vec2(13.30,9.65),local-vec2(13.30,9.65))*0.45);\n"
                "    rail+=exp(-dot(local-vec2(24.30,17.05),local-vec2(24.30,17.05))*0.45);\n"
                "    body+=vec3(0.005,0.017,0.022)*rail;\n"
                // Fine, non-emissive seating texture gives the stands scale.
                // It converges to an average below pixel size instead of
                // becoming a flickering field of point lights.
                "    float d=architecturalPosition.y;\n"
                "    float seating=(step(5.2,d)-step(12.8,d))+(step(16.2,d)-step(23.8,d))+(step(27.2,d)-step(33.8,d));\n"
                "    seating*=step(3.4,architecturalPosition.z)*(1.0-step(24.8,architecturalPosition.z));\n"
                "    vec2 seats=vec2(architecturalPosition.x*2.4,architecturalPosition.z*2.6);\n"
                "    vec2 seat=floor(seats);\n"
                "    float population=seatHash(seat);\n"
                "    vec2 offset=vec2(seatHash(seat+vec2(7.0,3.0)),seatHash(seat+vec2(2.0,9.0)))-0.5;\n"
                "    vec2 cell=fract(seats)-vec2(0.5,0.42)-offset*0.28;\n"
                "    float footprint=max(length(fwidth(seats)),0.001);\n"
                "    float resolved=1.0-smoothstep(0.5,1.4,footprint);\n"
                "    vec2 head=(cell-vec2(0.0,0.16))*vec2(7.5,8.0);\n"
                "    vec2 shoulders=(cell+vec2(0.0,0.10))*vec2(3.9,5.0);\n"
                "    float shape=exp(-dot(head,head))*0.7+exp(-dot(shoulders,shoulders))*0.55;\n"
                "    float grain=mix(0.075,shape*(0.25+0.75*population),resolved);\n"
                "    float aisle=abs(fract(architecturalPosition.x/fixtureSpacing+0.5)-0.5);\n"
                "    grain*=smoothstep(0.015,0.045,aisle);\n"
                "    body=mix(body,body*0.45+vec3(0.10,0.18,0.20)*grain,seating);\n"
                "    gl_FragColor=vec4(min(body,vec3(0.65)),materialColour.a);\n"
                "}\n";
            GLuint vs = Compile(GL_VERTEX_SHADER, vertex), fs = Compile(GL_FRAGMENT_SHADER, fragment);
            if (vs && fs)
            {
                Program() = glCreateProgram();
                glAttachShader(Program(), vs);
                glAttachShader(Program(), fs);
                glLinkProgram(Program());
                GLint success = GL_FALSE;
                glGetProgramiv(Program(), GL_LINK_STATUS, &success);
                if (!success)
                {
                    char message[1024] = "";
                    glGetProgramInfoLog(Program(), sizeof(message) - 1, NULL, message);
                    con << "Stadium material link: " << message << "\n";
                    glDeleteProgram(Program());
                    Program() = 0;
                }
                else
                {
                    Uniforms().frame = glGetUniformLocation(Program(), "arenaFrame");
                    Uniforms().origin = glGetUniformLocation(Program(), "arenaOrigin");
                    Uniforms().spacing = glGetUniformLocation(Program(), "fixtureSpacing");
                }
            }
            if (vs)
                glDeleteShader(vs);
            if (fs)
                glDeleteShader(fs);
            if (!Program())
            {
                Failed() = true;
                con << "Stadium material: using compatibility shading.\n";
            }
            return Program() != 0;
        }

    public:
        static bool Use(float x, float y, float tx, float ty, float ox, float oy, float spacing)
        {
            if (!Prepare())
                return false;
            glUseProgram(Program());
            glUniform4f(Uniforms().frame, tx, ty, ox, oy);
            glUniform2f(Uniforms().origin, x, y);
            glUniform1f(Uniforms().spacing, spacing);
            return true;
        }
    };
#endif

    // Architectural colours remain usable without shaders. The caller's GL
    // state is restored after structure, fixtures, and optical halos are drawn.
    class State
    {
    public:
        State()
        {
            glGetIntegerv(GL_MATRIX_MODE, &matrixMode_);
            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            // This is scenery outside the collision rim. Compress its vertical
            // profile into a low stadium; the playing plane and camera stay put.
            glScalef(1.f, 1.f, .58f);
            glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT |
                         GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT | GL_LIGHTING_BIT | GL_TEXTURE_BIT);
            glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
#ifdef HAVE_GLEW
            shaders_ = GLEW_VERSION_2_0;
            buffers_ = GLEW_VERSION_1_5;
            multitexture_ = GLEW_VERSION_1_3;
            if (shaders_)
            {
                glGetIntegerv(GL_CURRENT_PROGRAM, &program_);
                glUseProgram(0);
            }
            if (buffers_)
            {
                glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &arrayBuffer_);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
            }
            if (multitexture_)
            {
                glGetIntegerv(GL_ACTIVE_TEXTURE, &texture_);
                glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &clientTexture_);
                GLint units = 1;
                glGetIntegerv(GL_MAX_TEXTURE_UNITS, &units);
                for (GLint i = 0; i < units; ++i)
                {
                    glActiveTexture(GL_TEXTURE0 + i);
                    glClientActiveTexture(GL_TEXTURE0 + i);
                    DisableTexture();
                    glDisable(GL_TEXTURE_3D);
                    glDisable(GL_TEXTURE_CUBE_MAP);
                }
                glActiveTexture(GL_TEXTURE0);
                glClientActiveTexture(GL_TEXTURE0);
            }
            else
#endif
                DisableTexture();
            glDisable(GL_LIGHTING);
            glDisable(GL_ALPHA_TEST);
            glDisable(GL_BLEND);
            glDisable(GL_CULL_FACE);
            glDisable(GL_POLYGON_OFFSET_FILL);
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LEQUAL);
            glDepthMask(GL_TRUE);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glShadeModel(GL_SMOOTH);
            glDisableClientState(GL_NORMAL_ARRAY);
            glDisableClientState(GL_INDEX_ARRAY);
            glDisableClientState(GL_EDGE_FLAG_ARRAY);
#ifdef HAVE_GLEW
            if (GLEW_VERSION_1_4)
            {
                glDisableClientState(GL_SECONDARY_COLOR_ARRAY);
                glDisableClientState(GL_FOG_COORD_ARRAY);
            }
#endif
            glEnableClientState(GL_VERTEX_ARRAY);
            glEnableClientState(GL_COLOR_ARRAY);
        }
        ~State()
        {
            glPopClientAttrib();
            glPopAttrib();
            glMatrixMode(GL_MODELVIEW);
            glPopMatrix();
            glMatrixMode(matrixMode_);
#ifdef HAVE_GLEW
            if (buffers_)
                glBindBuffer(GL_ARRAY_BUFFER, arrayBuffer_);
            if (shaders_)
                glUseProgram(program_);
            if (multitexture_)
            {
                glActiveTexture(texture_);
                glClientActiveTexture(clientTexture_);
            }
#endif
        }

    private:
        GLint matrixMode_ = GL_MODELVIEW;
        static void DisableTexture()
        {
            glDisable(GL_TEXTURE_1D);
            glDisable(GL_TEXTURE_2D);
            glDisable(GL_TEXTURE_GEN_S);
            glDisable(GL_TEXTURE_GEN_T);
            glDisable(GL_TEXTURE_GEN_R);
            glDisable(GL_TEXTURE_GEN_Q);
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        }
#ifdef HAVE_GLEW
        GLint program_ = 0, arrayBuffer_ = 0, texture_ = GL_TEXTURE0, clientTexture_ = GL_TEXTURE0;
        bool shaders_ = false, buffers_ = false, multitexture_ = false;
#endif
    };
    static void Draw(Batch const& vertices)
    {
        if (vertices.empty())
            return;
        glVertexPointer(3, GL_FLOAT, sizeof(Vertex), &vertices.front().x);
        glColorPointer(4, GL_FLOAT, sizeof(Vertex), &vertices.front().r);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
    }
};

void gArenaBackdrop::RenderSegment(float x1, float y1, float x2, float y2,
                                   float outwardX, float outwardY, float r, float g, float b,
                                   bool miterStart, bool miterEnd, float seconds)
{
    ArenaBackdropImplementation::RenderSegment(x1, y1, x2, y2, outwardX, outwardY,
                                               r, g, b, miterStart, miterEnd, seconds);
}
#endif
