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

#include "rSDL.h"

#include "gServerFavorites.h"

#include "gLogo.h"
#include "gGame.h"

#include "uMenu.h"

#include "tConfiguration.h"

#include "gServerBrowser.h"
#include "nServerInfo.h"

#ifdef CONNECTION_STRESS
static bool sg_ConnectionStress = false;
#endif

#include <sstream>

enum { NUM_FAVORITES = 10 };

//! favorite server information, just to connect
/*
class gServerInfoFavorite: public nServerInfoBase
{
public:
    // construct a server directly with connection name and port
    gServerInfoFavorite( tString const & connectionName, unsigned int port )
    {
        nServerInfoBase::SetConnectionName( connectionName );
        nServerInfoBase::SetPort( port );
    };
};
*/
typedef nServerInfoRedirect gServerInfoFavorite;

static tString sg_ConfName( int ID, char const * name )
{
    std::stringstream s;
    s << "BOOKMARK_" << ID+1 << name;

    return tString( s.str().c_str() );
}

//! server favorite: holds connection info and configuration items
class gServerFavorite
{
public:
    friend class gServerFavoritesHolder;

    //! constructor
    gServerFavorite( int ID )
            : name_( "" )
            , port_( sn_defaultPort )
            , index_( ID )
            , confName_( sg_ConfName( ID, "_NAME") ,name_ )
            , confAddress_( sg_ConfName( ID, "_ADDRESS"), address_ )
            , confPort( sg_ConfName( ID, "_PORT"), port_ )
    {
    };

    //! connects to the server
    void Connect()
    {
        gServerInfoFavorite fav( address_, port_ );

        gLogo::SetDisplayed(false);

        ConnectToServer( &fav );
    }

    //! returns the index in the favorite holder
    int GetIndex()
    {
        return index_;
    }
public:
    tString         name_;        //!< the human readable name
    tString         address_;     //!< connection address
    int             port_;        //!< port to connect to

private:
    int             index_;       //!< index in favorite holder

    tConfItemLine confName_;      //!< configuration item holding the name
    tConfItemLine confAddress_;   //!< configuration item holding the address
    tConfItem<int> confPort;      //!< configuration item holding the port
};

//! server favorites management class: holds an array of servers
class gServerFavoritesHolder
{
public:
    gServerFavoritesHolder()
            :custom(-1)
    {
        // generate favorites
        for (int i = NUM_FAVORITES-1; i>=0; --i )
            favorites[i] = new gServerFavorite( i );
    }

    ~gServerFavoritesHolder()
    {
        // destroy favorites
        for (int i = NUM_FAVORITES-1; i>=0; --i )
            delete favorites[i];
    }

    //! returns the favorite server info with the given index
    gServerFavorite & GetFavorite( int index )
    {
        if ( index == -1 )
            return custom;

        tASSERT( index >=0 && index < NUM_FAVORITES );
        tASSERT( favorites[index] );
        return *favorites[index];
    }
private:
    // regular bookmarks
    gServerFavorite * favorites[NUM_FAVORITES];

    // custom connect server
    gServerFavorite custom;
};

// server bookmarks
static gServerFavoritesHolder sg_holder;

//! edit submenu item quitting the parent menu when it's done
class gMenuItemEditSubmenu: public uMenuItemSubmenu
{
public:
    gMenuItemEditSubmenu(uMenu *M,uMenu *s,
                         const tOutput& help)
            : uMenuItemSubmenu( M, s, help )
    {}

    //! enters the submenu
    virtual void Enter()
    {
        // delegate to base
        uMenuItemSubmenu::Enter();

        // exit the parent menu (so we don't have to update the edit menu)
        menu->Exit();
    }
};

//! connect to a favorite server
static void sg_ConnectFavorite( int ID )
{
    sg_holder.GetFavorite(ID).Connect();
}

