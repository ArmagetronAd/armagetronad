#include "doctest.h"
#include "gLightcycleAsset.h"
#include "tDirectories.h"

#include <fstream>
#include <iterator>
#include <sstream>

TEST_CASE("Lightcycle mesh rejects malformed data without losing a loaded model")
{
    tString path = tDirectories::Data().GetReadPath("models/lightcycle-authored.bin");
    std::ifstream input(static_cast<char const*>(path), std::ios::binary);
    REQUIRE(input.good());
    std::string bytes((std::istreambuf_iterator<char>(input)), {});
    gLightcycleAsset::Asset asset;
    std::istringstream valid(bytes);
    REQUIRE(gLightcycleAsset::Load(valid, asset));
    std::size_t count = 0;
    for (auto const& group : asset.parts)
        for (auto const& part : group)
            count += part.size();
    CHECK(count == 51744 * 3);
    CHECK(asset.parts[3][5].size() > 60000);
    CHECK_FALSE(asset.parts[1][3].empty());
    CHECK_FALSE(asset.parts[2][3].empty());

    auto reject = [&](std::string const& candidate)
    {
        std::istringstream stream(candidate);
        CHECK_FALSE(gLightcycleAsset::Load(stream, asset));
        CHECK(asset.parts[3][5].size() > 60000);
    };
    reject(bytes.substr(0, bytes.size() - 1));
    reject(bytes + "unexpected trailing data");
    std::string corrupt = bytes;
    corrupt[0] = '?';
    reject(corrupt);
    corrupt = bytes;
    corrupt[40] = char(0xff);
    corrupt[41] = char(0xff);
    corrupt[42] = char(0xff);
    corrupt[43] = char(0x7f);
    reject(corrupt);
    // The first coordinate becomes a quiet NaN in little-endian IEEE 754.
    corrupt = bytes;
    corrupt[44] = 0;
    corrupt[45] = 0;
    corrupt[46] = char(0xc0);
    corrupt[47] = char(0x7f);
    reject(corrupt);
}
