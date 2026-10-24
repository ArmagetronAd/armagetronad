// Authored model loading, fallback geometry and GPU caching. GPL version 2 or later.
#include "gLightcycleModel.h"
#ifndef DEDICATED
#define DONTDOIT
#include "rGL.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <vector>
#include <fstream>
#include "gLightcycleAsset.h"
#include "gLightcycleProfile.h"
#include "rTexture.h"
#include "tConsole.h"
#ifdef HAVE_GLEW
#include "rScreen.h"
#include <map>
#endif

namespace gLightcycleModel
{
namespace Detail
{
float const Pi = 3.14159265358979323846f;

struct Vec
{
    float x, y, z;
    Vec(float x_ = 0, float y_ = 0, float z_ = 0) : x(x_), y(y_), z(z_) {}
    Vec operator+(Vec const& v) const { return Vec(x + v.x, y + v.y, z + v.z); }
    Vec operator-(Vec const& v) const { return Vec(x - v.x, y - v.y, z - v.z); }
    Vec operator*(float f) const { return Vec(x * f, y * f, z * f); }
};

inline float Dot(Vec const& a, Vec const& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec Cross(Vec const& a, Vec const& b)
{
    return Vec(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}
inline Vec Unit(Vec const& v)
{
    float length = std::sqrt(Dot(v, v));
    // Finite-difference surface normals can be much smaller than a position
    // vector. Keep their direction instead of replacing valid small normals
    // with world-up; reserve the fallback for degenerate/nonfinite input.
    return std::isfinite(length) && length > 1.e-12f ? v * (1.f / length) : Vec(0, 0, 1);
}
inline Vec Catmull(Vec const& a, Vec const& b, Vec const& c, Vec const& d, float t)
{
    return (b * 2.f + (c - a) * t + (a * 2.f - b * 5.f + c * 4.f - d) * (t * t) + (b * 3.f - a - c * 3.f + d) * (t * t * t)) * .5f;
}

struct Vertex
{
    Vec position, normal;
    float optical[4] = {0, 0, 0, 0};
    Vertex(Vec const& p, Vec const& n) : position(p), normal(n) {}
};

#ifdef HAVE_GLEW
struct Mesh;
inline void ReleaseVertexBuffers();

// A zero entry records a failed allocation so an old/low-memory driver is not
// asked to upload the same mesh every frame. Reset the cache with the GL context.
inline std::map<Mesh const*, GLuint>& VertexBuffers()
{
    static std::map<Mesh const*, GLuint> buffers;
    static rCallbackBeforeScreenModeChange cleanup(&ReleaseVertexBuffers);
    (void)cleanup;
    return buffers;
}

inline void ReleaseVertexBuffers()
{
    std::map<Mesh const*, GLuint>& buffers = VertexBuffers();
    if (GLEW_VERSION_1_5)
        for (auto const& entry : buffers)
            if (entry.second)
                glDeleteBuffers(1, &entry.second);
    buffers.clear();
}

inline GLuint UploadVertexBuffer(std::vector<Vertex> const& vertices)
{
    GLuint buffer = 0;
    glGenBuffers(1, &buffer);
    if (buffer)
    {
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        GLsizeiptr bytes = static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex));
        glBufferData(GL_ARRAY_BUFFER, bytes, vertices.data(), GL_STATIC_DRAW);
        GLint allocated = 0;
        glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &allocated);
        if (allocated == bytes)
            return buffer;
        glDeleteBuffers(1, &buffer);
    }
    // Allocation errors belong to this optional path. Leave a clean client-array
    // fallback rather than letting an out-of-memory flag disable a later effect.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    while (glGetError() != GL_NO_ERROR)
        ;
    return 0;
}
#endif

struct Mesh
{
    std::vector<Vertex> vertices;

    void Triangle(Vertex a, Vertex b, Vertex c)
    {
        Vec face = Cross(b.position - a.position, c.position - a.position);
        if (Dot(face, face) < 1.e-15f)
            return;
        if (Dot(face, a.normal + b.normal + c.normal) < 0)
            std::swap(b, c);
        vertices.push_back(a);
        vertices.push_back(b);
        vertices.push_back(c);
    }

    void Quad(Vertex const& a, Vertex const& b, Vertex const& c, Vertex const& d)
    {
        Triangle(a, b, c);
        Triangle(a, c, d);
    }

