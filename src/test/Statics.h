#ifndef ArmageTron_STATICS_H
#define ArmageTron_STATICS_H

#include "defs.h"

#include <tuple>
#include <unistd.h>

inline bool InitStaticsCore()
{
    return true;
}

// call to make sure all static data is initialized
inline void InitStatics()
{
    CHECK(false); // we do not need this right now, maybe later

    static bool done = InitStaticsCore();
    std::ignore = done;
}

#endif