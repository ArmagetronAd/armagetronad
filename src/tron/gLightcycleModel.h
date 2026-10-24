// Model space: forward +X, axles along Y, floor at Z=0. GPL version 2 or later.
#ifndef ARMAGETRON_LIGHTCYCLE_MODEL_H
#define ARMAGETRON_LIGHTCYCLE_MODEL_H
#include "aa_config.h"

namespace gLightcycleModel
{
constexpr float RearAxle = -1.45f;
constexpr float FrontAxle = 1.55f;
constexpr float WheelRadius = .72f;
#ifndef DEDICATED
bool AuthoredAvailable();
float RearEmitterX();
float ModelWheelRadius(int wheel);
float RearProfileX(float height);
void Render(float r, float g, float b, float rearAngle, float frontAngle);
#endif
}
#endif
