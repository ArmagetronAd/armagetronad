%{
#include "tConfiguration.h"
%}

%feature("director") tConfItemBase;

class tConfItemBase
{
public:
    typedef void callbackFunc(void);
    tConfItemBase(const char *title, callbackFunc *cb=0);
    virtual ~tConfItemBase();
    static void LoadAll(std::istream &s);
    static void LoadLine(std::istream &s);
    virtual void ReadVal(std::istream &s)=0;
    virtual void WriteVal(std::ostream &s)=0;
};

