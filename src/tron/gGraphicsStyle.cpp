// Optional appearance presets. GPL version 2 or later.
#include "gGraphicsStyle.h"
#include "gStuff.h"
#include "tConfiguration.h"
#include "rDisplayList.h"

#include <sstream>
#include <string>

bool sg_blacklightVisuals = false;
static tConfItem<bool> sg_visualsConf("BLACKLIGHT_VISUALS", sg_blacklightVisuals);

namespace
{
int style = -1;
bool loading = false;
bool defaultsLoaded = false;

bool ExplicitStyle() { return style == 0 || style == 1; }

void StyleChanged()
{
    if (!loading || defaultsLoaded)
        sg_SelectGraphicsStyle(style);
}
tConfItem<int> styleConf("GRAPHICS_STYLE", style, &StyleChanged);

#ifndef DEDICATED
// Resource-pack settings normally are not saved. Persist only the appearance
// values owned by these presets, using the existing configuration serialization.
class AppearancePreference : public tConfItemBase
{
public:
    AppearancePreference(char const* setting)
        : tConfItemBase((std::string("GRAPHICS_PREF_") + setting).c_str()), setting_(setting)
    {
    }

    bool Save() override { return ExplicitStyle() && Target(); }

    void ReadVal(std::istream& in) override
    {
        std::getline(in, saved_);
        present_ = true;
    }

    void WriteVal(std::ostream& out) override
    {
        if (auto* target = Target())
            target->WriteVal(out);
    }

    void Clear()
    {
        saved_.clear();
        present_ = false;
    }

    void CaptureUserPreference()
    {
        auto* target = Target();
        if (!target || !target->Save())
            return;
        // Ordinary user.cfg entries remain authoritative when edited by hand.
        std::ostringstream value;
        target->WriteVal(value);
        saved_ = value.str();
        present_ = true;
    }

    void Restore() const
    {
        if (present_)
            if (auto* target = Target())
            {
                std::istringstream value(saved_);
                target->ReadVal(value);
            }
    }

private:
    tConfItemBase* Target() const
    {
        return tConfItemBase::FindConfigItem(tString(setting_));
    }
    char const* setting_;
    std::string saved_;
    bool present_ = false;
};

AppearancePreference preferences[] = {
    {"LIGHTCYCLE_SCALE"}, {"INDIVIDUAL_PLAYER_COLORS"}, {"COCKPIT_FILE"}, {"FLOOR_DETAIL"}, {"FLOOR_RED"}, {"FLOOR_GREEN"}, {"FLOOR_BLUE"}, {"GRID_SIZE"}, {"FLOOR_MIRROR"}, {"FLOOR_MIRROR_INT"}, {"FLOOR_REFLECTION_SOFTNESS"}, {"RIM_WALL_RED"}, {"RIM_WALL_GREEN"}, {"RIM_WALL_BLUE"}, {"UPPER_SKY"}, {"LOWER_SKY"}, {"SKY_WOBBLE"}, {"BLOOM"}, {"BLOOM_INTENSITY"}, {"BLOOM_THRESHOLD"}, {"BLOOM_RADIUS"}, {"BLOOM_HALO"}, {"BLOOM_VIGNETTE"}};

void LoadDefaults(int requested)
{
    tCurrentAccessLevel owner(tAccessLevel_Owner, true);
    tNoisinessSetter quiet(false, true);
    st_Include(tString(requested == 1 ? "graphics_legacy.cfg" : "graphics_classic.cfg"));
}

void ConfigLoaded(tConfigLoadPhase phase)
{
    switch (phase)
    {
    case tConfigLoadPhase::Begin:
        loading = true;
        defaultsLoaded = false;
        for (auto& preference : preferences)
            preference.Clear();
        break;
    case tConfigLoadPhase::User:
        if (ExplicitStyle())
            for (auto& preference : preferences)
                preference.CaptureUserPreference();
        break;
    case tConfigLoadPhase::Defaults:
        if (ExplicitStyle())
        {
            LoadDefaults(style);
            for (auto const& preference : preferences)
                preference.Restore();
            sg_ApplyGraphicsStyle();
        }
        defaultsLoaded = true;
        break;
    case tConfigLoadPhase::Complete:
        sg_ApplyGraphicsStyle();
        loading = false;
        break;
    }
}
tCallbackConfigLoad configLoaded(&ConfigLoaded);

void DefaultStyle(std::istream& input)
{
    int requested = -1;
    if (input >> requested && !ExplicitStyle() && (requested == 0 || requested == 1))
    {
        LoadDefaults(requested);
        style = requested;
    }
}
tConfItemFunc defaultStyle("GRAPHICS_STYLE_DEFAULT", &DefaultStyle);
#endif
} // namespace

void sg_ApplyGraphicsStyle()
{
#ifndef DEDICATED
    if (!ExplicitStyle())
        return;
    bool const changed = sg_blacklightVisuals != (style == 1) || sg_moviepackUse;
    sg_blacklightVisuals = style == 1;
    sg_moviepackUse = false;
    if (changed)
        rDisplayList::ClearAll();
#endif
}

void sg_SelectGraphicsStyle(int const& requested)
{
    if (requested != 0 && requested != 1)
        return;
#ifndef DEDICATED
    bool const change = sg_blacklightVisuals != (requested == 1) || sg_MoviePack();
    style = requested;
    if (change)
        LoadDefaults(requested);
    sg_ApplyGraphicsStyle();
#else
    style = requested;
#endif
}
