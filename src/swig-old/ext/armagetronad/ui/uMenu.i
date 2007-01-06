%{
#include "uMenu.h"
#include "tString.h"
%}

%rename(Menu) uMenu;
%rename(MenuItem) uMenuItem;
%rename(MenuItemExit) uMenuItemExit;
%rename(MenuItemInt) uMenuItemInt;
%rename(MenuItemString) uMenuItemString;
%rename(MenuItemSubmenu) uMenuItemSubmenu;
%feature("director") uMenuItemAction;
%rename(MenuItemAction) uMenuItemAction; 

template<class T> class uMenuItemSelection;

%template(MenuItemSelectionInt) uMenuItemSelection<int>;
%template(MenuItemSelectionString) uMenuItemSelection<tString>;
%template(MenuItemSelectionBool) uMenuItemSelection<bool>;
%template(MenuItemSelectionFloat) uMenuItemSelection<float>;

%rename(MenuItemToggle) uMenuItemToggle; // class uMenuItemToggle : public uMenuItemSelection<bool>



// TODO
%ignore uMenuItemStringWithHistory;
// template<class T> class uSelectItem
%ignore uSelectItem;
// template<class T> class uSelectEntry
%ignore uSelectEntry;


// class uMenuItemFunction;
%ignore uMenuItemFunction;
// class uMenuItemFunctionInt
%ignore uMenuItemFunctionInt;
// class uMenuItemFileSelection: public uMenuItemSelection<tString>
%ignore uMenuItemFileSelection;
// class uCallbackMenuEnter: public tCallback
%ignore uCallbackMenuEnter;
// class uCallbackMenuLeave: public tCallback
%ignore uCallbackMenuLeave;
// class uCallbackMenuBackground: public tCallback
%ignore uCallbackMenuBackground;

%include "uMenu.h"
