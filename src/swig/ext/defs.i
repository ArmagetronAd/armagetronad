%{
#if defined(SWIGPYTHON)
#include "scripting/sScripting.h"
extern sCallable::ptr CreateCallablePython(PyObject * input); 
#endif
%}

typedef float REAL;

#if defined(SWIGPYTHON)
%typemap(in) sCallable::ptr {
    $1 = CreateCallablePython($input);
}
#endif

