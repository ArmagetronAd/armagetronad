// Bounded reader for the local, exported lightcycle mesh. No rendering or I/O
// paths are embedded in the asset. GPL version 2 or later, like the renderer.
#ifndef ARMAGETRON_LIGHTCYCLE_ASSET_H
#define ARMAGETRON_LIGHTCYCLE_ASSET_H
#include <array>
#include <vector>
#include <istream>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <utility>

namespace gLightcycleAsset
{
struct Vertex
{
    float position[3], normal[3], optical[4];
};
struct Asset
{
    std::array<std::array<std::vector<Vertex>, 10>, 4> parts;
    std::array<std::array<float, 3>, 2> centres;
};
inline bool Word(std::istream& in, std::uint32_t& value)
{
    unsigned char bytes[4];
    if (!in.read(reinterpret_cast<char*>(bytes), 4))
        return false;
    value = std::uint32_t(bytes[0]) | (std::uint32_t(bytes[1]) << 8) | (std::uint32_t(bytes[2]) << 16) | (std::uint32_t(bytes[3]) << 24);
    return true;
}
inline bool Number(std::istream& in, float& value)
{
    static_assert(sizeof(float) == 4, "Asset requires 32-bit IEEE floats");
    std::uint32_t bits;
    if (!Word(in, bits))
        return false;
    std::memcpy(&value, &bits, 4);
    return std::isfinite(value);
}
inline bool Load(std::istream& in, Asset& destination)
{
    Asset result;
    char magic[8];
    std::uint32_t groups, materials;
    if (!in.read(magic, 8) || std::memcmp(magic, "LCMESH1\0", 8) || !Word(in, groups) || groups != 4 || !Word(in, materials) || materials != 10)
        return false;
    for (auto& centre : result.centres)
        for (float& value : centre)
            if (!Number(in, value) || std::fabs(value) > 5)
                return false;
    if (std::fabs(result.centres[0][0] + 1.45f) > .001f || std::fabs(result.centres[1][0] - 1.55f) > .001f)
        return false;
    std::uint32_t total = 0;
    for (auto& group : result.parts)
        for (auto& part : group)
        {
            std::uint32_t count;
            if (!Word(in, count) || count % 3 || count > 600000 || total + count > 1200000)
                return false;
            total += count;
            part.resize(count);
            for (Vertex& vertex : part)
            {
                for (float& value : vertex.position)
                    if (!Number(in, value) || std::fabs(value) > 8)
                        return false;
                float length = 0;
                for (float& value : vertex.normal)
                {
                    if (!Number(in, value))
                        return false;
                    length += value * value;
                }
                if (length < .99f || length > 1.01f)
                    return false;
                for (float& value : vertex.optical)
                    if (!Number(in, value) || std::fabs(value) > 16)
                        return false;
            }
        }
    if (total == 0 || in.peek() != std::char_traits<char>::eof())
        return false;
    destination = std::move(result);
    return true;
}
} // namespace gLightcycleAsset
#endif