    void Draw() const
    {
        if (vertices.empty())
            return;
#ifdef HAVE_GLEW
        if (GLEW_VERSION_1_5)
        {
            std::map<Mesh const*, GLuint>& buffers = VertexBuffers();
            auto entry = buffers.find(this);
            if (entry == buffers.end())
                entry = buffers.emplace(this, UploadVertexBuffer(vertices)).first;
            glBindBuffer(GL_ARRAY_BUFFER, entry->second);
            if (entry->second)
            {
                glVertexPointer(3, GL_FLOAT, sizeof(Vertex), reinterpret_cast<void const*>(offsetof(Vertex, position)));
                glNormalPointer(GL_FLOAT, sizeof(Vertex), reinterpret_cast<void const*>(offsetof(Vertex, normal)));
                glTexCoordPointer(4, GL_FLOAT, sizeof(Vertex), reinterpret_cast<void const*>(offsetof(Vertex, optical)));
                glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
                return;
            }
        }
#endif
        unsigned char const* first = reinterpret_cast<unsigned char const*>(vertices.data());
        glVertexPointer(3, GL_FLOAT, sizeof(Vertex), first + offsetof(Vertex, position));
        glNormalPointer(GL_FLOAT, sizeof(Vertex), first + offsetof(Vertex, normal));
        glTexCoordPointer(4, GL_FLOAT, sizeof(Vertex), first + offsetof(Vertex, optical));
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
    }
};

enum Material
{
    Shell,
    Armour,
    Metal,
    Rubber,
    Recess,
    Suit,
    Visor,
    Light,
    SubtleLight,
    Headlight,
    MaterialCount
};
typedef std::array<Mesh, MaterialCount> Parts;

template <class Function>
inline void Surface(Mesh& mesh, int columns, int rows, Function sample)
{
    for (int y = 0; y < rows; ++y)
        for (int x = 0; x < columns; ++x)
        {
            float u0 = float(x) / columns, u1 = float(x + 1) / columns;
            float v0 = float(y) / rows, v1 = float(y + 1) / rows;
            mesh.Quad(sample(u0, v0), sample(u1, v0), sample(u1, v1), sample(u0, v1));
        }
}

inline void Ellipsoid(Mesh& mesh, Vec const& center, Vec const& radius,
                      int longitude = 28, int latitude = 16)
{
    Surface(mesh, longitude, latitude, [&](float u, float v)
            {
        float a = 2.f * Pi * u, b = Pi * (v - .5f);
        Vec n(std::cos(b) * std::cos(a), std::cos(b) * std::sin(a), std::sin(b));
        Vec p(n.x * radius.x, n.y * radius.y, n.z * radius.z);
        return Vertex(center + p, Unit(Vec(n.x / radius.x, n.y / radius.y, n.z / radius.z))); });
}

inline void Rod(Mesh& mesh, Vec const& a, Vec const& b, float ra, float rb,
                int sides = 14)
{
    Vec axis = b - a;
    float length = std::sqrt(Dot(axis, axis));
    Vec direction = Unit(axis);
    Vec u = Unit(Cross(direction, std::fabs(direction.z) < .95f ? Vec(0, 0, 1) : Vec(0, 1, 0)));
    Vec v = Cross(direction, u);
    for (int i = 0; i < sides; ++i)
    {
        float p0 = 2 * Pi * i / sides, p1 = 2 * Pi * (i + 1) / sides;
        Vec radial0 = u * std::cos(p0) + v * std::sin(p0);
        Vec radial1 = u * std::cos(p1) + v * std::sin(p1);
        Vec n0 = Unit(radial0 + direction * ((ra - rb) / std::max(.0001f, length)));
        Vec n1 = Unit(radial1 + direction * ((ra - rb) / std::max(.0001f, length)));
        mesh.Quad(Vertex(a + radial0 * ra, n0), Vertex(b + radial0 * rb, n0),
                  Vertex(b + radial1 * rb, n1), Vertex(a + radial1 * ra, n1));
        mesh.Triangle(Vertex(a, direction * -1), Vertex(a + radial1 * ra, direction * -1),
                      Vertex(a + radial0 * ra, direction * -1));
        mesh.Triangle(Vertex(b, direction), Vertex(b + radial0 * rb, direction),
                      Vertex(b + radial1 * rb, direction));
    }
}

// A closed profile revolved around the axle. Normals follow the profile instead
// of flattening the broad tyre shoulders, metal lips or the curved fender crown.
struct Radial
{
    float y, radius;
};

inline void Lathe(Mesh& mesh, std::vector<Radial> const& profile, Vec const& center,
                  float start = 0, float finish = 2 * Pi, int segments = 72)
{
    int count = static_cast<int>(profile.size());
    for (int j = 0; j < count; ++j)
    {
        int next = (j + 1) % count;
        auto sample = [&](int k, float angle)
        {
            Radial const& previous = profile[(k + count - 1) % count];
            Radial const& following = profile[(k + 1) % count];
            float dy = following.y - previous.y, dr = following.radius - previous.radius;
            float c = std::cos(angle), s = std::sin(angle);
            return Vertex(center + Vec(profile[k].radius * c, profile[k].y, profile[k].radius * s),
                          Unit(Vec(dy * c, -dr, dy * s)));
        };
        for (int i = 0; i < segments; ++i)
        {
            float a = start + (finish - start) * i / segments;
            float b = start + (finish - start) * (i + 1) / segments;
            mesh.Quad(sample(j, a), sample(j, b), sample(next, b), sample(next, a));
        }
        if (finish - start < 2 * Pi - .001f)
        {
            Radial middle = {0, 0};
            for (Radial const& p : profile)
            {
                middle.y += p.y / count;
                middle.radius += p.radius / count;
            }
            for (int cap = 0; cap < 2; ++cap)
            {
                float angle = cap ? finish : start;
                Vec n(std::sin(angle), 0, -std::cos(angle));
                if (cap)
                    n = n * -1;
                Vec c = center + Vec(middle.radius * std::cos(angle), middle.y, middle.radius * std::sin(angle));
                Vertex a = sample(j, angle), b = sample(next, angle);
                mesh.Triangle(Vertex(c, n), Vertex(a.position, n), Vertex(b.position, n));
            }
        }
    }
}

inline void Annulus(Mesh& mesh, float inner, float outer, float y, Vec const& center,
                    float start = 0, float finish = 2 * Pi, int segments = 72)
{
    Vec n(0, y < 0 ? -1.f : 1.f, 0);
    for (int i = 0; i < segments; ++i)
    {
        float a = start + (finish - start) * i / segments;
        float b = start + (finish - start) * (i + 1) / segments;
        auto p = [&](float r, float t)
        { return Vertex(center + Vec(r * std::cos(t), y, r * std::sin(t)), n); };
        mesh.Quad(p(inner, a), p(outer, a), p(outer, b), p(inner, b));
    }
}

inline Vec PathPoint(std::vector<Vec> const& path, float t)
{
    float scaled = std::max(0.f, std::min(1.f, t)) * (path.size() - 1);
    int i = std::min(static_cast<int>(path.size()) - 2, static_cast<int>(scaled));
    return Catmull(path[std::max(0, i - 1)], path[i], path[i + 1],
                   path[std::min(static_cast<int>(path.size()) - 1, i + 2)], scaled - i);
}

inline void Tube(Mesh& mesh, std::vector<Vec> const& path, float radius, int sides = 8)
{
    Surface(mesh, static_cast<int>(path.size() - 1) * 7, sides, [&](float u, float v)
            {
        Vec p = PathPoint(path, u);
        Vec tangent = Unit(PathPoint(path, std::min(1.f, u + .001f))
                         - PathPoint(path, std::max(0.f, u - .001f)));
        Vec first = Unit(Cross(tangent, std::fabs(tangent.y) < .95f ? Vec(0, 1, 0) : Vec(0, 0, 1)));
        Vec second = Cross(tangent, first);
        Vec n = first * std::cos(2 * Pi * v) + second * std::sin(2 * Pi * v);
        return Vertex(p + n * radius, n); });
}

struct Section
{
    float x, z, width, height;
};

inline Section Interpolate(std::vector<Section> const& sections, float u)
{
    float scaled = std::max(0.f, std::min(1.f, u)) * (sections.size() - 1);
    int i = std::min(static_cast<int>(sections.size()) - 2, static_cast<int>(scaled));
    Section const& a = sections[std::max(0, i - 1)];
    Section const& b = sections[i];
    Section const& c = sections[i + 1];
    Section const& d = sections[std::min(static_cast<int>(sections.size()) - 1, i + 2)];
    float t = scaled - i;
    Vec geometry = Catmull(Vec(a.x, a.z, a.width), Vec(b.x, b.z, b.width),
                           Vec(c.x, c.z, c.width), Vec(d.x, d.z, d.width), t);
    Vec vertical = Catmull(Vec(a.height, 0, 0), Vec(b.height, 0, 0),
                           Vec(c.height, 0, 0), Vec(d.height, 0, 0), t);
    return Section{geometry.x, geometry.y, std::max(.003f, geometry.z), std::max(.003f, vertical.x)};
}

inline Vec LoftPoint(std::vector<Section> const& sections, float u, float angle)
{
    Section p = Interpolate(sections, u);
    float c = std::cos(angle), s = std::sin(angle);
    // Broad painted faces with radiused shoulders, instead of a cylindrical
    // cross-section. This is the authored hard-surface shell profile.
    auto rounded = [](float value)
    { return std::copysign(std::pow(std::fabs(value), .72f), value); };
    return Vec(p.x, p.width * rounded(c), p.z + p.height * rounded(s));
}

inline void Loft(Mesh& mesh, std::vector<Section> const& sections)
{
    Surface(mesh, static_cast<int>(sections.size() - 1) * 7, 28, [&](float u, float v)
            {
        float angle = 2 * Pi * v;
        Vec along = LoftPoint(sections, std::min(1.f, u + .001f), angle)
                  - LoftPoint(sections, std::max(0.f, u - .001f), angle);
        Vec around = LoftPoint(sections, u, angle + .001f) - LoftPoint(sections, u, angle - .001f);
        return Vertex(LoftPoint(sections, u, angle), Unit(Cross(around, along))); });
    for (int end = 0; end < 2; ++end)
    {
        float u = float(end);
        Section p = Interpolate(sections, u);
        Vec n(end ? 1.f : -1.f, 0, 0);
        for (int i = 0; i < 28; ++i)
            mesh.Triangle(Vertex(Vec(p.x, 0, p.z), n),
                          Vertex(LoftPoint(sections, u, 2 * Pi * i / 28), n),
                          Vertex(LoftPoint(sections, u, 2 * Pi * (i + 1) / 28), n));
    }
}

inline void Stripe(Mesh& mesh, std::vector<Section> const& sections, float angle, float radius)
{
    std::vector<Vec> path;
    for (int i = 0; i <= 28; ++i)
        path.push_back(LoftPoint(sections, float(i) / 28, angle) + Vec(0, std::cos(angle) * .004f, std::sin(angle) * .004f));
    Tube(mesh, path, radius);
}

// Flush bands follow the fairing surface; they are inlaid panels, not glowing
// pipes floating above the paint.
inline void FairingBand(Mesh& mesh, std::vector<Section> const& sections,
                        float angle, float width, float offset = .004f)
{
    Surface(mesh, static_cast<int>(sections.size() - 1) * 10, 3, [&](float u, float v)
            {
        float a=angle+(v-.5f)*width;
        Vec along=LoftPoint(sections,std::min(1.f,u+.001f),a)-LoftPoint(sections,std::max(0.f,u-.001f),a);
        Vec around=LoftPoint(sections,u,a+.001f)-LoftPoint(sections,u,a-.001f);
        Vec cross=Cross(around,along);
        Vec n=Unit(cross);
        // Preserve the authored inlay position while correcting its lighting
        // normal; these bands previously used a 1e-6 placement threshold.
        float placementLength=std::sqrt(Dot(cross,cross));
        Vec placement=placementLength>.000001f ? n : Vec(0,0,1);
        return Vertex(LoftPoint(sections,u,a)+placement*offset,n); });
}

inline void CrownBand(Mesh& mesh, Vec const& center, float y0, float y1,
                      float radius, float start, float finish)
{
    Surface(mesh, 72, 3, [&](float u, float v)
            {
        float a=start+(finish-start)*u;
        float y=y0+(y1-y0)*v;
        // Match the broad shroud's gently rounded crown.
        float r=radius-.035f*(y/.45f)*(y/.45f);
        Vec n=Unit(Vec(std::cos(a),.07f*y/(.45f*.45f),std::sin(a)));
        return Vertex(center+Vec(r*std::cos(a),y,r*std::sin(a)),n); });
}

// A curved, elliptic volume with independently shaped cross-sections. The
// rider uses these for actual upper/lower limbs: muscle bellies, narrowed joints,
// and a stable lateral axis, rather than straight constant-radius frame tubes.
struct BodySection
{
    Vec center;
    float width, depth;
};

inline BodySection BodySample(std::vector<BodySection> const& sections, float t)
{
    float scaled = std::max(0.f, std::min(1.f, t)) * (sections.size() - 1);
    int i = std::min(static_cast<int>(sections.size()) - 2, static_cast<int>(scaled));
    BodySection const &a = sections[std::max(0, i - 1)], &b = sections[i], &c = sections[i + 1];
    BodySection const& d = sections[std::min(static_cast<int>(sections.size()) - 1, i + 2)];
    float u = scaled - i;
    Vec size = Catmull(Vec(a.width, a.depth, 0), Vec(b.width, b.depth, 0),
                       Vec(c.width, c.depth, 0), Vec(d.width, d.depth, 0), u);
    return {Catmull(a.center, b.center, c.center, d.center, u),
            std::max(.002f, size.x), std::max(.002f, size.y)};
}

inline Vec BodyPoint(std::vector<BodySection> const& sections, float t, float angle)
{
    BodySection section = BodySample(sections, t);
    Vec tangent = Unit(BodySample(sections, std::min(1.f, t + .001f)).center - BodySample(sections, std::max(0.f, t - .001f)).center);
    Vec lateral = Vec(0, 1, 0) - tangent * Dot(Vec(0, 1, 0), tangent);
    lateral = Unit(lateral);
    Vec depth = Cross(tangent, lateral);
    return section.center + lateral * (section.width * std::cos(angle)) + depth * (section.depth * std::sin(angle));
}

inline Vertex BodyVertex(std::vector<BodySection> const& sections, float t, float angle, float offset = 0)
{
    Vec along = BodyPoint(sections, std::min(1.f, t + .001f), angle) - BodyPoint(sections, std::max(0.f, t - .001f), angle);
    Vec around = BodyPoint(sections, t, angle + .001f) - BodyPoint(sections, t, angle - .001f);
    // Normalize the tangents before crossing: raw finite differences on a
    // human-sized hand can be below Unit's position-vector epsilon.
    Vec n = Unit(Cross(Unit(around), Unit(along)));
    return Vertex(BodyPoint(sections, t, angle) + n * offset, n);
}

inline void BodyVolume(Mesh& mesh, std::vector<BodySection> const& sections)
{
    Surface(mesh, static_cast<int>(sections.size() - 1) * 9, 28, [&](float u, float v)
            { return BodyVertex(sections, u, 2 * Pi * v); });
    for (int end = 0; end < 2; ++end)
    {
        float t = float(end);
        Vec n = Unit(BodySample(sections, std::min(1.f, t + .001f)).center - BodySample(sections, std::max(0.f, t - .001f)).center) * (end ? 1.f : -1.f);
        Vec center = BodySample(sections, t).center;
        for (int i = 0; i < 28; ++i)
            mesh.Triangle(Vertex(center, n), Vertex(BodyPoint(sections, t, 2 * Pi * i / 28), n),
                          Vertex(BodyPoint(sections, t, 2 * Pi * (i + 1) / 28), n));
    }
}

inline void BodyPanel(Mesh& mesh, std::vector<BodySection> const& sections,
                      float start, float finish, float angle, float width, float offset = .003f)
{
    Surface(mesh, 24, 8, [&](float u, float v)
            { return BodyVertex(sections, start + (finish - start) * u, angle + (v - .5f) * width, offset); });
}

struct Cache
{
    Parts body, wheel, rider;

