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

#include "nConfig.h"
#include "nNetObject.h"
#include "nProtoBuf.h"
#include "tConsole.h"
#include "tSysTime.h"
#include <set>
#include <string.h>

#include "nConfig.pb.h"

nConfItemBase::nConfItemBase()
        :tConfItemBase(""), lastChangeTime_(-10000), lastChangeMessage_(0), watcher_(0){}

//nConfItemBase::nConfItemBase(const char *title,const char *help)
//  :tConfItemBase(title, help){}

nConfItemBase::nConfItemBase(const char *title)
        :tConfItemBase(title), lastChangeTime_(-10000), lastChangeMessage_(0), watcher_(0){}

nConfItemBase::~nConfItemBase(){}

void nConfItemBase::s_GetConfigStream(nStreamMessage &m){
    if (sn_GetNetState()==nSERVER){
        nReadError(); // never accept config messages from the clients
    }
    else{
        tString name;
        m >> name;

        //con << "got conf message for " << name << "\n";

        tConfItemMap & confmap = ConfItemMap();
        tConfItemMap::iterator iter = confmap.find( name );
        if ( iter != confmap.end() )
        {
            tConfItemBase * item = (*iter).second;
            nConfItemBase *netitem = dynamic_cast<nConfItemBase*> (item);
            if (netitem)
            {
                // check if message was new
                if ( tSysTimeFloat() > netitem->lastChangeTime_ + 100 || sn_Update( netitem->lastChangeMessage_, m.MessageIDBig() ) )
                {
                    netitem->lastChangeMessage_ = m.MessageIDBig();
                    netitem->lastChangeTime_ = tSysTimeFloat();
                    netitem->NetReadVal(m);
                }
                else
                {
                    static bool warn = true;
                    if ( warn )
                        con << tOutput( "$nconfig_error_ignoreold", name );
                    warn = false;
                }
            }
            else
            {
                static bool warn = true;
                if ( warn )
                    con << tOutput( "$nconfig_error_nonet", name );
                warn = false;
            }
        }
        else
        {
            static bool warn = true;
            if ( warn )
                con << tOutput( "$nconfig_error_unknown", name );
            warn = false;
        }
    }
}


static nStreamDescriptor transferConfig(60,nConfItemBase::s_GetConfigStream,
                                        "transfer config");

void nConfItemBase::s_GetConfigProtoBuf( Network::Config const & protoBuf, nSenderInfo const & sender ){
    if (sn_GetNetState()==nSERVER){
        nReadError(); // never accept config messages from the clients
    }
    else{
        tString name = protoBuf.name();

        //con << "got conf message for " << name << "\n";

        tConfItemMap & confmap = ConfItemMap();
        tConfItemMap::iterator iter = confmap.find( name );
        if ( iter != confmap.end() )
        {
            tConfItemBase * item = (*iter).second;
            nConfItemBase *netitem = dynamic_cast<nConfItemBase*> (item);
            if (netitem)
            {
                // check if message was new
                if ( tSysTimeFloat() > netitem->lastChangeTime_ + 100 || sn_Update( netitem->lastChangeMessage_, sender.MessageIDBig() ) )
                {
                    netitem->lastChangeMessage_ = sender.MessageIDBig();
                    netitem->lastChangeTime_ = tSysTimeFloat();
                    netitem->NetReadVal( protoBuf );
                }
                else
                {
                    static bool warn = true;
                    if ( warn )
                        con << tOutput( "$nconfig_error_ignoreold", name );
                    warn = false;
                }
            }
            else
            {
                static bool warn = true;
                if ( warn )
                    con << tOutput( "$nconfig_error_nonet", name );
                warn = false;
            }
        }
        else
        {
            static bool warn = true;
            if ( warn )
                con << tOutput( "$nconfig_error_unknown", name );
            warn = false;
        }
    }
}


static nProtoBufDescriptor< Network::Config >
sn_ConfigDescriptor( 60, nConfItemBase::s_GetConfigProtoBuf );

// helper functions for templated subclasses
int     nConfItemBase::NetReadHelper( Network::Config const & protoBuf, int )
{
    return protoBuf.int_value();
}

float   nConfItemBase::NetReadHelper( Network::Config const & protoBuf, float )
{
    return protoBuf.float_value();
}

tString nConfItemBase::NetReadHelper( Network::Config const & protoBuf, tString const & )
{
    return protoBuf.string_value();
}

void nConfItemBase::NetWriteHelper( Network::Config & protoBuf, int value )
{
    protoBuf.set_int_value( value );
}

