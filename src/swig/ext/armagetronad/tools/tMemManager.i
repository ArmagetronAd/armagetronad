%{
#include "tMemManager.h"
%}

%rename(MemMan) tMemMan;
class tMemMan{
public:
    static void *Alloc(size_t s);
    static void  Dispose(void *p);
    static void  DisposeButKeep(void *p);
    static void  Check();
    static void  Profile();
};