    Cache()
    {
        BuildWheel();
        BuildChassis();
        BuildRider();
    }

    void BuildWheel()
    {
        // A deep annular wheel: no central axle, spokes or fork. Most of the
        // visible side opening is empty and the silver barrel recedes into it.
        Lathe(wheel[Rubber], {{-.36f, .510f}, {-.415f, .54f}, {-.442f, .59f}, {-.433f, .65f}, {-.38f, .690f}, {-.27f, .716f}, {-.12f, .72f}, {.12f, .72f}, {.27f, .716f}, {.38f, .690f}, {.433f, .65f}, {.442f, .59f}, {.415f, .54f}, {.36f, .510f}}, Vec(), 0, 2 * Pi, 112);
        Lathe(wheel[Metal], {{-.405f, .492f}, {-.386f, .511f}, {-.34f, .506f}, {-.24f, .498f}, {-.12f, .490f}, {.12f, .490f}, {.24f, .498f}, {.34f, .506f}, {.386f, .511f}, {.405f, .492f}, {.23f, .486f}, {.12f, .480f}, {-.12f, .480f}, {-.23f, .486f}}, Vec(), 0, 2 * Pi, 112);
        for (int side = -1; side <= 1; side += 2)
        {
            float s = float(side);
            // Recessed bearing tracks and staggered inner registers give the
            // hub a real layered mechanism, leaving its centre open. Both sides
            // are modelled; none of this detail is painted onto the outer rim.
            float track = s * .28f;
            Lathe(wheel[Metal], {{track - .026f, .477f}, {track - .015f, .482f}, {track + .015f, .482f}, {track + .026f, .477f}, {track + .015f, .466f}, {track - .015f, .466f}}, Vec(), 0, 2 * Pi, 96);
            for (int i = 0; i < 24; ++i)
            {
                float a = 2 * Pi * i / 24;
                Annulus(wheel[Armour], .462f, .489f, s * .20f, Vec(), a, a + .14f, 4);
                Annulus(wheel[Metal], .463f, .469f, s * .203f, Vec(), a + .015f, a + .07f, 3);
                Vec fastener(.484f * std::cos(a), s * .302f, .484f * std::sin(a));
                Rod(wheel[Recess], fastener, fastener + Vec(0, s * .010f, 0), .009f, .007f, 8);
            }
            Annulus(wheel[Shell], .492f, .550f, s * .443f, Vec(), 0, 2 * Pi, 112);
            Annulus(wheel[Light], .558f, .645f, s * .446f, Vec(), 0, 2 * Pi, 144);
            // The optical core shares the rider's hue. Using Headlight here
            // overrode every colour with a white stripe, even on blue/amber bikes.
            Annulus(wheel[SubtleLight], .549f, .557f, s * .447f, Vec(), 0, 2 * Pi, 144);
            Annulus(wheel[Armour], .646f, .670f, s * .435f, Vec(), 0, 2 * Pi, 112);
            Annulus(wheel[Recess], .491f, .500f, s * .407f, Vec(), 0, 2 * Pi, 112);
            // Recessed register marks stay on the annular lip, never across the
            // opening. They give scale without reconstructing a spoked wheel.
            for (int i = 0; i < 32; ++i)
            {
                float a = 2 * Pi * i / 32;
                Annulus(wheel[Armour], .514f, .535f, s * .445f, Vec(), a, a + .018f, 2);
            }
            // Unequal optical registers make actual distance-driven rotation
            // visible without adding spokes to the film's hubless wheels.
            for (int i = 0; i < 3; ++i)
            {
                float a = 2 * Pi * i / 3 + .17f;
                Annulus(wheel[Metal], .522f, .549f, s * .446f, Vec(), a, a + .34f, 12);
                Annulus(wheel[Metal], .525f, .546f, s * .447f, Vec(), a, a + .065f, 4);
            }
        }
        // Twin luminous tread inlays remain visible in the unchanged rear view.
        for (int side = -1; side <= 1; side += 2)
        {
            CrownBand(wheel[SubtleLight], Vec(), side * .12f - .013f, side * .12f + .013f, .723f, 0, 2 * Pi);
            CrownBand(wheel[Light], Vec(), side * .12f - .003f, side * .12f + .003f, .724f, 0, 2 * Pi);
        }
        for (int i = 0; i < 40; ++i)
        {
            float center = 2 * Pi * i / 40;
            Surface(wheel[Recess], 1, 8, [&](float u, float v)
                    {
                float y=-.26f+.52f*v;
                float angle=center+(u-.5f)*.007f+y*.19f;
                float radius=.722f-.009f*std::pow(std::fabs(y)/.27f,3.f);
                return Vertex(Vec(radius*std::cos(angle),y,radius*std::sin(angle)),
                              Unit(Vec(std::cos(angle),.02f*y,std::sin(angle)))); });
        }
    }

