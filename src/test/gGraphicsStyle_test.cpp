#include "doctest.h"
#include "ConfItemTempValue.h"
#include "MockConsole.h"
#include "gGraphicsStyle.h"
#include "gStuff.h"
#include "tConfiguration.h"

#include <memory>
#include <sstream>
#include <string>
#include <vector>

#ifndef DEDICATED
namespace
{
struct StyleFixture
{
    MockConsole quiet;
    tCurrentAccessLevel owner{tAccessLevel_Owner, true};
    int mirror = 0;
    REAL intensity = .5f;
    tString cockpit{"Anonymous/standard-0.0.1.aacockpit.xml"};
    std::vector<std::unique_ptr<tConfItemBase>> frontendSettings;
    std::vector<std::unique_ptr<ConfItemTempValueBase>> saved;

    StyleFixture()
    {
        // The unit binary omits these frontend modules. Bind their configuration
        // dependencies using the same saved/unsaved setting types as production.
        if (!Item("FLOOR_MIRROR"))
            frontendSettings.emplace_back(new tConfItem<int>("FLOOR_MIRROR", mirror));
        if (!Item("BLOOM_INTENSITY"))
            frontendSettings.emplace_back(new tSettingItem<REAL>("BLOOM_INTENSITY", intensity));
        if (!Item("COCKPIT_FILE"))
            frontendSettings.emplace_back(new tConfItem<tString>("COCKPIT_FILE", cockpit));
        for (auto const& entry : tConfItemBase::GetConfItemMap())
        {
            std::string name = entry.first.c_str();
            bool appearance = name.find("GRAPHICS_PREF_") == 0 || name == "GRAPHICS_STYLE" ||
                              name == "BLACKLIGHT_VISUALS" || name == "MOVIEPACK" || name == "LIGHTCYCLE_SCALE" ||
                              name == "INDIVIDUAL_PLAYER_COLORS" || name == "COCKPIT_FILE" || name == "GRID_SIZE" ||
                              name.find("FLOOR_") == 0 || name.find("RIM_WALL_") == 0 || name.find("BLOOM") == 0 ||
                              name == "UPPER_SKY" || name == "LOWER_SKY" || name == "SKY_WOBBLE" ||
                              name == "MAX_FPS" || name == "ANTIALIAS_SAMPLES" ||
                              name == "ARMAGETRON_WINDOWSIZE_W" || name == "ARMAGETRON_WINDOWSIZE_H";
            if (appearance)
                saved.emplace_back(new ConfItemTempValueBase(name.c_str()));
        }
    }
    ~StyleFixture()
    {
        tCallbackConfigLoad::Exec(tConfigLoadPhase::Begin);
        saved.clear();
        tCallbackConfigLoad::Exec(tConfigLoadPhase::Complete);
    }
    static tConfItemBase* Item(char const* name)
    {
        return tConfItemBase::FindConfigItem(tString(name));
    }
    static void Set(char const* name, char const* value)
    {
        REQUIRE(Item(name));
        std::istringstream input(value);
        Item(name)->ReadVal(input);
    }
    static std::string Value(char const* name)
    {
        REQUIRE(Item(name));
        std::ostringstream output;
        Item(name)->WriteVal(output);
        return output.str();
    }
    static std::string Save()
    {
        std::ostringstream all;
        tConfItemBase::SaveAll(all);
        // Replay only appearance/preferences; unrelated commands may affect I/O.
        std::istringstream input(all.str());
        std::string line, result;
        while (std::getline(input, line))
        {
            std::istringstream row(line);
            std::string key;
            row >> key;
            if (key.find("GRAPHICS_") == 0 || key == "BLACKLIGHT_VISUALS" ||
                key == "FLOOR_MIRROR" || key == "COCKPIT_FILE")
                result += line + '\n';
        }
        return result;
    }
};
} // namespace

TEST_CASE("Graphics reconciliation and repeated selection preserve customized appearance")
{
    StyleFixture fixture;
    fixture.Set("BLACKLIGHT_VISUALS", "0");
    fixture.Set("MOVIEPACK", "0");
    sg_SelectGraphicsStyle(1);
    fixture.Set("FLOOR_MIRROR", "0");
    fixture.Set("BLOOM_INTENSITY", "0.73");
    fixture.Set("COCKPIT_FILE", "Local/custom-1.0.aacockpit.xml");
    auto customized = fixture.Save();

    sg_ApplyGraphicsStyle();
    CHECK(fixture.Save() == customized);
    sg_SelectGraphicsStyle(1);
    CHECK(fixture.Save() == customized);
    sg_SelectGraphicsStyle(99);
    CHECK(fixture.Save() == customized);
}