void nConfItemBase::NetWriteHelper( Network::Config & protoBuf, float value )
{
    protoBuf.set_float_value( value );
}

void nConfItemBase::NetWriteHelper( Network::Config & protoBuf, tString const & value )
{
    protoBuf.set_string_value( value );
}

void nConfItemBase::s_SendConfig(bool force, int peer){
    if(sn_GetNetState()==nSERVER){
        tConfItemMap & confmap = ConfItemMap();
        for(tConfItemMap::iterator iter = confmap.begin(); iter != confmap.end() ; ++iter)
        {
            tConfItemBase * item = (*iter).second;

            nConfItemBase *netitem = dynamic_cast<nConfItemBase*> (item);
            if (netitem)
                netitem->SendConfig(force, peer);
        }
    }
}

static void sn_SendConfig( nStreamMessage * stream, nProtoBufMessageBase * protoBuf, int peer )
{
    if ( sn_protoBuf.Supported( peer ) )
    {
        protoBuf->Send( peer );
    }
    else
    {
        stream->Send( peer );
    }
}

void nConfItemBase::SendConfig(bool force, int peer){
    if ( (changed || force) && sn_GetNetState()==nSERVER)
    {
        //con << "sending conf message for " << tConfItems(i)->title << "\n";
        tJUST_CONTROLLED_PTR< nStreamMessage > m=tNEW(nStreamMessage)(transferConfig);
        *m << title;
        NetWriteVal(*m);

        tJUST_CONTROLLED_PTR< nProtoBufMessage< Network::Config > > m2=sn_ConfigDescriptor.CreateMessage();
        Network::Config & config = m2->AccessProtoBuf();
        config.set_name( title );
        NetWriteVal( config );

        if (peer==-1)
        {
            for( int i = MAXCLIENTS; i > 0; --i )
            {
                if ( sn_Connections[i].socket )
                {
                    sn_SendConfig( m, m2, i );
                }
            }
            changed = false;
        }
        else
        {
            sn_SendConfig( m, m2, peer );
        }
    }
}

void nConfItemBase::CheckChange( bool nonDefault ){
    // inform watcher
    if (this->watcher_ )
        this->watcher_->Change( nonDefault );

    SendConfig();
}

bool nConfItemBase::Writable()
{
    // network settings are read only on the client
    if ( sn_GetNetState() == nCLIENT )
        return false;

    // on the server, we need to check for a watcher...
    if ( !watcher_ )
        return true;

    // delegate
    return watcher_->Writable();
}

// *******************************************************************************************
// *
// *	s_RevertToDefaults
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

void nConfItemBase::s_RevertToDefaults( void )
{
    tConfItemMap & confmap = ConfItemMap();
    for(tConfItemMap::iterator iter = confmap.begin(); iter != confmap.end() ; ++iter)
    {
        tConfItemBase * item = (*iter).second;
        nConfItemBase *netitem = dynamic_cast<nConfItemBase*> (item);
        if (netitem)
        {
            netitem->RevertToDefaults();
        }
    }
}

// *******************************************************************************************
// *
// *	s_SaveValues
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

void nConfItemBase::s_SaveValues( void )
{
    tConfItemMap & confmap = ConfItemMap();
    for(tConfItemMap::iterator iter = confmap.begin(); iter != confmap.end() ; ++iter)
    {
        tConfItemBase * item = (*iter).second;
        nConfItemBase *netitem = dynamic_cast<nConfItemBase*> (item);
        if (netitem)
        {
            netitem->SaveValue();
        }
    }
}

// *******************************************************************************************
// *
// *	s_RevertToSavedValues
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

void nConfItemBase::s_RevertToSavedValues( void )
{
    tConfItemMap & confmap = ConfItemMap();
    for(tConfItemMap::iterator iter = confmap.begin(); iter != confmap.end() ; ++iter)
    {
        tConfItemBase * item = (*iter).second;
        nConfItemBase *netitem = dynamic_cast<nConfItemBase*> (item);
        if (netitem)
        {
            netitem->RevertToSavedValue();
        }
    }
}

nConfItemLine::nConfItemLine(const char *title,tString &s)
        :tConfItemBase(title),nConfItem<tString>(s){}

nConfItemLine::nConfItemLine(const char *title,tString &s, callbackFunc *cb)
        :tConfItemBase(title, cb),nConfItem<tString>(s){}

nConfItemLine::~nConfItemLine(){}

