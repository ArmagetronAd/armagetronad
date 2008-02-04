/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2000  Manuel Moos (manuel@moosnet.de)

**************************************************************************

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
  
***************************************************************************

*/

#ifndef ArmageTron_CONFIGURATION_H
#define ArmageTron_CONFIGURATION_H

#include "tList.h"
#include "tString.h"
#include "tLinkedList.h"
#include "tLocale.h"
#include "tConsole.h"
#include "tLocale.h"
#include <iostream>
#include <ctype.h>
#include <string>
#include <deque>
#include <map>

class tConfItemBase
{
    friend class tCheckedPTRBase;

    int id;
protected:
    const tString title;
    const tOutput help;
    bool changed;
    typedef std::map< tString, tConfItemBase * > tConfItemMap;
    static tConfItemMap & ConfItemMap();

public:
    typedef void callbackFunc(void);
private:
    callbackFunc *callback;
protected:
    void ExecuteCallback() {if (callback != 0) (*callback)(); }

    // static tConfItemBase* s_ConfItemAnchor;
    //static tConfItemBase* Anchor(){return dynamic_cast<tConfItemBase *>(s_ConfItemAnchor);}
public:
    static bool printChange; //!< if set, setting changes are printed to the console and, if printErrors is set as well, suggestions of typo fixes are given.
    static bool printErrors; //!< if set, unknown settings are pointed out.

    tConfItemBase(const char *title, const tOutput& help, callbackFunc *cb=0);
    tConfItemBase(const char *title, callbackFunc *cb=0);
    virtual ~tConfItemBase();

    tString const & GetTitle() const { return title; }

    static int EatWhitespace(std::istream &s); // eat whitespace from stream; return: first non-whitespace char

    static void SaveAll(std::ostream &s);
    static void LoadAll(std::istream &s);  //! loads configuration from file
    static void LoadLine(std::istream &s); //! loads one configuration line
    static bool LoadPlayback( bool print = false ); //! loads configuration from playback
    static void DocAll(std::ostream &s);
    static std::deque<tString> GetCommands(void);
    static tConfItemBase *FindConfigItem(tString const &name);

    virtual void ReadVal(std::istream &s)=0;
    virtual void WriteVal(std::ostream &s)=0;

    virtual void WasChanged(){} // what to do if a read changed the thing

    virtual bool Writable(){return true;}

    virtual bool Save(){return true;}
};

// Arg! Msvc++ could not handle bool IO. Seems to be fine now.
#ifdef _MSC_VER_XXX
inline std::istream & operator >> (std::istream &s,bool &b){
    int x;
    s >> x;
    b=(x!=0);
    return s;
}

inline std::ostream & operator << (std::ostream &s,bool b){
    if (b)
        return s << 1;
    else
        return s << 0;
}
#endif

//! type modifying class mapping types in memory to types to stream
template< class T > struct tTypeToConfig
{
    typedef T TOSTREAM;             //!< type to put into the stream
    typedef int DUMMYREQUIRED;      //!< change this type to "int *" to indicate the conversion is required
};

//! macro declaring that type TYPE should be converted to type STREAM before
// recording (and back after playback) by specializing the tTypeToStream class template
#define tCONFIG_AS( TYPE, STREAM ) \
template<> struct tTypeToConfig< TYPE > \
{ \
    typedef STREAM TOSTREAM; \
  typedef int * DUMMYREQUIRED; \
} \

//! macro for configuration enums: convert them to int.
#define tCONFIG_ENUM( TYPE ) tCONFIG_AS( TYPE, int )

