%module(directors="1") "ArmagetronAd::tools"

%{
#define HAVE_LIBRUBY
%}


%include "cpointer.i"
%include "std_deque.i"

%include "tTypemaps.i"
%include "tLocale.i"
%include "tArray.i"
%include "tCallback.i"
%include "tColor.i"
%include "tCommandLine.i"
%include "tConfiguration.i"
%include "tCoord.i"
%include "tDirectories.i"
%include "tError.i"
%include "tEventQueue.i"
%include "tLinkedList.i"
%include "tMemManager.i"
%include "tRandom.i"
%include "tResourceManager.i"
%include "tRing.i"
%include "tString.i"
%include "tSysTime.i"