    void BuildChassis()
    {
        // A single closed monocoque carries the painted side surface all the
        // way between the wheel housings. Its underside and shoulder are one
        // volume, not separate ribbons around an exposed tubular frame.
        std::vector<Section> hull = {
            {-.85f, 1.28f, .18f, .04f}, {-.65f, 1.05f, .38f, .23f}, {-.48f, .85f, .435f, .40f}, {-.15f, .78f, .445f, .42f}, {.20f, .79f, .445f, .425f}, {.44f, .84f, .44f, .43f}, {.68f, .98f, .435f, .40f}, {.84f, 1.17f, .41f, .29f}, {1.02f, 1.37f, .35f, .10f}, {1.30f, 1.46f, .26f, .035f}};
        Loft(body[Shell], hull);
        std::vector<Section> saddle = {
            {-1.70f, 1.40f, .19f, .035f}, {-1.45f, 1.49f, .25f, .040f}, {-1.10f, 1.45f, .28f, .050f}, {-.78f, 1.33f, .29f, .055f}, {-.48f, 1.23f, .28f, .045f}, {-.17f, 1.20f, .25f, .035f}, {.16f, 1.24f, .22f, .030f}, {.38f, 1.30f, .16f, .025f}};
        Loft(body[Armour], saddle);
        Ellipsoid(body[Rubber], Vec(-.40f, 0, 1.20f), Vec(.27f, .19f, .045f), 32, 12);
        for (int side = -1; side <= 1; side += 2)
        {
            float s = float(side);
            FairingBand(body[Light], hull, side > 0 ? -.58f : Pi + .58f, .20f);
            FairingBand(body[SubtleLight], saddle, side > 0 ? .12f : Pi - .12f, .11f);
            // Transverse panel boundaries follow the same curved surface.
            for (float station : {.27f, .59f, .78f})
            {
                std::vector<Vec> seam;
                for (int i = 0; i <= 24; ++i)
                {
                    float angle = side > 0 ? -.68f + i * .072f : Pi + .68f - i * .072f;
                    seam.push_back(LoftPoint(hull, station, angle) + Vec(0, s * .003f, 0));
                }
                Tube(body[Recess], seam, .004f, 6);
            }
            // Inset power-unit window: a dark bowl, a rounded painted lip and
            // individually modelled fins inside it. No bright cylinder hangs
            // below the rider or replaces the chassis side panel.
            std::vector<Vec> lip;
            Vec windowCenter(-.035f, s * .456f, .835f), normal(0, s, 0);
            for (int i = 0; i <= 64; ++i)
            {
                float a = 2 * Pi * i / 64;
                Vec edge = windowCenter + Vec(.365f * std::cos(a), 0, .145f * std::sin(a));
                lip.push_back(edge + Vec(0, s * .022f, 0));
                if (i > 0)
                {
                    float previous = 2 * Pi * (i - 1) / 64;
                    Vec before = windowCenter + Vec(.365f * std::cos(previous), 0, .145f * std::sin(previous));
                    body[Recess].Triangle(Vertex(windowCenter, normal), Vertex(before, normal), Vertex(edge, normal));
                }
            }
            Tube(body[Armour], lip, .025f, 10);
            for (int fin = 0; fin < 9; ++fin)
            {
                float x = -.285f + .060f * fin;
                Rod(body[Metal], Vec(x, s * .463f, .755f), Vec(x, s * .463f, .915f), .010f, .010f, 8);
                Rod(body[SubtleLight], Vec(x + .015f, s * .466f, .770f), Vec(x + .015f, s * .466f, .900f), .006f, .006f, 6);
            }
            // The foot rest and hand controls are nestled into the fairing;
            // nothing crosses the large hubless wheel openings.
            Rod(body[Armour], Vec(-.29f, s * .34f, .46f), Vec(-.29f, s * .40f, .34f), .041f, .032f, 16);
            Rod(body[Recess], Vec(-.29f, s * .39f, .34f), Vec(-.29f, s * .47f, .34f), .036f, .036f, 16);
            Rod(body[Rubber], Vec(1.12f, s * .25f, 1.12f), Vec(1.14f, s * .39f, 1.12f), .031f, .031f, 18);
        }
        for (int index = 0; index < 2; ++index)
        {
            Vec center(index ? FrontAxle : RearAxle, 0, WheelRadius);
            float start = (index ? -22.f : 15.f) * Pi / 180;
            float finish = (index ? 157.f : 202.f) * Pi / 180;
            // Wide integrated shoulders wrap the wheels; no raised motorcycle
            // mudguards, fork rods, axle caps or exposed suspension pistons.
            Lathe(body[Shell], {{-.447f, .668f}, {-.451f, .699f}, {-.425f, .739f}, {-.34f, .769f}, {-.18f, .783f}, {0, .790f}, {.18f, .783f}, {.34f, .769f}, {.425f, .739f}, {.451f, .699f}, {.447f, .668f}, {.41f, .704f}, {.28f, .736f}, {0, .747f}, {-.28f, .736f}, {-.41f, .704f}},
                  center, start, finish, 112);
            CrownBand(body[Light], center, -.025f, .025f, .795f, start + .02f, finish - .02f);
            for (int side = -1; side <= 1; side += 2)
            {
                std::vector<Vec> seam;
                for (int i = 0; i <= 24; ++i)
                {
                    float a = start + (finish - start) * i / 24;
                    seam.push_back(center + Vec(.746f * std::cos(a), side * .423f, .746f * std::sin(a)));
                }
                Tube(body[Recess], seam, .005f, 6);
            }
        }
        // Vertical nose lights sit flush in the front housing, like the film's
        // narrow slit lamps. White is confined to these functional lights.
        for (int side = -1; side <= 1; side += 2)
        {
            Vec center(FrontAxle, 0, WheelRadius);
            float y = side * .255f;
            CrownBand(body[Recess], center, y - .038f, y + .038f, .796f, -.28f, .55f);
            CrownBand(body[Headlight], center, y - .013f, y + .013f, .799f, -.23f, .50f);
        }
        // Rear optical spine connects the wheel glow to the emitted light wall.
        CrownBand(body[Light], Vec(RearAxle, 0, WheelRadius), -.035f, .035f, .797f,
                  .78f * Pi, 1.12f * Pi);
    }

