%{
#include "tConfiguration.h"
#include "tString.h"
#include <iostream>
%}

%import "tLocale.i"
%import "std_deque.i"

%feature("director") tConfItemBase;
%rename(ConfItemBase) tConfItemBase;
class tConfItemBase
{
public:
    typedef void callbackFunc(void);
%rename(print_change) printChange;
    static bool printChange;
%rename(print_errors) printErrors;
    static bool printErrors;
    // tConfItemBase(const char *title, const tOutput& help, callbackFunc *cb=0);
    tConfItemBase(const char *title, callbackFunc *cb=0);
    virtual ~tConfItemBase();
    tString const & GetTitle() const;
    static int EatWhitespace(std::istream &s);
    static void SaveAll(std::ostream &s);

    // static void LoadAll(std::istream &s);
%extend {
	static void LoadAll(char *c)
	{
		std::string str(c);
		std::stringstream s(str);
		tConfItemBase::LoadAll(s);
	}
}

	// static void LoadLine(std::istream &s);
%extend {
	static void LoadLine(char *c)
	{
		std::string str(c);
		std::stringstream s(str);
		tConfItemBase::LoadLine(s);
	}
}
    static bool LoadPlayback( bool print = false );
    static void DocAll(std::ostream &s);
    static std::deque<tString> GetCommands(void);
    static tConfItemBase *FindConfigItem(tString const &name);
    virtual void ReadVal(std::istream &s) = 0;
    virtual void WriteVal(std::ostream &s) = 0;
    virtual void WasChanged();
    virtual bool Writable();
    virtual bool Save();
};

%pointer_class(tString, StringPointer);
%pointer_class(bool, BoolPointer);
%pointer_class(int, IntPointer);
%pointer_class(float, FloatPointer);

template<class T> class tConfItem : virtual public tConfItemBase{
public:
	// tConfItem(const char *title,const tOutput& help,T& t, callbackFunc *cb=0);
	tConfItem(const char *title,T& t, callbackFunc *cb=0);
    virtual ~tConfItem(){}
    typedef typename tTypeToConfig< T >::DUMMYREQUIRED DUMMYREQUIRED;
    static void DoRead(std::istream &s, T & value, int );
    static void DoRead(std::istream &s, T & value, int * );
    static void DoWrite(std::ostream &s, T const & value, int );
    static void DoWrite(std::ostream &s, T const & value, int * );
    virtual void ReadVal(std::istream &s);
    virtual void WriteVal(std::ostream &s);
};
%template(TConfItemString) tConfItem<tString>;
%template(TConfItemBool) tConfItem<bool>;
%template(TConfItemInt) tConfItem<int>;
%template(TConfItemFloat) tConfItem<float>;

template<class T> class tSettingItem : public tConfItem<T>
{
public:
	tSettingItem(const char *title, T& t, void (*cb)(void) = 0);
	virtual ~tSettingItem();
    virtual bool Save();
};
%template(TSettingItemString) tSettingItem<tString>;
%template(TSettingItemBool) tSettingItem<bool>;
%template(TSettingItemInt) tSettingItem<int>;
%template(TSettingItemFloat) tSettingItem<float>;

%rename(TConfItemLine) tConfItemLine;
class tConfItemLine : public tConfItem<tString>, virtual public tConfItemBase
{
public:
    // tConfItemLine(const char *title,const char *help,tString &s, callbackFunc *cb=0);
    virtual ~tConfItemLine();
    tConfItemLine(const char *title, tString &s, callbackFunc *cb=0);
    virtual void ReadVal(std::istream &s);
    virtual void WriteVal(std::ostream &s);
};
