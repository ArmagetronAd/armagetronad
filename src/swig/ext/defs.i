typedef float REAL;

#if defined(SWIGPYTHON)
  %typemap(in) sScripting::proc_type "$1 = (PyObject*)$input;"
#else
  #warning no "in" typemap defined
#endif


