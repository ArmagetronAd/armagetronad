%{
#include "tColor.h"
%}

%rename(Color) tColor;
%rename(r) r_;
%rename(g) g_;
%rename(b) b_;
%rename(a) a_;
%ignore operator<<;
%ignore operator==;
%include "tColor.h"
