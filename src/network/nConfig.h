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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
  
***************************************************************************

*/

#ifndef ArmageTron_NET_CONFIGURATION_H
#define ArmageTron_NET_CONFIGURATION_H

#include "tConfiguration.h"
#include "nNetwork.h"

template<class T> inline bool sn_compare(T&a, T&b)
{
    return (a!=b);
}

inline bool sn_compare(float&a, float &b)
{
    return (1000 * fabs(a-b) > fabs(a) + fabs(b));
}

class nConfItemBase;

//! interface for objects watching over the change of network configuration items
class nIConfItemWatcher
{
public:
    nIConfItemWatcher( nConfItemBase & item );            //!< constructor

    inline void Change( bool nonDefault );                //!< called on configuration item changes
    inline bool Writable() const;                         //!< returns whether the item should be writable
protected:
    virtual ~nIConfItemWatcher()=0;                       //!< destructor

    nConfItemBase & watched_;                             //!< the watched item
private:
    virtual void OnChange( bool nonDefault ) = 0;         //!< called on configuration item changes
    virtual bool DoWritable() const = 0;                  //!< returns whether the item should be writable

    // disable copying and default construction
    nIConfItemWatcher();            //!< constructor
    nIConfItemWatcher(nIConfItemWatcher const &);            //!< copy constructor
    nIConfItemWatcher& operator=(nIConfItemWatcher const &); //!< copy operator
};

//! base clas for network configuration items
class nConfItemBase:public virtual tConfItemBase
{
    friend class nIConfItemWatcher;
private:
    double lastChangeTime_;             //!< the time of the last change
    unsigned long lastChangeMessage_;   //!< the ID of the networking message responsible for the change
    nIConfItemWatcher * watcher_;       //!< the watcher that reacts on changes of this item
protected:
    nConfItemBase();
public:
    //  nConfItemBase(const char *title,const char *help);
    nConfItemBase(const char *title);
    virtual ~nConfItemBase();

    virtual void NetReadVal(nMessage &m)=0;
    virtual void NetWriteVal(nMessage &m)=0;

    static void s_GetConfigMessage(nMessage &m);

    virtual void WasChanged(bool nonDefault);

    virtual bool Writable();

    static void s_SendConfig(bool force=true, int peer=-1);
    void          SendConfig(bool force=true, int peer=-1);

    static void s_RevertToDefaults();       //!< revert all settings to defaults defined in the code
    inline void RevertToDefaults();         //!< revert this setting to its default

    static void s_SaveValues();             //!< saves all values for later restoring
    inline void SaveValue();                //!< saves value for later restoring

    static void s_RevertToSavedValues();    //!< reverts all settings to the saved values
    inline void RevertToSavedValue();       //!< reverts to the saved value
private:
    virtual void OnRevertToDefaults()=0;      //!< revert this setting to its default

    virtual void OnSaveValue()=0;             //!< saves value for later restoring
    virtual void OnRevertToSavedValue()=0;    //!< reverts to the saved value
};


template<class T> class nConfItem
            : public nConfItemBase,
            public tConfItem<T>,
            public virtual tConfItemBase
{
protected:
    nConfItem(T& t)
            :tConfItemBase(""), tConfItem<T>("", t),default_(t),saved_(t){}

public:
    nConfItem(const char *title,const char *help,T& t)
            :tConfItem<T>(t),
            tConfItemBase(title, help ),
            default_(t),
    saved_(t){}
    virtual ~nConfItem(){}


    virtual void NetReadVal(nMessage &m){
        T dummy;
        m >> dummy;
        if (sn_compare(dummy,*this->target)){
            if (printChange)
            {
                tOutput o;
                o.SetTemplateParameter(1, title);
                o.SetTemplateParameter(2, *this->target);
                o.SetTemplateParameter(3, dummy);
                o << "$nconfig_value_changed";
                con << con.ColorString(1,.3,.3) << o;
            }
            *this->target=dummy;
            changed=true;
        }
    }

    virtual void NetWriteVal(nMessage &m){
        m << *this->target;
    }

    void Set( const T& newval )
    {
        bool changed = ( newval != *this->target );
        *this->target = newval;

        if ( changed )
        {
            WasChanged();
        }
    }

    virtual void OnRevertToDefaults()      //!< revert this setting to its default
    {
        Set( default_ );
    }

    virtual void OnSaveValue()             //!< saves the current value
    {
        saved_ = *this->target;
    }

    virtual void OnRevertToSavedValue()    //!< revert this setting to the saved value
    {
        Set( saved_ );
    }
private:
    void WasChanged()
    {
        nConfItemBase::WasChanged( ! (*this->target == default_) );
    }

    T default_;     //!< default value
    T saved_;       //!< a saved value
};

