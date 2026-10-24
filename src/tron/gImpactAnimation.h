// Deterministic presentation sampling; never consumes simulation randomness.
// GPL-2.0-or-later.
#ifndef ARMAGETRON_IMPACT_ANIMATION_H
#define ARMAGETRON_IMPACT_ANIMATION_H
#include <algorithm>
#include <cmath>

namespace gImpactAnimation
{
constexpr unsigned FragmentCount = 216;
constexpr unsigned RimFragmentCount = 48;
constexpr float Duration = 1.9f;
inline float Hash(unsigned value)
{
    value = (value ^ (value >> 16)) * 0x7feb352du;
    value = (value ^ (value >> 15)) * 0x846ca68bu;
    return float((value ^ (value >> 16)) & 65535u) / 65535.f;
}
struct Fragment
{
    float along = 0, across = 0, height = 0, sizeX = 0, sizeY = 0, sizeZ = 0, spin = 0, fade = 0;
    bool Active() const { return fade > 0; }
};
inline Fragment Sample(unsigned index, float age, float speed)
{
    Fragment f;
    if (!std::isfinite(age) || age < 0 || age >= Duration || !std::isfinite(speed))
        return f;
    float h = Hash(index * 7 + 1), j = Hash(index * 7 + 2), k = Hash(index * 7 + 3);
    float life = .90f + .95f * Hash(index * 7 + 4);
    bool fine = index >= 144;
    float delay = index < 24 ? 0.f : .015f + .065f * h;
    if (fine)
        delay += .04f + .06f * k;
    float t = age - delay;
    if (t < 0 || t >= life)
        return f;
    speed = std::max(0.f, std::min(70.f, speed));
    float launch = 1 - std::exp(-t * 3.2f);
    // A finite impulse carries the incoming momentum, then drag slows it.
    // Pieces begin across the bike's volume behind the point of contact.
    f.along = -.25f - 1.55f * h + launch * (speed * .085f + 1.9f * (h - .30f));
    f.across = (j - .5f) * .70f + (j - .5f) * launch * (3.2f + speed * .022f);
    float z = .15f + .95f * k + (1.4f + 3.0f * k) * t - 4.8f * t * t;
    // A damped floor bounce retains depth without dropping fragments through it.
    f.height = z >= .025f ? z : .025f + std::min(.35f, std::max(0.f, -z * .16f)) * std::exp(-t * 2.f);
    f.fade = 1 - t / life;
    float size = index < 24 ? .085f + .095f * Hash(index * 7 + 5) : fine ? .007f + .022f * Hash(index * 7 + 5)
                                                                         : .018f + .050f * Hash(index * 7 + 5);
    f.sizeX = size * (.8f + 1.8f * h);
    f.sizeY = size * (.35f + .55f * j);
    f.sizeZ = size * (.10f + .24f * k);
    f.spin = index * 2.3999632f + t * (2.5f + 11.f * j);
    return f;
}

// Recognizable fragments retain the two wheel silhouettes at contact, then
// separate and tumble before the finer debris finishes dissolving. Coordinates
// use the same authored axles, emitter radius and -1.5 origin as the bike mesh.
struct RimFragment
{
    float along = 0, across = 0, height = 0, radius = 0, angle = 0, pitch = 0, roll = 0, fade = 0;
    bool Active() const { return fade > 0; }
};
inline RimFragment SampleRim(unsigned index, float age, float speed, float modelScale)
{
    RimFragment f;
    if (index >= RimFragmentCount || !std::isfinite(age) || age < 0 || age >= Duration ||
        !std::isfinite(speed) || !std::isfinite(modelScale) || modelScale <= 0)
        return f;
    unsigned sector = index % 12, ring = index / 12;
    bool front = ring >= 2;
    float side = ring % 2 ? 1.f : -1.f;
    float h = Hash(index * 11 + 1103), j = Hash(index * 11 + 1105), k = Hash(index * 11 + 1109);
    float delay = (front ? 0.f : .055f) + .035f * h;
    float t = std::max(0.f, age - delay);
    float life = .62f + .36f * j;
    if (t >= life)
        return f;
    speed = std::max(0.f, std::min(70.f, speed));
    f.angle = (sector + .5f) * (6.2831853f / 12.f);
    f.radius = .607f * modelScale;
    float launch = 1 - std::exp(-t * 4.f);
    f.along = ((front ? 1.55f : -1.45f) - 1.5f) * modelScale + f.radius * std::cos(f.angle) + launch * (speed * .055f + .9f * (h - .25f));
    f.across = side * .446f * modelScale + launch * (side * (.35f + .60f * j) + .25f * (h - .5f));
    float z = .72f * modelScale + f.radius * std::sin(f.angle) + (1.1f + 2.1f * k) * t - 4.8f * t * t;
    f.height = std::max(.035f, z);
    f.pitch = t * launch * (2.f + 7.f * j) * (h > .5f ? 1.f : -1.f);
    f.roll = t * launch * (2.f + 6.f * k) * side;
    f.fade = 1 - t / life;
    return f;
}
} // namespace gImpactAnimation
#endif
