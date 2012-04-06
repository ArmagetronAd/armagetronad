%{
#include "scripting/sScripting.h"
extern sCallable::ptr CreateCallablePython(PyObject * input);
#include "tString.h"
#include <string>"
%}

%import "std_string.i"

%typemap(in) sCallable::ptr {
    $1 = CreateCallablePython($input);
}

/* Return the results as a string object in the scripting language */
%typemap(argout) std::ostream&
{
    std::ostringstream *output = static_cast<std::ostringstream *> ($1);
    Py_DECREF($result);
    $result = PyString_FromString(output->str().c_str());
}
/* This typemap allows the scripting language to pass in a string
   which will be converted to an istringstream */
%typemap(in) std::istream& (std::stringstream stream)
{
    if (!PyString_Check($input))
    {
        PyErr_SetString(PyExc_TypeError, "not a string");
        return NULL;
    }
    std::string str(PyString_AsString($input));
    stream << str;
    $1 = &stream;
}


%typemap(in) tString {
    $1 = tString(PyString_AsString($input));
}
%typemap(in) tString & (tString str) {
    str=PyString_AsString($input);
    $1 = &str;
}
%typemap(out) tString {
    $result = PyString_FromString($1.c_str());
}
%typemap(out) tString * {
    $result = PyString_FromString($1->c_str());
}
%typemap(out) tString & {
    $result = PyString_FromString($1->c_str());
}