template<class T> class nSettingItem:public nConfItem<T>{
private:
public:
    //  nSettingItem(const char *title,const char *help,T& t)
    //    :nConfItem<T>(t), tConfItemBase(title, help){}
    //  virtual ~nSettingItem(){}

    nSettingItem(const char *title,T& t)
            :tConfItemBase(title), nConfItem<T>(t){}
    virtual ~nSettingItem(){}

    virtual bool Save(){return false;}
};


class nConfItemLine:public nConfItem<tString>{
protected:
public:
    //  nConfItemLine(const char *title,const tOutput &help,tString &s)
    //    :nConfItem<tString>(s), tConfItemBase(title,help){};
    nConfItemLine(const char *title,tString &s);

    virtual ~nConfItemLine();

    virtual void ReadVal(std::istream &s);
};

//! how we react on a client with a version incompatible with a setting
enum nConfigItemBehavior
{
    Behavior_Nothing = 0, //!< do nothing, let client on
    Behavior_Revert = 1,  //!< revert setting to default value
    Behavior_Block = 2,   //!< don't let the client play at all
    Behavior_Default = 3  //!< do whatever someone else says
};

class nConfItemVersionWatcher;
tCONFIG_ENUM( nConfigItemBehavior );

//! configuration item watcher that shuts out clients that don't support a certain interface
class nConfItemVersionWatcher: public nIConfItemWatcher
{
public:
    //! enum describing the effect a nondefault setting has on clients that don't support it
    enum Group
    {
        Group_Breaking,  //!< client breaks horribly
        Group_Bumpy,     //!< client works, but physics are not simulated correctly
        Group_Annoying,  //!< small, annoying incompatibility
        Group_Cheating,  //!< the behavior of the old client is considered cheating
        Group_Visual,    //!< some displayed value is wrong, but everything behaves right

        Group_Max
    };

    typedef nConfigItemBehavior Behavior;

    nConfItemVersionWatcher( nConfItemBase & item, Group group, int min, int max = -1 );          //!< constructor
    virtual ~nConfItemVersionWatcher();                      //!< destructor

    static void AdaptVersion( nVersion & version );          //!< adapt version so it is compatible with all settings
    static void OnVersionChange( nVersion const & version ); //!< revert relevant settings that don't fit with the version to their defaults

    Behavior GetBehavior( void ) const;	                     //!< Gets behavior if this setting is on non-default and a client that does not support it connects
    nConfItemVersionWatcher const & GetBehavior( Behavior & behavior ) const;	//!< Gets behavior if this setting is on non-default and a client that does not support it connects

    void FillTemplateParameters( tOutput & o ) const;        //!< fills output with template parameters about the setting
private:
    virtual void OnChange( bool nonDefault );                //!< called on configuration item changes
    virtual bool DoWritable() const;                         //!< returns whether the item should be writable

    nVersion version_;                                       //!< the versions that support this
    bool nonDefault_;                                        //!< flag memorizing whether the setting is at default by the user
    bool reverted_;                                          //!< flag memorizing whether the setting has been reverted to the default temporarily

    Group group_;                                            //!< class of incompatibility
    Behavior overrideGroupBehavior_;                         //!< if set, the global behavior for the class gets ignored
    tSettingItem< Behavior > overrideGroupBehaviorConf_;     //!< setting item for override
};

//! convenience helper class: setting item and version watcher combined
template<class T> class nSettingItemWatched
{
public:
    typedef nConfItemVersionWatcher::Group Group;

    // constructor
    nSettingItemWatched( char const * title, T & value, Group group, int min, int max = -1 )
            : setting_( title, value )
            , watcher_( setting_, group, min, max )
    {
    }

    void Set( T const & value )
    {
        this->setting_.Set( value );
    }

    nSettingItem< T > & GetSetting(){ return setting_; }
private:
    nSettingItem< T > setting_;
    nConfItemVersionWatcher watcher_;
};

// *******************************************************************************************
// *
// *	Change
// *
// *******************************************************************************************
//!
//!		@param	nonDefault
//!
// *******************************************************************************************

void nIConfItemWatcher::Change( bool nonDefault )
{
    this->OnChange( nonDefault );
}

// *******************************************************************************************
// *
// *	RevertToDefaults
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

void nConfItemBase::RevertToDefaults( void )
{
    // inform watcher
    if (this->watcher_ )
        this->watcher_->Change( false );

    // reset last change information
    lastChangeMessage_ = 0;
    lastChangeTime_ = -1000000;

    this->OnRevertToDefaults();
}

// *******************************************************************************************
// *
// *	SaveValue
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

void nConfItemBase::SaveValue( void )
{
    this->OnSaveValue();
}

// *******************************************************************************************
// *
// *	RevertToSavedValue
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

void nConfItemBase::RevertToSavedValue( void )
{
    this->OnRevertToSavedValue();
}

// *******************************************************************************************
// *
// *	Writable
// *
// *******************************************************************************************
//!
//!		@return		true if the watched item is changable
//!
// *******************************************************************************************

bool nIConfItemWatcher::Writable( void ) const
{
    return this->DoWritable();
}

tOutput sn_GetClientVersionString(int version);

#endif