TEST_CASE("Appearance preferences survive loading resource defaults and explicit autoexec overrides")
{
    StyleFixture fixture;
    fixture.Set("BLACKLIGHT_VISUALS", "0");
    sg_SelectGraphicsStyle(1);
    fixture.Set("FLOOR_MIRROR", "0");
    fixture.Set("BLOOM_INTENSITY", "0.73");
    fixture.Set("GRID_SIZE", "31");
    fixture.Set("COCKPIT_FILE", "Local/custom-1.0.aacockpit.xml");
    std::string saved = fixture.Save();
    REQUIRE(saved.find("GRAPHICS_PREF_BLOOM_INTENSITY") != std::string::npos);

    tCallbackConfigLoad::Exec(tConfigLoadPhase::Begin);
    fixture.Set("GRAPHICS_STYLE", "-1");
    fixture.Set("BLACKLIGHT_VISUALS", "0");
    fixture.Set("BLOOM_INTENSITY", "0.5");
    fixture.Set("GRID_SIZE", "1");
    std::istringstream input(saved);
    tConfItemBase::LoadAll(input);
    tCallbackConfigLoad::Exec(tConfigLoadPhase::User);
    // Texture packs set these after user.cfg in the real startup sequence.
    fixture.Set("GRID_SIZE", "1");
    tCallbackConfigLoad::Exec(tConfigLoadPhase::Defaults);
    CHECK(fixture.Value("FLOOR_MIRROR") == "0");
    CHECK(std::stof(fixture.Value("BLOOM_INTENSITY")) == doctest::Approx(.73));
    CHECK(fixture.Value("GRID_SIZE") == "31");
    CHECK(fixture.Value("COCKPIT_FILE") == "Local/custom-1.0.aacockpit.xml");

    fixture.Set("GRAPHICS_STYLE_DEFAULT", "1");
    CHECK(std::stof(fixture.Value("BLOOM_INTENSITY")) == doctest::Approx(.73));
    fixture.Set("BLOOM_INTENSITY", "0.31");
    tCallbackConfigLoad::Exec(tConfigLoadPhase::Complete);
    CHECK(std::stof(fixture.Value("BLOOM_INTENSITY")) == doctest::Approx(.31));
    CHECK(fixture.Value("BLACKLIGHT_VISUALS") == "1");
}

TEST_CASE("Classic remains selected across old Legacy defaults and quality is unchanged")
{
    StyleFixture fixture;
    fixture.Set("MAX_FPS", "120");
    fixture.Set("ARMAGETRON_WINDOWSIZE_W", "3024");
    fixture.Set("ARMAGETRON_WINDOWSIZE_H", "1964");
#if SDL_VERSION_ATLEAST(2, 0, 0)
    fixture.Set("ANTIALIAS_SAMPLES", "4");
#endif
    sg_SelectGraphicsStyle(0);
    fixture.Set("GRAPHICS_STYLE_DEFAULT", "1");
    CHECK(fixture.Value("BLACKLIGHT_VISUALS") == "0");
    fixture.Set("BLACKLIGHT_VISUALS", "1");
    sg_ApplyGraphicsStyle();
    CHECK(fixture.Value("BLACKLIGHT_VISUALS") == "0");
    sg_SelectGraphicsStyle(1);
    CHECK(fixture.Value("BLACKLIGHT_VISUALS") == "1");
    CHECK(fixture.Value("MAX_FPS") == "120");
    CHECK(fixture.Value("ARMAGETRON_WINDOWSIZE_W") == "3024");
    CHECK(fixture.Value("ARMAGETRON_WINDOWSIZE_H") == "1964");
#if SDL_VERSION_ATLEAST(2, 0, 0)
    CHECK(fixture.Value("ANTIALIAS_SAMPLES") == "4");
#endif
}

TEST_CASE("A fallback style adopts preference persistence without resetting a later customization")
{
    StyleFixture fixture;
    tCallbackConfigLoad::Exec(tConfigLoadPhase::Begin);
    fixture.Set("GRAPHICS_STYLE", "-1");
    fixture.Set("BLACKLIGHT_VISUALS", "0");
    tCallbackConfigLoad::Exec(tConfigLoadPhase::User);
    tCallbackConfigLoad::Exec(tConfigLoadPhase::Defaults);
    fixture.Set("GRAPHICS_STYLE_DEFAULT", "1");
    fixture.Set("BLOOM_INTENSITY", "0.41");
    tCallbackConfigLoad::Exec(tConfigLoadPhase::Complete);
    CHECK(fixture.Value("GRAPHICS_STYLE") == "1");
    CHECK(fixture.Save().find("GRAPHICS_PREF_BLOOM_INTENSITY") != std::string::npos);
    fixture.Set("GRAPHICS_STYLE_DEFAULT", "1");
    CHECK(std::stof(fixture.Value("BLOOM_INTENSITY")) == doctest::Approx(.41));
}
#endif
