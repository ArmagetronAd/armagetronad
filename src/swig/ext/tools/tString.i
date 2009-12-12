%{
#include "tString.h"
#include <string>"
%}

%import "std_string.i"

#if defined(SWIGPYTHON)
%typemap(out) tString {
    $result = PyString_FromString($1.c_str());
}
%typemap(in) tString {
    $1 = tString(PyString_AsString($input));
}
%typemap(out) tString * {
    $result = PyString_FromString($1->c_str());
}
%typemap(out) tString & {
    $result = PyString_FromString($1->c_str());
}

#elif defined(SWIGRUBY)
%typemap(out) tString {
    $result = rb_str_new2($1.c_str());
}
%typemap(in) tString {
    $1 = tString(StringValuePtr($input));
}
%typemap(out) tString * {
    $result = rb_str_new2($1->c_str());
}
%typemap(out) tString & {
    $result = rb_str_new2($1->c_str());
}

#else
  #warning no typemap defined

#endif


%rename(String) tString;
class tString : public std::string
{
};

%rename(ColoredString) tColoredString;
class tColoredString : public tString
{
%rename(remove_colors) RemoveColors;
    static tString RemoveColors( const char *c ); 
};

