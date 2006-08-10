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

/*
 * HACK 
 * This is a very bad solution that hopefully will find a better design
 *
 * Basis for the "other" data that might be passed to an effect group 
 * ATM: only the value from the monitor is passed 
 * We use an auto_ptr so we can control if there is a value or not 
*/
typedef std::auto_ptr<REAL> miscDataPtr;



#endif
