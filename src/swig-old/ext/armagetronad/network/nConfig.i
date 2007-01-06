%{
#include "nConfig.h"
#include "tString.h"
#include "tLocale.h"
typedef void callbackFunc(void);
%}

%import "tConfiguration.i"
%import "tLocale.i"

%ignore sn_compare;

%rename(IConfItemWatcher) nIConfItemWatcher;
%rename(ConfItemVersionWatcher) nConfItemVersionWatcher;

%include "nConfig.h"

%template(NConfItemInt) nConfItem<int>;
%template(NConfItemBool) nConfItem<bool>;
%template(NConfItemFloat) nConfItem<float>;
%template(NConfItemString) nConfItem<tString>;

%template(NSettingItemInt) nSettingItem<int>;
%template(NSettingItemBool) nSettingItem<bool>;
%template(NSettingItemFloat) nSettingItem<float>;
%template(NSettingItemString) nSettingItem<tString>;

%template(SettingItemWatchedInt) nSettingItemWatched<int>;
%template(SettingItemWatchedBool) nSettingItemWatched<bool>;
%template(SettingItemWatchedFloat) nSettingItemWatched<float>;
%template(SettingItemWatchedString) nSettingItemWatched<tString>;

// nConfItemLine


