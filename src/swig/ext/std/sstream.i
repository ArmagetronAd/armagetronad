%{
#include <sstream>
%}

/* Enabling default typemaps for std::strings */
%include std_string.i


/* Get rid of the ostream parameter by setting numinputs to 0 */   
%typemap(in,numinputs=0) std::ostream& (std::ostringstream stream)
{
    $1 = &stream;
}



#if defined(SWIGRUBY)
/* Return the results as a string object in the scripting language */
%typemap(argout) std::ostream& 
{
    std::string str = stream$argnum.str();	
    $result = SWIG_FromCharPtrAndSize(str.data(), str.size());
}
/* This typemap allows the scripting language to pass in a string
   which will be converted to an istringstream */
%typemap(in) std::istream& (char *buf = 0, size_t size = 0, int alloc = 0, std::stringstream stream)
{
    /* Convert from scripting language string to char* */
    if (SWIG_AsCharPtrAndSize($input, &buf, &size, &alloc) != SWIG_OK)
    {
        %argument_fail(SWIG_TypeError, "(TYPEMAP, SIZE)", $symname, $argnum);
    }
    /* Write data to the stream.  Note that the returned size includes
     the last character, which is a null character.  We do not want
     to write that to the stream so subtract one from its size. */
    stream.write(buf, size - 1);
    $1 = &stream;
}
/* Free allocated buffer created in the (in) typemap */
%typemap(freearg) std::istream&
{
  if (alloc$argnum == SWIG_NEWOBJ) %delete_array(buf$argnum);
}



#elif defined(SWIGPYTHON)
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

#else
  #warning no "in" typemap defined
#endif


