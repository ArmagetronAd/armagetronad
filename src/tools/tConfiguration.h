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
#include "tException.h"
#include "tLocale.h"
#include "tConsole.h"
#include "tLocale.h"
#include <iostream>
#include <ctype.h>
#include <string>
#include <deque>
#include <map>

//! access levels for admin interfaces; lower numeric values are better
enum tAccessLevel
{
    tAccessLevel_Owner = 0,        // the server owner
    tAccessLevel_Admin = 1,        // one of his admins
    tAccessLevel_Moderator = 2,    // one of the moderators
    tAccessLevel_3 = 3,            // reserved
    tAccessLevel_4 = 4,            // reserved
    tAccessLevel_5 = 5,            // reserved
    tAccessLevel_6 = 6,            // reserved
    tAccessLevel_TeamLeader = 7,   // a team leader
    tAccessLevel_TeamMember = 8,   // a team member
    tAccessLevel_9 = 9,            // reserved
    tAccessLevel_10 = 10,          // reserved
    tAccessLevel_11 = 11,          // reserved
    tAccessLevel_Local      = 12,  // user with a local account
    tAccessLevel_13 = 13,          // reserved
    tAccessLevel_14 = 14,          // reserved
    tAccessLevel_Remote = 15,      // user with remote account
    tAccessLevel_DefaultAuthenticated = 15,     // default access level for authenticated users
    tAccessLevel_FallenFromGrace = 16,          // authenticated, but not liked
    tAccessLevel_Shunned = 17,          // authenticated, but disliked
    tAccessLevel_18 = 18,          // reserved
    tAccessLevel_Authenticated = 19,// any authenticated player
    tAccessLevel_Program = 20,     // a regular player
    tAccessLevel_21 = 21,          // reserved
    tAccessLevel_22 = 22,          // reserved
    tAccessLevel_23 = 23,          // reserved
    tAccessLevel_24 = 24,          // reserved
    tAccessLevel_25 = 25,          // reserved
    tAccessLevel_Invalid = 255,    // completely invalid level
    tAccessLevel_Default = 20
};

//! class to temporarily allow/forbid the use of casacl
class tCasaclPreventer
{
public:
    tCasaclPreventer( bool prevent = true );
    ~tCasaclPreventer();
private:
    bool previous_; //!< previous value of prevention flag
};

//! class managing the current access level
class tCurrentAccessLevel
{
    friend class tCasacl;
public:
    //! for the lifetime of this object, change the user's admit level to the passed one.
    tCurrentAccessLevel( tAccessLevel newLevel, bool allowElevation = false );

    //! does not change the access level on construction, but resets it on destruction
    tCurrentAccessLevel();
    ~tCurrentAccessLevel();

    //! returns the current access level
    static tAccessLevel GetAccessLevel();

    //! returns the name of an access level
    static tString GetName( tAccessLevel level );
private:
    tCurrentAccessLevel( tCurrentAccessLevel const & );
    tCurrentAccessLevel & operator = ( tCurrentAccessLevel const & );

    tAccessLevel lastLevel_; //!< used to restore the last admin level when the object goes out of scope
    static tAccessLevel currentLevel_; //!< the current access level
};

class tConfItemBase
{
    friend class tCheckedPTRBase;
    friend class tConfItemLevel;
    friend class tAccessLevelSetter;

    int id;
protected:
    const tString title;
    const tOutput help;
    bool changed;
    tAccessLevel requiredLevel; //!< access level required to change this setting
    tAccessLevel setLevel;      //!< access level of the user making the last change to this setting

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

    tString const & GetTitle() const {
        return title;
    }

    tAccessLevel GetRequiredLevel() const { return requiredLevel; }
    tAccessLevel GetSetLevel() const { return setLevel; }

    static int EatWhitespace(std::istream &s); // eat whitespace from stream; return: first non-whitespace char

    static void SaveAll(std::ostream &s);
    static void LoadAll(std::istream &s, bool record = false );  //! loads configuration from stream
    static void LoadAll(std::ifstream &s, bool record = false );  //! loads configuration from file
    static void LoadLine(std::istream &s); //! loads one configuration line
    static bool LoadPlayback( bool print = false ); //! loads configuration from playback
    static void DocAll(std::ostream &s);
    static std::deque<tString> GetCommands(void);
    static tConfItemBase *FindConfigItem(tString const &name);

    // helper functions for files (use these, they manage recording and playback properly)
    enum SearchPath
    {
        Config = 1,
        Var    = 2,
        All    = 3
    };

    static bool OpenFile( std::ifstream & s, tString const & filename, SearchPath path ); //! opens a file stream for configuration reading
    static void ReadFile( std::ifstream & s ); //! loads configuration from a file

    virtual void ReadVal(std::istream &s)=0;
    virtual void WriteVal(std::ostream &s)=0;

    virtual void WasChanged(){} // what to do if a read changed the thing

    virtual bool Writable(){
        return true;
    }

    virtual bool Save(){
        return true;
    }
};

//! just to do some work in static initializers, to modify default access levels:
class tAccessLevelSetter
{
public:
    //! modifies the access level of <item> to <level>
    tAccessLevelSetter( tConfItemBase & item, tAccessLevel level );
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

tCONFIG_ENUM( tAccessLevel );

//! exception to be thrown when the current script should be aborted
class tAbortLoading: public tException
{
public:
    tAbortLoading( char const * command );
private:
    tString command_; //!< the command responsible for the abort

    virtual tString DoGetName() const;
    virtual tString DoGetDescription() const;
};

template<class T> class tConfItem:virtual public tConfItemBase{
public:
    typedef bool (*ShouldChangeFuncT)(T const &newValue);
protected:
    T    *target;
    ShouldChangeFuncT shouldChangeFunc_;

    tConfItem(T &t):tConfItemBase(""),target(&t), shouldChangeFunc_(NULL) {};
public:
    tConfItem(const char *title,const tOutput& help,T& t, callbackFunc *cb)
            :tConfItemBase(title,help,cb),target(&t){}
    tConfItem(const char *title,const tOutput& help,T& t)
            :tConfItemBase(title,help),target(&t), shouldChangeFunc_(NULL) {}

   tConfItem(const char *title,T& t, callbackFunc *cb)
            :tConfItemBase(title,cb),target(&t){}
    tConfItem(const char *title,T& t)
            :tConfItemBase(title),target(&t), shouldChangeFunc_(NULL) {}
        
    tConfItem(const char*title, T& t, ShouldChangeFuncT changeFunc)
            :tConfItemBase(title),target(&t),shouldChangeFunc_(changeFunc) {}

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
                        o << "$nconfig_errror_protected";
                        con << "";
                    }
                    else{
                        if (!shouldChangeFunc_ || shouldChangeFunc_(dummy))
                        {
                            if (printChange)
                            {
                                tOutput o;
                                o.SetTemplateParameter(1, title);
                                o.SetTemplateParameter(2, *target);
                                o.SetTemplateParameter(3, dummy);
                                o << "$config_value_changed";
                                con << o;
                            }

                            *target = dummy;
                            changed = true;
                        }
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
        while (c!='\n' && s.good() && !s.eof()) c=s.get();
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
    
    tSettingItem(const char *title, T& t, typename tConfItem<T>::ShouldChangeFuncT changeFunc)
            :tConfItemBase(title), tConfItem<T>(title, t, changeFunc) {}

    virtual ~tSettingItem(){}

    virtual bool Save(){
        return false;
    }
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

