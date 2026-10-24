#include "doctest.h"
#include "ConfItemTempValue.h"
#include "MockConsole.h"
#include "gStuff.h"
#include "tConfiguration.h"

#include <memory>
#include <sstream>
#include <vector>

#ifndef DEDICATED
TEST_CASE("Saved graphics style overrides an old startup preset without changing display quality")
{
    MockConsole quiet;
    tCurrentAccessLevel owner(tAccessLevel_Owner, true);
    std::vector<std::unique_ptr<ConfItemTempValueBase>> saved;
    for (char const* key : {"GRAPHICS_STYLE", "BLACKLIGHT_VISUALS", "LIGHTCYCLE_SCALE",
                            "MOVIEPACK", "INDIVIDUAL_PLAYER_COLORS", "COCKPIT_FILE", "FLOOR_DETAIL",
                            "FLOOR_RED", "FLOOR_GREEN", "FLOOR_BLUE", "GRID_SIZE", "FLOOR_MIRROR",
                            "FLOOR_MIRROR_INT", "FLOOR_REFLECTION_SOFTNESS", "RIM_WALL_RED", "RIM_WALL_GREEN",
                            "RIM_WALL_BLUE", "UPPER_SKY", "LOWER_SKY", "SKY_WOBBLE", "BLOOM",
                            "BLOOM_INTENSITY", "BLOOM_THRESHOLD", "BLOOM_RADIUS", "BLOOM_HALO", "BLOOM_VIGNETTE"})
    {
        // The unit executable omits the cockpit and other renderer-only modules.
        // Restore every setting that is registered in this fixture; native client
        // captures exercise both complete presets with those modules present.
        if (tConfItemBase::FindConfigItem(tString(key)))
            saved.emplace_back(new ConfItemTempValueBase(key));
    }

    ConfItemTempValue<int> choice("GRAPHICS_STYLE"), fps("MAX_FPS");
    ConfItemTempValue<bool> legacy("BLACKLIGHT_VISUALS");
    ConfItemTempValue<int> width("ARMAGETRON_WINDOWSIZE_W"), height("ARMAGETRON_WINDOWSIZE_H");
#if SDL_VERSION_ATLEAST(2, 0, 0)
    ConfItemTempValue<int> samples("ANTIALIAS_SAMPLES");
    samples.SetValue(4);
#endif
    width.SetValue(3024);
    height.SetValue(1964);
    fps.SetValue(120);
    choice.SetValue(-1);
    legacy.SetValue(true);
    sg_ApplyGraphicsStyle();
    CHECK(legacy.GetCurrentValue());

    sg_SelectGraphicsStyle(0);
    CHECK_FALSE(legacy.GetCurrentValue());
    CHECK(choice.GetCurrentValue() == 0);
    // Simulate an autoexec loading BLACKLIGHT_VISUALS 1 after the saved choice.
    legacy.SetValue(true);
    sg_ApplyGraphicsStyle();
    CHECK_FALSE(legacy.GetCurrentValue());

    sg_SelectGraphicsStyle(1);
    CHECK(legacy.GetCurrentValue());
    CHECK(choice.GetCurrentValue() == 1);
    sg_SelectGraphicsStyle(99);
    CHECK(choice.GetCurrentValue() == 1);
    CHECK(legacy.GetCurrentValue());
    CHECK(fps.GetCurrentValue() == 120);
    CHECK(width.GetCurrentValue() == 3024);
    CHECK(height.GetCurrentValue() == 1964);
#if SDL_VERSION_ATLEAST(2, 0, 0)
    CHECK(samples.GetCurrentValue() == 4);
#endif

    // Exercise the same serialization used by user configuration, in memory.
    std::ostringstream encoded;
    tConfItemBase::FindConfigItem(tString("GRAPHICS_STYLE"))->WriteVal(encoded);
    choice.SetValue(0);
    std::istringstream restored("GRAPHICS_STYLE " + encoded.str());
    tConfItemBase::LoadLine(restored);
    sg_ApplyGraphicsStyle();
    CHECK(choice.GetCurrentValue() == 1);
    CHECK(legacy.GetCurrentValue());
}
#endif