//! conglomerate of menus and entries for custom connect
class gCustomConnectEntries
{
public:
    void Generate( gServerFavorite & fav, uMenu * menu )
    {
        // prepare output reading "Edit <server name>"
        // create menu items (autodeleted when the edit menu is killed)
        tNEW(uMenuItemFunctionInt) ( menu,"$network_custjoin_connect_text" ,"$network_custjoin_connect_help" ,&sg_ConnectFavorite, fav.GetIndex() );
        tNEW(uMenuItemInt)         ( menu,"$network_custjoin_port_text","$network_custjoin_port_help", fav.port_, gServerBrowser::lowPort, gServerBrowser::highPort);
        tNEW(uMenuItemString)      ( menu,"$bookmarks_menu_address","$bookmarks_menu_address_help",fav.address_);
    }

    gCustomConnectEntries()
    {
    }

    gCustomConnectEntries( gServerFavorite & fav, uMenu * menu )
    {
        Generate( fav, menu );
    }

    ~gCustomConnectEntries()
    {
    }

private:
};

//! conglomerate of menus and entries
class gServerFavoriteMenuEntries: public gCustomConnectEntries
{
public:
    gServerFavoriteMenuEntries( gServerFavorite & fav, uMenu & edit_menu )
    {
        // prepare output reading "Edit <server name>"
        tOutput fe;
        tString serverName = tColoredString::RemoveColors(fav.name_);
        if ( serverName == "" || serverName == "Empty" )
        {
            std::stringstream s;
            s << "Server " << fav.GetIndex()+1;

            serverName = s.str().c_str();
        }
        fe.SetTemplateParameter(1, serverName);
        fe << "$bookmarks_menu_edit_slot";

        // create edit menu
        edit_     = tNEW(uMenu)                (fe);
        editmenu_ = tNEW(gMenuItemEditSubmenu) ( &edit_menu, edit_, fe);

        Generate( fav, edit_ );

        tNEW(uMenuItemString)      ( edit_,"$bookmarks_menu_name","$bookmarks_menu_name_help",fav.name_);
    }

    ~gServerFavoriteMenuEntries()
    {
        delete editmenu_; editmenu_ = 0;
        delete edit_; edit_ = 0;
    }

private:
    uMenu     * edit_;
    uMenuItem * editmenu_;
};

static void sg_GenerateConnectionItems();

// Edit servers submenu funcion
static void sg_EditServers()
{
    int i;

    // create menu
    uMenu edit_menu("$bookmarks_menu_edit");

    // create menu entries
    gServerFavoriteMenuEntries * entries[ NUM_FAVORITES ];
    for ( i = NUM_FAVORITES-1; i>=0; --i )
        entries[i] = tNEW( gServerFavoriteMenuEntries )( sg_holder.GetFavorite(i), edit_menu );

    // enter menu
    edit_menu.Enter();

    // delete menu entries
    for ( i = NUM_FAVORITES-1; i>=0; --i )
        delete entries[i];

    // regenerate parent menu
    sg_GenerateConnectionItems();
}

// ugly hack: functions clearing and filling the connection menu
static uMenu * sg_connectionMenu = 0;
static uMenuItem * sg_connectionMenuItemKeep = 0;
static void sg_ClearConnectionItems()
{
    tASSERT( sg_connectionMenu );

    // delete old connection items
    for ( int i = sg_connectionMenu->NumItems()-1; i>=0; --i )
    {
        uMenuItem * item = sg_connectionMenu->Item(i);
        if ( item != sg_connectionMenuItemKeep )
            delete item;
    }
}
static void sg_GenerateConnectionItems()
{
    tASSERT( sg_connectionMenu );

    // delete old connection items
    sg_ClearConnectionItems();

    // create new connection items
    for ( int i = NUM_FAVORITES-1; i>=0; --i )
    {
        gServerFavorite & fav = sg_holder.GetFavorite(i);

        if (fav.name_ != "" && fav.name_ != "Empty" && fav.address_ != "")
        {
            tOutput fc; // Connect to "favn_name"
            fc.SetTemplateParameter(1,tColoredString::RemoveColors(fav.name_) );
            fc << "$bookmarks_menu_connect";

            tNEW(uMenuItemFunctionInt)(sg_connectionMenu ,fc ,"$network_custjoin_connect_help" ,&sg_ConnectFavorite, i );
        }
    }
}

