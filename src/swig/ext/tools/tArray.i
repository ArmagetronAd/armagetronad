%{
#include "tArray.h"	
#include "tList.h"	
%}
%ignore tArray::operator=;
%ignore tArray::operator[];
%include "tArray.h"
%template(ArrayString) tArray<tString>;
%template(ArrayBool) tArray<bool>;
%template(ArrayInt) tArray<int>;
%template(ArrayFloat) tArray<float>;

%ignore tList::Add( T* t );
%ignore tList::Remove( T* t );

%include "tList.h"

