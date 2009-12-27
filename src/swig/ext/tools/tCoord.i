%{
#include "tCoord.h"
#include "eCoord.h"
%}

%ignore operator!=;
%ignore operator+=;
%ignore operator-=;
%ignore operator*=;
%ignore operator=;

%rename(Coord) tCoord;
%rename(estimated_range_of_mult) se_EstimatedRangeOfMult;
%rename(difference) st_GetDifference;
%rename(zero_coord) se_zeroCoord;

%include "tCoord.h"
%include "eCoord.h"

