// Stadium presentation outside the physical rim. GPL version 2 or later.
#ifndef ARMAGETRON_ARENA_BACKDROP_H
#define ARMAGETRON_ARENA_BACKDROP_H
#include "aa_config.h"
#ifndef DEDICATED
class gArenaBackdrop
{
public:
    static void RenderSegment(float x1, float y1, float x2, float y2,
                              float outwardX, float outwardY, float r, float g, float b,
                              bool miterStart = false, bool miterEnd = false, float seconds = 0);
};
#endif
#endif