    void BuildRider()
    {
        // A forward racing tuck carries the shoulders toward the controls.
        // The chest, neck and arms share that pose instead of placing a round
        // head on an upright mannequin above a separate motorcycle frame.
        std::vector<Section> torso = {
            {-.66f, 1.32f, .105f, .10f}, {-.54f, 1.37f, .195f, .16f}, {-.34f, 1.43f, .205f, .16f}, {-.13f, 1.49f, .184f, .155f}, {.11f, 1.56f, .232f, .185f}, {.35f, 1.61f, .274f, .20f}, {.52f, 1.64f, .260f, .175f}, {.67f, 1.65f, .125f, .095f}};
        std::vector<BodySection> torsoBody;
        for (Section const& section : torso)
            torsoBody.push_back({Vec(section.x, 0, section.z), section.width, section.height});
        BodyVolume(rider[Suit], torsoBody);
        // Thin fitted shoulder/back panels catch broad room light, while the
        // joints and torso underneath retain the rough woven suit finish.
        for (int side = -1; side <= 1; side += 2)
            BodyPanel(rider[Armour], torsoBody, .48f, .84f, side > 0 ? .72f : Pi - .72f, .92f, .003f);
        std::vector<BodySection> neck = {
            {Vec(.61f, 0, 1.64f), .092f, .075f},
            {Vec(.69f, 0, 1.70f), .087f, .072f},
            {Vec(.77f, 0, 1.76f), .079f, .070f}};
        BodyVolume(rider[Suit], neck);
        BodyPanel(rider[Armour], neck, 0, .22f, Pi / 2, 2.6f, .004f);

        // One helmet shell has a taller cranium, narrowed jaw, and integrated
        // chin bar. The lower half is shorter than the crown; there is no second
        // sphere projecting from the face like a beak.
        Vec helmet(.87f, 0, 1.82f);
        auto helmetPoint = [&](float angle, float latitude)
        {
            float x = std::cos(latitude) * std::cos(angle);
            float y = std::cos(latitude) * std::sin(angle);
            float z = std::sin(latitude);
            float jaw = std::max(0.f, -z), crown = std::max(0.f, z);
            Vec p(x * (.245f - .027f * jaw) + .022f * jaw - .010f * crown,
                  y * (.172f - .034f * jaw), z * (z < 0 ? .160f : .195f));
            // A slight downward head pitch preserves a forward-looking visor.
            float pitch = .10f;
            return helmet + Vec(p.x * std::cos(pitch) + p.z * std::sin(pitch), p.y,
                                -p.x * std::sin(pitch) + p.z * std::cos(pitch));
        };
        auto helmetVertex = [&](float angle, float latitude, float offset)
        {
            float normalLatitude = std::max(-Pi / 2 + .015f, std::min(Pi / 2 - .015f, latitude));
            Vec along = helmetPoint(angle + .001f, normalLatitude) - helmetPoint(angle - .001f, normalLatitude);
            Vec upward = helmetPoint(angle, normalLatitude + .001f) - helmetPoint(angle, normalLatitude - .001f);
            Vec normal = Unit(Cross(Unit(along), Unit(upward)));
            return Vertex(helmetPoint(angle, latitude) + normal * offset, normal);
        };
        Surface(rider[Shell], 56, 32, [&](float u, float v)
                { return helmetVertex(2 * Pi * u, Pi * (v - .5f), 0); });
        Surface(rider[Visor], 40, 16, [&](float u, float v)
                { return helmetVertex(-1.38f + 2.76f * u, -.42f + .77f * v, .004f); });
        std::vector<Vec> brow, jawLine;
        for (int i = 0; i <= 36; ++i)
        {
            float a = -1.36f + 2.72f * i / 36;
            brow.push_back(helmetVertex(a, .36f, .006f).position);
            jawLine.push_back(helmetVertex(a, -.47f, .004f).position);
        }
        Tube(rider[SubtleLight], brow, .007f, 8);
        Tube(rider[SubtleLight], jawLine, .004f, 8);

        for (int side = -1; side <= 1; side += 2)
        {
            float s = float(side), outside = side > 0 ? 0.f : Pi;
            // Deltoid, biceps, elbow, forearm, wrist: the elbow sits outboard of
            // the fairing and the forearm leads visibly to a grasped handlebar.
            std::vector<BodySection> upperArm = {
                {Vec(.44f, s * .245f, 1.64f), .105f, .106f},
                {Vec(.49f, s * .310f, 1.60f), .119f, .115f},
                {Vec(.57f, s * .388f, 1.48f), .111f, .102f},
                {Vec(.65f, s * .445f, 1.39f), .089f, .086f},
                {Vec(.70f, s * .472f, 1.345f), .077f, .075f}};
            std::vector<BodySection> forearm = {
                {Vec(.68f, s * .470f, 1.355f), .078f, .078f},
                {Vec(.83f, s * .478f, 1.255f), .094f, .089f},
                {Vec(1.00f, s * .478f, 1.145f), .077f, .074f},
                {Vec(1.16f, s * .468f, 1.070f), .056f, .056f},
                {Vec(1.275f, s * .458f, 1.055f), .051f, .051f}};
            BodyVolume(rider[Suit], upperArm);
            BodyVolume(rider[Suit], forearm);
            BodyPanel(rider[Armour], upperArm, .02f, .39f, outside, 1.65f, .004f);
            BodyPanel(rider[Armour], forearm, .12f, .59f, outside, 1.20f, .004f);
            BodyPanel(rider[Light], upperArm, .12f, .84f, outside, .18f, .006f);
            BodyPanel(rider[SubtleLight], forearm, .18f, .80f, outside, .15f, .006f);

            // Raised grips sit just outside the upper shell. The gloved palm,
            // curled fingers and thumb actually close around the control.
            Rod(rider[Armour], Vec(.94f, s * .34f, 1.18f), Vec(1.335f, s * .425f, 1.052f), .022f, .019f, 16);
            Rod(rider[Rubber], Vec(1.335f, s * .405f, 1.052f), Vec(1.335f, s * .530f, 1.052f), .027f, .027f, 20);
            std::array<std::size_t, MaterialCount> handStart;
            for (int material = 0; material < MaterialCount; ++material)
                handStart[material] = rider[material].vertices.size();
            std::vector<BodySection> hand = {
                {Vec(1.025f, s * .458f, 1.340f), .051f, .048f},
                {Vec(1.075f, s * .462f, 1.353f), .062f, .044f},
                {Vec(1.122f, s * .465f, 1.352f), .063f, .034f},
                {Vec(1.143f, s * .466f, 1.333f), .052f, .025f}};
            BodyVolume(rider[Rubber], hand);
            BodyPanel(rider[Armour], hand, .20f, .68f, Pi / 2, 1.4f, .003f);
            for (int finger = 0; finger < 4; ++finger)
            {
                float y = s * (.425f + .026f * finger);
                Tube(rider[Suit], {{1.12f, y, 1.369f}, {1.143f, y, 1.352f}, {1.142f, y, 1.324f}, {1.12f, y, 1.308f}}, .012f, 8);
            }
            Tube(rider[Suit], {{1.062f, s * .414f, 1.342f}, {1.085f, s * .408f, 1.315f}, {1.112f, s * .423f, 1.310f}}, .018f, 10);
            for (int material = 0; material < MaterialCount; ++material)
                for (std::size_t vertex = handStart[material]; vertex < rider[material].vertices.size(); ++vertex)
                    rider[material].vertices[vertex].position =
                        rider[material].vertices[vertex].position + Vec(.230f, 0, -.285f);

            // The femur slopes forward from a seated pelvis to an exposed knee;
            // the shin folds back to the peg. Both legs sit outside the power
            // unit instead of being swallowed by its luminous cylinders.
            std::vector<BodySection> thigh = {
                {Vec(-.47f, s * .170f, 1.345f), .139f, .150f},
                {Vec(-.38f, s * .285f, 1.305f), .150f, .157f},
                {Vec(-.23f, s * .398f, 1.225f), .141f, .144f},
                {Vec(-.04f, s * .476f, 1.110f), .117f, .126f},
                {Vec(.105f, s * .500f, 1.015f), .095f, .101f}};
            std::vector<BodySection> calf = {
                {Vec(.11f, s * .500f, 1.015f), .095f, .102f},
                {Vec(.065f, s * .510f, .930f), .106f, .118f},
                {Vec(-.075f, s * .520f, .790f), .107f, .123f},
                {Vec(-.225f, s * .505f, .615f), .080f, .093f},
                {Vec(-.36f, s * .483f, .455f), .060f, .068f}};
            BodyVolume(rider[Suit], thigh);
            BodyVolume(rider[Suit], calf);
            // A compact knee protector follows the bend, not the reactor axis.
            std::vector<BodySection> knee = {
                {Vec(.02f, s * .501f, 1.085f), .098f, .094f},
                {Vec(.12f, s * .506f, 1.025f), .104f, .105f},
                {Vec(.10f, s * .512f, .956f), .102f, .103f}};
            BodyPanel(rider[Armour], knee, 0, 1, outside, 1.50f, .006f);
            BodyPanel(rider[Armour], calf, .18f, .55f, outside, .95f, .004f);
            BodyPanel(rider[Light], thigh, .13f, .82f, outside, .16f, .005f);
            BodyPanel(rider[SubtleLight], calf, .19f, .88f, outside, .13f, .005f);

            std::vector<BodySection> boot = {
                {Vec(-.50f, s * .483f, .379f), .064f, .066f},
                {Vec(-.43f, s * .485f, .398f), .081f, .111f},
                {Vec(-.34f, s * .487f, .390f), .086f, .107f},
                {Vec(-.23f, s * .487f, .355f), .086f, .061f},
                {Vec(-.115f, s * .481f, .346f), .075f, .046f},
                {Vec(-.070f, s * .479f, .349f), .040f, .034f}};
            BodyVolume(rider[Rubber], boot);
            BodyPanel(rider[Armour], boot, .24f, .67f, Pi / 2, 1.75f, .004f);
            BodyPanel(rider[SubtleLight], boot, .17f, .82f, outside, .070f, .006f);
            Rod(rider[Metal], Vec(-.29f, s * .36f, .315f), Vec(-.29f, s * .56f, .315f), .025f, .025f, 16);

            // Restrained inlays trace the anatomy of the back and side of the
            // ribcage. They no longer form a second luminous motorcycle frame.
            BodyPanel(rider[SubtleLight], torsoBody, .10f, .94f, side > 0 ? .35f : Pi - .35f, .13f, .005f);
            BodyPanel(rider[Light], torsoBody, .12f, .90f, side > 0 ? .95f : Pi - .95f, .23f, .005f);
        }
        // The identity disc lies on the upper back, above the saddle. Its dark
        // housing and small luminous ring read as equipment worn by the person.
        Vertex back = BodyVertex(torsoBody, .60f, Pi / 2, .018f);
        Vec discCenter = back.position - back.normal * .030f;
        Vec discAxis = back.normal, discAcross(0, 1, 0);
        Vec discAlong = Unit(Cross(discAcross, discAxis));
        Rod(rider[Recess], discCenter - discAxis * .011f, discCenter + discAxis * .013f, .145f, .145f, 40);
        std::vector<Vec> disc;
        for (int i = 0; i <= 48; ++i)
        {
            float a = 2 * Pi * i / 48;
            disc.push_back(discCenter + discAxis * .017f + discAlong * (.126f * std::cos(a)) + discAcross * (.126f * std::sin(a)));
        }
        Tube(rider[Light], disc, .016f, 10);
    }
};

inline void SetMaterial(Material material, float r, float g, float b)
{
    GLfloat ambient[4] = {0, 0, 0, 1}, diffuse[4] = {0, 0, 0, 1};
    GLfloat specular[4] = {0, 0, 0, 1}, emission[4] = {0, 0, 0, 1};
    // Explicit roughness, metal, coat and cloth weights. Inferring these from
    // diffuse colour made the black suit and rubber share the fairing's sheen.
    GLfloat finish[4] = {.65f, 0, 0, 0};
    switch (material)
    {
    case Shell:
        finish[0] = .38f;
        finish[2] = .06f;
        break;
    case Armour:
        finish[0] = .58f;
        finish[2] = .04f;
        break;
    case Metal:
        finish[0] = .42f;
        finish[1] = .70f;
        break;
    case Rubber:
        finish[0] = .78f;
        break;
    case Recess:
        finish[0] = .86f;
        break;
    case Suit:
        finish[0] = .82f;
        finish[3] = 1;
        break;
    case Visor:
        finish[0] = .14f;
        finish[2] = 1;
        break;
    default:
        break;
    }
#ifdef HAVE_GLEW
    if (GLEW_VERSION_1_3)
        glMultiTexCoord4fv(GL_TEXTURE2, finish);
#endif
    float shine = 64;
    if (material == Light || material == SubtleLight || material == Headlight)
    {
        float power = material == SubtleLight ? .55f : 1.f;
        emission[0] = power * r;
        emission[1] = power * g;
        emission[2] = power * b;
        if (material == Headlight)
        {
            emission[0] = .77f;
            emission[1] = .87f;
            emission[2] = .95f;
        }
    }
    else
    {
        Vec base, highlight;
        switch (material)
        {
        case Shell:
            base = Vec(.0025f, .0035f, .005f);
            highlight = Vec(.15f, .15f, .15f);
            shine = 80;
            break;
        case Armour:
            base = Vec(.002f, .003f, .004f);
            highlight = Vec(.12f, .12f, .12f);
            shine = 48;
            break;
        case Metal:
            base = Vec(.006f, .009f, .012f);
            highlight = Vec(.06f, .08f, .10f);
            shine = 60;
            break;
        case Rubber:
            base = Vec(.016f, .019f, .024f);
            highlight = Vec(.21f, .25f, .30f);
            shine = 31;
            break;
        case Recess:
            base = Vec(.010f, .014f, .018f);
            highlight = Vec(.07f, .10f, .13f);
            shine = 24;
            break;
        case Suit:
            base = Vec(.002f, .003f, .004f);
            highlight = Vec(.12f, .12f, .12f);
            shine = 48;
            break;
        case Visor:
            base = Vec(.012f, .031f, .043f);
            highlight = Vec(.84f, .95f, 1.f);
            shine = 120;
            break;
        default:
            base = Vec(.04f, .05f, .06f);
            highlight = Vec(.4f, .4f, .4f);
            break;
        }
        diffuse[0] = base.x;
        diffuse[1] = base.y;
        diffuse[2] = base.z;
        ambient[0] = base.x * .48f;
        ambient[1] = base.y * .48f;
        ambient[2] = base.z * .48f;
        // Small reflected neon contributions separate the black shell from the
        // suit and light the recessed metal barrels without bleaching the hue.
        float bounce = material == Metal ? .055f : material == Shell ? .018f
                                                                     : .006f;
        ambient[0] += r * bounce;
        ambient[1] += g * bounce;
        ambient[2] += b * bounce;
        specular[0] = highlight.x;
        specular[1] = highlight.y;
        specular[2] = highlight.z;
    }
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emission);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shine);
}

