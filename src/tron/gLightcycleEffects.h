// Local optical effects; no simulation state. GPL version 2 or later.
#ifndef ARMAGETRON_LIGHTCYCLE_EFFECTS_H
#define ARMAGETRON_LIGHTCYCLE_EFFECTS_H
#include "aa_config.h"
#ifndef DEDICATED
namespace gLightcycleEffects
{
void Render(float r, float g, float b, float seconds, float speed, bool braking, float bank, float birthAge);
void GroundLight(float r, float g, float b, float assembly);
}
#endif
#endif
