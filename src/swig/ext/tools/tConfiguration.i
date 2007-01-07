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

template<class T> class tConfItem : virtual public tConfItemBase{
public:
	// tConfItem(const char *title,const tOutput& help,T& t, callbackFunc *cb=0);
	tConfItem(const char *title,T& t, callbackFunc *cb=0);
    virtual ~tConfItem(){}
};
%template(TConfItemValue) tConfItem<VALUE>;

template<class T> class tSettingItem : public tConfItem<T>
{
public:
	tSettingItem(const char *title, T& t, void (*cb)(void) = 0);
	virtual ~tSettingItem();
    virtual bool Save();
};
%template(TSettingItemValue) tSettingItem<VALUE>;

%template(TConfItemString) tConfItem<tString>;
class tConfItemLine : public tConfItem<tString>, virtual public tConfItemBase
{
public:
    // tConfItemLine(const char *title,const char *help,tString &s, callbackFunc *cb=0);
    virtual ~tConfItemLine();
    tConfItemLine(const char *title, tString &s, callbackFunc *cb=0);
};