void nConfItemLine::ReadVal(std::istream & s)
{
    tString dummy;
    dummy.ReadLine(s,true);
    if(strcmp(dummy,*target)){
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

    *target=dummy;
}

// *******************************************************************************************
// *
// *	nIConfItemWatcher
// *
// *******************************************************************************************
//!
//!		@param	item	the item to watch
//!
// *******************************************************************************************

nIConfItemWatcher::nIConfItemWatcher( nConfItemBase & item )
        :watched_( item )
{
    item.watcher_ = this;
}

// *******************************************************************************************
// *
// *	~nIConfItemWatcher
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

nIConfItemWatcher::~nIConfItemWatcher( void )
{
    watched_.watcher_ = NULL;
}

// *******************************************************************************************
// *
// *	OnChange
// *
// *******************************************************************************************
//!
//!		@param	nonDefault	flag indicating whether the change was away from the default
//!
// *******************************************************************************************

void nIConfItemWatcher::OnChange( bool nonDefault )
{
}

// *******************************************************************************************
// *
// *	nIConfItemWatcher
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

//nIConfItemWatcher::nIConfItemWatcher( void )
//{
//}

// *******************************************************************************************
// *
// *	nIConfItemWatcher
// *
// *******************************************************************************************
//!
//!		@param	other object to copy from
//!
// *******************************************************************************************

//nIConfItemWatcher::nIConfItemWatcher( nIConfItemWatcher const & other )
//{
//}

// *******************************************************************************************
// *
// *	operator =
// *
// *******************************************************************************************
//!
//!     @param  other object to copy from
//!		@return reference to self
//!
// *******************************************************************************************

//nIConfItemWatcher & nIConfItemWatcher::operator =( nIConfItemWatcher const & other )
//{
//	return *this;
//}

typedef std::set< nConfItemVersionWatcher * > nStrongWatcherList;
static nStrongWatcherList * sn_watchers=0;
static int sn_refcount=0;

static nStrongWatcherList & sn_GetStrongWatchers()
{
    if (!sn_watchers)
    {
        sn_watchers = tNEW(nStrongWatcherList)();
    }

    return *sn_watchers;
}

void sn_StrongWatchersAddRef()
{
    sn_refcount++;
}

void sn_StrongWatchersRelease()
{
    if ( --sn_refcount <= 0 )
    {
        tDESTROY( sn_watchers );
    }
}

// *******************************************************************************************
// *
// *	nConfItemVersionWatcher
// *
// *******************************************************************************************
//!
//!		@param	item
//!		@param	feature
//!
// *******************************************************************************************

nConfItemVersionWatcher::nConfItemVersionWatcher( nConfItemBase & item, Group c, int min, int max )
        : nIConfItemWatcher( item )
        , version_( min, max > 0 ? max : 0x7FFFFFFF )
        , nonDefault_( false )
        , reverted_( false )
        , group_( c )
        , overrideGroupBehavior_( Behavior_Default )
        , overrideGroupBehaviorConf_( item.GetTitle() + "_OVERRIDE", overrideGroupBehavior_ )
{
    sn_StrongWatchersAddRef();
    sn_GetStrongWatchers().insert(this);
}

// *******************************************************************************************
// *
// *	~nConfItemVersionWatcher
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

nConfItemVersionWatcher::~nConfItemVersionWatcher( void )
{
    sn_GetStrongWatchers().erase(this);
    sn_StrongWatchersRelease();
}

// *******************************************************************************************
// *
// *	OnChange
// *
// *******************************************************************************************
//!
//!     @param  nonDefault  flag indicating whether the change was away from the default
//!
// *******************************************************************************************

void nConfItemVersionWatcher::OnChange( bool nonDefault )
{
    bool changed = ( nonDefault != nonDefault_ );

    nonDefault_ = nonDefault;
    if ( changed )
    {
        sn_UpdateCurrentVersion();
    }
}

static char const * sn_groupName[ nConfItemVersionWatcher::Group_Max ] =
    {
        "Breaking",
        "Bumpy",
        "Annoying",
        "Cheating",
        "Visual"
    };

// uncomment on Trunk as soon as it diverges from 0.4
// #define RESERVE_FOR_0_4

// mapping network version to program version
static char const * sn_versionString[] =
    {
        "0.2.0",   // 0
        "0.2.0",   // 1
        "0.2.5.0", // 2
        "0.2.6.0", // 3
        "0.2.7.1", // 4
        "0.2.8_beta1", // 5
        "0.2.8_beta1", // 6
        "0.2.8_beta2", // 7
        "0.2.8_beta3", // 8
        "0.2.8_beta4",   // 9
        "0.2.8.0_rc1",     // 10
        "0.2.8.0",       // 11
        "0.2.8_alpha20060414", // 12
        "0.2.8.2", // 13
        "0.2.8.3_alpha", // 14
        "0.2.8.3_alpha_auth", // 15
        "0.2.8.3.X", // 16, was: 0.2.8.3_beta2
        "0.2.9_alpha", // 17
        "0.2.9_alpha2", // 18
        "0.2.9_alpha3", // 19
        "0.3.1", // 20
        "0.3.1_pb", // 21
        "0.4_auto_team", // 22
        // move #ifdef downwards to define new protocol versions on 0.4
#ifdef RESERVE_FOR_0_4 
        "0.4_reserved2", // 23
        "0.4_reserved3", // 24
        "0.4_reserved4", // 25
        "0.4_reserved5", // 26
        "0.4_reserved6", // 27
        "0.4_reserved7", // 28
        "0.4_reserved8", // 29
        "0.5_alpha", // 30
#endif
        0
    };

int sn_GetCurrentProtocolVersion()
{
    return (sizeof(sn_versionString)/sizeof(char const *)) - 2;
}

static char const * sn_GetVersionString( int version )
{
    tVERIFY ( version * sizeof( char * ) < sizeof sn_versionString );
    tVERIFY ( version >= 0 );

    return sn_versionString[ version ];
}

tOutput sn_GetClientVersionString(int version) {
    if(version >= 0 && version * sizeof(char *) < sizeof(sn_versionString)) {
        tOutput ret;
        ret.AddLiteral(sn_GetVersionString(version));
        return ret;
    }
    return tOutput("$network_unknown_version", version);
}

// *******************************************************************************************
// *
// *	AdaptVersion
// *
// *******************************************************************************************
//!
//!		@param	version	the version to adapt to the settings
//!
// *******************************************************************************************

// the last version we warned the user about
static nVersion lastVersion = sn_MyVersion();

void nConfItemVersionWatcher::AdaptVersion( nVersion & version )
{
    if ( sn_GetNetState() != nSERVER )
        return;

    // iterate over all watchers
    nStrongWatcherList & watchers = sn_GetStrongWatchers();
    for ( nStrongWatcherList::iterator iter = watchers.begin(); iter != watchers.end(); ++iter )
    {
        nConfItemVersionWatcher * run = *iter;

        // adapt version to needs
        if ( run->nonDefault_ && run->GetBehavior() >= Behavior_Block )
        {
            tVERIFY( version.Merge( version, run->version_ ) );
            if ( version.Min() > lastVersion.Min() )
            {
                // inform user about potential problem
                tOutput o;
                run->FillTemplateParameters(o);
                o << "$setting_legacy_clientblock";
                con << o;

                lastVersion = version;
            }
        }
    }
}

// *******************************************************************************************
// *
// *	OnVersionChange
// *
// *******************************************************************************************
//!
//!		@param	version	the version to adapt the settings to
//!
// *******************************************************************************************

void nConfItemVersionWatcher::OnVersionChange( nVersion const & version )
{
    // store version for reference
    lastVersion = version;

    if ( sn_GetNetState() != nSERVER )
        return;

    // iterate over all watchers
    nStrongWatcherList & watchers = sn_GetStrongWatchers();
    for ( nStrongWatcherList::iterator iter = watchers.begin(); iter != watchers.end(); ++iter )
    {
        nConfItemVersionWatcher * run = *iter;

        // warn about settings that will revert or be ignored
        Behavior behavior = run->GetBehavior();
        if (run->nonDefault_ && behavior != Behavior_Block )
        {
            // don't warn twice for the same group and behavior
            static int warnedRevert[ Behavior_Default ][ Group_Max ];
            {
                static bool inited = false;
                if (!inited)
                {
                    inited = true;
                    for ( int i = Behavior_Default-1; i>=0; --i )
                        for ( int j = Group_Max-1; j>=0; --j )
                            warnedRevert[i][j] = sn_MyVersion().Min();
                }
            }
            int & warned = warnedRevert[ behavior ][ run->group_ ];

            if ( warned < run->version_.Min() && run->version_.Min() > version.Min() )
            {
                warned = run->version_.Min();

                // inform user about potential problem with nondefault settings
                tOutput o;
                run->FillTemplateParameters(o);
                o << ( ( behavior == Behavior_Revert ) ? "$setting_legacy_revert" : "$setting_legacy_ignore" );
                con << o;
            }
        }

        // ignore settings where this is desired
        if ( run->GetBehavior() == Behavior_Nothing )
            continue;

        // if version is supported..
        if ( run->version_.Min() <= version.Max() )
        {
            // ...restore saved value of config item
            if ( run->reverted_ )
            {
                run->reverted_ = false;
                run->watched_.RevertToSavedValue();
            }
        }
        else
        {
            // version is not supported. Revert to defaults.
            if ( !run->reverted_ && run->nonDefault_ )
            {
                run->reverted_ = true;
                run->watched_.SaveValue();
                run->watched_.RevertToDefaults();
            }
        }

    }
}

static nConfItemVersionWatcher::Behavior sn_GroupBehaviors[ nConfItemVersionWatcher::Group_Max ] =
    {
        Behavior_Block,
        Behavior_Block,
        Behavior_Nothing,
        Behavior_Block,
        Behavior_Nothing,
    };

static tSettingItem< nConfItemVersionWatcher::Behavior > sn_GroupBehaviorBreaks( "SETTING_LEGACY_BEHAVIOR_BREAKING", sn_GroupBehaviors[ nConfItemVersionWatcher::Group_Breaking] );
static tSettingItem< nConfItemVersionWatcher::Behavior > sn_GroupBehaviorBumpy( "SETTING_LEGACY_BEHAVIOR_BUMPY", sn_GroupBehaviors[ nConfItemVersionWatcher::Group_Bumpy] );
static tSettingItem< nConfItemVersionWatcher::Behavior > sn_GroupBehaviorAnnoyance( "SETTING_LEGACY_BEHAVIOR_ANNOYING", sn_GroupBehaviors[ nConfItemVersionWatcher::Group_Annoying] );
static tSettingItem< nConfItemVersionWatcher::Behavior > sn_GroupBehaviorCheat( "SETTING_LEGACY_BEHAVIOR_CHEATING", sn_GroupBehaviors[ nConfItemVersionWatcher::Group_Cheating] );
static tSettingItem< nConfItemVersionWatcher::Behavior > sn_GroupBehaviorDisplay( "SETTING_LEGACY_BEHAVIOR_VISUAL", sn_GroupBehaviors[ nConfItemVersionWatcher::Group_Visual] );

// *******************************************************************************************
// *
// *	GetBehavior
// *
// *******************************************************************************************
//!
//!		@return		behavior if this setting is on non-default and a client that does not support it connects
//!
// *******************************************************************************************

nConfItemVersionWatcher::Behavior nConfItemVersionWatcher::GetBehavior( void ) const
{
    // look up default behavior
    tASSERT( 0 <= group_ && group_ < nConfItemVersionWatcher::Group_Max );
    Behavior behavior = sn_GroupBehaviors[ group_ ];

    // override it
    if ( Behavior_Default != this->overrideGroupBehavior_ )
        behavior = this->overrideGroupBehavior_;

    return behavior;
}

// *******************************************************************************************
// *
// *	GetBehavior
// *
// *******************************************************************************************
//!
//!		@param	behavior	behavior if this setting is on non-default and a client that does not support it connects to fill
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************************

nConfItemVersionWatcher const & nConfItemVersionWatcher::GetBehavior( Behavior & behavior ) const
{
    behavior = this->GetBehavior();
    return *this;
}

// *******************************************************************************************
// *
// *	DoWritable
// *
// *******************************************************************************************
//!
//!		@return		true if the watched item is changable
//!
// *******************************************************************************************

bool nConfItemVersionWatcher::DoWritable( void ) const
{
    // if we're not set to revert, the setting is writable
    if ( GetBehavior() != Behavior_Revert )
        return true;

    // and if the setting is currently supported by all parites, it's writable too
    if ( version_.Min() <= sn_CurrentVersion().Max() )
        return true;

    // inform user about impossible change
    tOutput o;
    FillTemplateParameters(o);
    o << "$setting_legacy_change_blocked";
    con << o;

    // only if it's not, it needs to be protected.
    return false;

}

// *******************************************************************************************
// *
// *	FillTemplateParameters
// *
// *******************************************************************************************
//!
//!		@param	o	output to fill. Template parameter 1 will be the setting name, parameter 2 the setting's group, and parameter 3 the first version supporting the setting
//!
// *******************************************************************************************

void nConfItemVersionWatcher::FillTemplateParameters( tOutput & o ) const
{
    o.SetTemplateParameter(1, watched_.GetTitle() );
    o.SetTemplateParameter(2, sn_groupName[group_] );
    o.SetTemplateParameter(3, sn_GetVersionString( version_.Min() ) );
}