inline void Draw(Parts const& parts, float r, float g, float b)
{
    for (int i = 0; i < MaterialCount; ++i)
        if (!parts[i].vertices.empty())
        {
            SetMaterial(static_cast<Material>(i), r, g, b);
            parts[i].Draw();
        }
}

// CPU geometry stays canonical across screen-mode changes. The optional VBO cache
// is released before context destruction and lazily repopulated by the next Draw.
inline Cache const& Geometry()
{
    static Cache const cache;
    return cache;
}

struct AuthoredCache
{
    std::array<Parts, 4> groups;
    std::array<std::array<float, 3>, 2> centres;
    std::array<std::array<float, 2>, 2> opticalRings;
    bool loaded = false;
    AuthoredCache()
    {
        tString path = tDirectories::Data().GetReadPath("models/lightcycle-authored.bin");
        std::ifstream file(static_cast<char const*>(path), std::ios::in | std::ios::binary);
        gLightcycleAsset::Asset asset;
        if (!file || !gLightcycleAsset::Load(file, asset))
        {
            con << "Authored lightcycle unavailable or invalid; using built-in fallback.\n";
            return;
        }
        std::size_t count = 0;
        for (unsigned group = 0; group < 4; ++group)
            for (unsigned material = 0; material < MaterialCount; ++material)
            {
                auto& vertices = groups[group][material].vertices;
                vertices.reserve(asset.parts[group][material].size());
                for (auto const& source : asset.parts[group][material])
                {
                    Vertex v(Vec(source.position[0], source.position[1], source.position[2]),
                             Vec(source.normal[0], source.normal[1], source.normal[2]));
                    std::copy(source.optical, source.optical + 4, v.optical);
                    vertices.push_back(v);
                }
                count += vertices.size();
            }
        for (int wheel = 0; wheel < 2; ++wheel)
        {
            float low = 10.f, high = 0.f;
            for (auto const& vertex : groups[wheel + 1][Light].vertices)
            {
                float radius = std::hypot(vertex.position.x, vertex.position.z);
                low = std::min(low, radius);
                high = std::max(high, radius);
            }
            opticalRings[wheel] = {{(low + high) * .5f, std::max(.001f, (high - low) * .5f)}};
        }
        centres = asset.centres;
        loaded = true;
        if (sr_hiddenWindow)
            std::cerr << "Authored lightcycle loaded: " << count / 3 << " triangles\n";
    }
};
inline AuthoredCache const& AuthoredGeometry()
{
    static AuthoredCache const cache;
    return cache;
}

