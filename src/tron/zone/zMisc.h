#ifndef ArmageTron_zMisc_H
#define ArmageTron_zMisc_H

#include "gCycle.h"

enum Triad {
    _false,
    _true,
    _ignore
};

struct Triggerer {
    gCycle * who;
    Triad    positive;
    Triad    marked;
};

/*
 *
 * Logic table:
 *          _false  | _true   | _ignore
 *------------------|---------|----------
 * _false  |   T    |    F    |    T
 * _true   |   F    |    T    |    T
 * _ignore |   T    |    T    |    T
 *
 */
inline bool validateTriad(Triad a, Triad b) {
    if (a==_ignore || b==_ignore || a==b)
    {
        return true;
    }
    return false;
}

#endif
