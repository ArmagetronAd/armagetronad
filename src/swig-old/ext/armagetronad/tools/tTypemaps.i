%{
#include "tString.h"
%}

typedef float REAL;

%typemap(out) tString {
	$result = rb_str_new2($1.c_str());
}

%typemap(in) tString {
	$1 = tString(StringValuePtr($input));
}