//!TODO for 3.0 or 3.1: phase out this legacy support
static tString sg_customServerName("");
static tConfItemLine sg_serverName_ci("CUSTOM_SERVER_NAME",sg_customServerName);
static int sg_clientPort = 4534;
static tConfItem<int> sg_cport("CLIENT_PORT",sg_clientPort);

//! transfer old custom server name to favorite
static void sg_TransferCustomServer()
{
    if ( sg_customServerName != "" )
    {
        // add custom connect server to favorites
        gServerInfoFavorite server( sg_customServerName, sg_clientPort );
        gServerFavorites::AddFavorite( &server );

        // clear custom connect server
        sg_customServerName = "";
    }
}

// *********************************************************************************************
// *
// *	FavoritesMenu
// *
// *********************************************************************************************
//!
//!
// *********************************************************************************************

void gServerFavorites::FavoritesMenu( void )
{
    sg_TransferCustomServer();

    uMenu net_menu("$bookmarks_menu");
    sg_connectionMenu = & net_menu;

    uMenuItemFunction edit(&net_menu,"$bookmarks_menu_edit", "$bookmarks_menu_edit_help",&sg_EditServers);
    sg_connectionMenuItemKeep = & edit;

    sg_GenerateConnectionItems();
    net_menu.Enter();
    sg_ClearConnectionItems();

    sg_connectionMenuItemKeep = NULL;
    sg_connectionMenu = NULL;
}

// *********************************************************************************************
// *
// *	CustomConnectMenu
// *
// *********************************************************************************************
//!
//!
// *********************************************************************************************

void gServerFavorites::CustomConnectMenu( void )
{
    sg_TransferCustomServer();

    uMenu net_menu("$network_custjoin_text");
    sg_connectionMenu = & net_menu;

    gServerFavorite & fav = sg_holder.GetFavorite(-1);

    // create menu entries
    gCustomConnectEntries submenu( fav, &net_menu );

    net_menu.Enter();
}

// *********************************************************************************************
// *
// *	AddFavorite
// *
// *********************************************************************************************
//!
//!		@param	server	 the server to add to the favorites
//!     @return true if successful, false if favorite list is full
//!
// *********************************************************************************************

bool gServerFavorites::AddFavorite( nServerInfoBase const * server )
{
    if ( !server )
        return false;

    for ( int i = NUM_FAVORITES-1; i>=0; --i )
    {
        gServerFavorite & fav = sg_holder.GetFavorite(i);

        if (fav.name_ == "" || fav.name_ == "Empty")
        {
            fav.name_ = tColoredString::RemoveColors(server->GetName());
            fav.address_ = server->GetConnectionName();
            fav.port_ = server->GetPort();

            return true;
        }
    }

    return false;
}

// *********************************************************************************************
// *
// *	IsFavorite
// *
// *********************************************************************************************
//!
//!		@param	server	server to check whether it is bookmarked
//!		@return		    true if the server is in the list of favorites
//!
// *********************************************************************************************

bool gServerFavorites::IsFavorite( nServerInfoBase const * server )
{
    if ( !server )
        return false;

    for ( int i = NUM_FAVORITES-1; i>=0; --i )
    {
        gServerFavorite & fav = sg_holder.GetFavorite(i);

        if (fav.name_ != "" && fav.name_ != "Empty" && fav.address_ == server->GetConnectionName() && fav.port_ == static_cast< int >( server->GetPort() ) )
        {
            return true;
        }
    }

    return false;
}


