%{
#include "tLocale.h"
%}

%ignore tLanguage;
%ignore tOutputItemBase;
%ignore tLocale;

%ignore operator=;

%rename(Output) tOutput;

// class tLanguage;
// class tOutputItemBase;
// %import "tLinkedList.i"
// %template(ListItemLanguage) tListItem<tLanguage>;
// %template (ListItemOutputItemBase) tListItem<tOutputItemBase >;

%include "tLocale.h"
