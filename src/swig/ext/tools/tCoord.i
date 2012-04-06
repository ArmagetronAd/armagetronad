%{
#include "eCoord.h"
%}

%ignore tCoord::operator!=;
%ignore tCoord::operator+=;
%ignore tCoord::operator-=;
%ignore tCoord::operator*=;
%ignore tCoord::operator=;
%ignore tCoord::operator<<;
%ignore tCoord::operator>>;

%rename(Coord) tCoord;
%rename(estimated_range_of_mult) se_EstimatedRangeOfMult;
%rename(difference) st_GetDifference;
%rename(zero_coord) se_zeroCoord;

%include "eCoord.h"

