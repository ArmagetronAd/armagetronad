%{
#include <sstream>
%}

/* Enabling default typemaps for std::strings */
%include std_string.i
%include <stl.i>

/* Get rid of the ostream parameter by setting numinputs to 0 */   
%typemap(in,numinputs=0) std::ostream& (std::ostringstream stream)
{
    $1 = &stream;
}