inline void DrawAuthored(Parts const& parts, float r, float g, float b, float ringCentre = 0, float ringHalfWidth = 0)
{
    for (int i = 0; i < MaterialCount; ++i)
        if (!parts[i].vertices.empty())
        {
            SetMaterial(static_cast<Material>(i), r, g, b);
#ifdef HAVE_GLEW
            if (GLEW_VERSION_1_3)
                glMultiTexCoord3f(GL_TEXTURE1, i == Suit ? 2.f : i == Shell ? 3.f
                                                                            : 1.f,
                                  ringCentre, ringHalfWidth);
#endif
            parts[i].Draw();
        }
}
} // namespace Detail

bool AuthoredAvailable() { return Detail::AuthoredGeometry().loaded; }
float RearEmitterX()
{
    return AuthoredAvailable() ? gLightcycleProfile::RearExtent : RearAxle - WheelRadius - .08f;
}
float ModelWheelRadius(int wheel)
{
    return AuthoredAvailable() ? gLightcycleProfile::Radii[wheel ? 1 : 0] : WheelRadius;
}
float RearProfileX(float height)
{
    if (!AuthoredAvailable())
        return RearEmitterX();
    auto const& p = gLightcycleProfile::Rear;
    for (unsigned i = 1; i < 33; ++i)
        if (height <= p[i][0])
        {
            float t = std::max(0.f, std::min(1.f, (height - p[i - 1][0]) / (p[i][0] - p[i - 1][0])));
            return p[i - 1][1] + t * (p[i][1] - p[i - 1][1]);
        }
    return p[32][1];
}