template<class T> class tConfItem:virtual public tConfItemBase{
protected:
    T    *target;

    tConfItem(T &t):tConfItemBase(""),target(&t){};
public:
    tConfItem(const char *title,const tOutput& help,T& t, callbackFunc *cb=0)
            :tConfItemBase(title,help,cb),target(&t){}

    tConfItem(const char *title,T& t, callbackFunc *cb=0)
            :tConfItemBase(title,cb),target(&t){}

    virtual ~tConfItem(){}

    typedef typename tTypeToConfig< T >::DUMMYREQUIRED DUMMYREQUIRED;

    // read without conversion
    static void DoRead(std::istream &s, T & value, int )
    {
        s >> value;
    }

    // read with conversion
    static void DoRead(std::istream &s, T & value, int * )
    {
        typename tTypeToConfig< T >::TOSTREAM dummy;
        s >> dummy;
        value = static_cast< T >( dummy );
    }

    // write without conversion
    static void DoWrite(std::ostream &s, T const & value, int )
    {
        s << value;
    }

    // write with conversion
    static void DoWrite(std::ostream &s, T const & value, int * )
    {
        s << static_cast< typename tTypeToConfig< T >::TOSTREAM >( value );
    }

    virtual void ReadVal(std::istream &s){
        // eat whitepsace
        int c= EatWhitespace(s);

        T dummy( *target );
        if (c!='\n' && s && !s.eof() && s.good()){
            DoRead( s, dummy, DUMMYREQUIRED() );
            if (!s.good() && !s.eof() )
            {
                tOutput o;
                o.SetTemplateParameter(1, title);
                o << "$config_error_read";
                con << o;
            }
            else
                if (dummy!=*target){
                    if (!Writable())
                    {
                        tOutput o;
                        o.SetTemplateParameter(1, title);
                        o << "nconfig_errror_protected";
                        con << "";
                    }
                    else{
                        if (printChange)
                        {
                            tOutput o;
                            o.SetTemplateParameter(1, title);
                            o.SetTemplateParameter(2, *target);
                            o.SetTemplateParameter(3, dummy);
                            o << "$config_value_changed";
                            con << o;
                        }

                        *target=dummy;
                        changed=true;
                        ExecuteCallback();
                    }
                }
        }
        else
        {
            tOutput o;
            o.SetTemplateParameter(1, title);
            o.SetTemplateParameter(2, *target);
            o << "$config_message_info";
            con << o;
        }

        // read the rest of the line
        c=' ';
        while(c!='\n' && s.good() && !s.eof()) c=s.get();
    }

    virtual void WriteVal(std::ostream &s){
        DoWrite( s, *target, DUMMYREQUIRED() );
    }
};

template<class T> class tSettingItem:public tConfItem<T>{
public:
    //  tSettingItem(const char *title,const tOutput& help,T& t)
    //    :tConfItemBase(title,help),tConfItem<T>(t){}

    tSettingItem(const char *title,T& t, void (*cb)(void)=0)
            :tConfItemBase(title),tConfItem<T>(title, t, cb){}

    virtual ~tSettingItem(){}

    virtual bool Save(){return false;}
};


class tConfItemLine:public tConfItem<tString>, virtual public tConfItemBase{
public:
    tConfItemLine(const char *title,const char *help,tString &s, callbackFunc *cb=0)
            :tConfItemBase(title,help),tConfItem<tString>(title,help,s,cb){};

    virtual ~tConfItemLine(){}

    tConfItemLine(const char *title, tString &s, callbackFunc *cb=0)
            :tConfItemBase(title,cb),tConfItem<tString>(title,s){};

    virtual void ReadVal(std::istream &s);
    virtual void WriteVal(std::ostream &s);
};

typedef void CONF_FUNC(std::istream &s);

class tConfItemFunc:public tConfItemBase{
    CONF_FUNC *f;
public:
    tConfItemFunc(const char *title, CONF_FUNC *func);
    virtual ~tConfItemFunc();

    virtual void ReadVal(std::istream &s);
    virtual void WriteVal(std::ostream &s);

    virtual bool Save();
};

// includes a single configuration file by name, searches in var and config directories
void st_Include( tString const & file, bool reportError = true );

void st_LoadConfig();
void st_SaveConfig();

extern bool st_FirstUse;

#endif

