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
    static tConfItemBase *FindConfigItem(std::string const &name);
    
    %extend {
        static void LoadString(std::string str)
        {
            std::istringstream stream(str);
            tConfItemBase::LoadAll(stream);
        }
    }
    
    virtual void ReadVal(std::istream &s)=0;
    virtual void WriteVal(std::ostream &s)=0;
};

//template<class T>
//class tConfItem:virtual public tConfItemBase{
//public:
//    tConfItem(const char *title,T& t, callbackFunc *cb=0);    
//    virtual ~tConfItem(){}
//};
//%template(tConfItemValue) tConfItem<VALUE>;