void Render(float r, float g, float b, float rearAngle, float frontAngle)
{
    r = std::max(0.f, std::min(1.f, r));
    g = std::max(0.f, std::min(1.f, g));
    b = std::max(0.f, std::min(1.f, b));
    Detail::AuthoredCache const& authored = Detail::AuthoredGeometry();

    GLint oldMatrixMode = GL_MODELVIEW;
    glGetIntegerv(GL_MATRIX_MODE, &oldMatrixMode);
    glPushAttrib(GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT | GL_TEXTURE_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT | GL_DEPTH_BUFFER_BIT);
    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
#ifdef HAVE_GLEW
    GLint oldArrayBuffer = 0;
    if (GLEW_VERSION_1_5)
    {
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &oldArrayBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    if (GLEW_VERSION_1_3)
    {
        GLint units = 1;
        glGetIntegerv(GL_MAX_TEXTURE_UNITS, &units);
        for (GLint unit = 0; unit < units; ++unit)
        {
            glActiveTexture(GL_TEXTURE0 + unit);
            glDisable(GL_TEXTURE_1D);
            glDisable(GL_TEXTURE_2D);
            glDisable(GL_TEXTURE_3D);
            glDisable(GL_TEXTURE_CUBE_MAP);
            glDisable(GL_TEXTURE_GEN_S);
            glDisable(GL_TEXTURE_GEN_T);
            glDisable(GL_TEXTURE_GEN_R);
            glDisable(GL_TEXTURE_GEN_Q);
            glClientActiveTexture(GL_TEXTURE0 + unit);
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        }
        glActiveTexture(GL_TEXTURE0);
        glClientActiveTexture(GL_TEXTURE0);
    }
    if (GLEW_VERSION_1_4)
    {
        glDisableClientState(GL_SECONDARY_COLOR_ARRAY);
        glDisableClientState(GL_FOG_COORD_ARRAY);
    }
#endif
    glDisable(GL_TEXTURE_1D);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glEnable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_INDEX_ARRAY);
    glDisableClientState(GL_EDGE_FLAG_ARRAY);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

#ifdef HAVE_GLEW
    if (GLEW_VERSION_1_3)
        glMultiTexCoord1f(GL_TEXTURE3, 0.f);
#endif
    if (authored.loaded)
    {
        static rFileTexture mask(rTextureGroups::TEX_OBJ, "textures/lightcycle_suit_mask.png");
        mask.Select(true);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        Detail::DrawAuthored(authored.groups[0], r, g, b);
        for (int wheel = 0; wheel < 2; ++wheel)
        {
            glPushMatrix();
            auto const& c = authored.centres[wheel];
            glTranslatef(c[0], c[1], c[2]);
            glRotatef(-(wheel ? frontAngle : rearAngle) * 180.f / Detail::Pi, 0, 1, 0);
            Detail::DrawAuthored(authored.groups[wheel + 1], r, g, b, authored.opticalRings[wheel][0], authored.opticalRings[wheel][1]);
            glPopMatrix();
        }
#ifdef HAVE_GLEW
        if (GLEW_VERSION_1_3)
            glMultiTexCoord1f(GL_TEXTURE3, 1.f);
#endif
        Detail::DrawAuthored(authored.groups[3], r, g, b);
    }
    else
    {
        Detail::Cache const& cache = Detail::Geometry();
#ifdef HAVE_GLEW
        if (GLEW_VERSION_1_3)
            glMultiTexCoord1f(GL_TEXTURE1, 0.f);
#endif
        Detail::Draw(cache.body, r, g, b);
        for (int wheel = 0; wheel < 2; ++wheel)
        {
            glPushMatrix();
            glTranslatef(wheel ? FrontAxle : RearAxle, 0, WheelRadius);
            glRotatef(-(wheel ? frontAngle : rearAngle) * 180.f / Detail::Pi, 0, 1, 0);
            Detail::Draw(cache.wheel, r, g, b);
            glPopMatrix();
        }
#ifdef HAVE_GLEW
        if (GLEW_VERSION_1_3)
            glMultiTexCoord1f(GL_TEXTURE3, 1.f);
#endif
        Detail::Draw(cache.rider, r, g, b);
    }
#ifdef HAVE_GLEW
    if (GLEW_VERSION_1_3)
    {
        glMultiTexCoord1f(GL_TEXTURE3, 0.f);
        glMultiTexCoord1f(GL_TEXTURE1, 0.f);
    }
#endif

    glPopMatrix();
    glMatrixMode(oldMatrixMode);
#ifdef HAVE_GLEW
    if (GLEW_VERSION_1_5)
        glBindBuffer(GL_ARRAY_BUFFER, oldArrayBuffer);
#endif
    glPopClientAttrib();
    glPopAttrib();
}

} // namespace gLightcycleModel
#endif
