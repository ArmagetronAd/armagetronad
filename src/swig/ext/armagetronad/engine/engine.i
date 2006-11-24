%module(directors="1") "ArmagetronAd::engine"

%{
#include "aa_config.h"
#define HAVE_LIBRUBY
%}

%include "tTypemaps.i"
%include "nTypemaps.i"
%include "rTypemaps.i"
%include "uTypemaps.i"
%include "eTypemaps.i"
%include "eAxis.i"
%include "eLagCompensation.i"
%include "ePath.i"
%include "ePlayer.i"
%include "eRectangle.i"
%include "eVoter.i"
