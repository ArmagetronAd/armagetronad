%{
#include "tArray.h"	
%}
%ignore operator=;
%include "tArray.h"
%template(ArrayString) tArray<tString>;
%template(ArrayBool) tArray<bool>;
%template(ArrayInt) tArray<int>;
%template(ArrayFloat) tArray<float>;