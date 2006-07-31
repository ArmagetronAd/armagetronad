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

#include "gLanguageMenu.h"
#include "uMenu.h"
#include "tLocale.h"
#include "tConfiguration.h"

static tString sg_FirstLanguage("American English"), sg_SecondLanguage("American English");

static tConfItemLine sg_fl("LANGUAGE_FIRST",sg_FirstLanguage);
static tConfItemLine sg_sl("LANGUAGE_SECOND",sg_SecondLanguage);

static void sg_UpdateLanguage( tString & language )
{
    // update outdated language choices
    if ( language == "English" )
        language = "British English";

    if ( language == "American" )
        language = "American English";
}

void sg_LanguageInit()
{
    sg_UpdateLanguage( sg_FirstLanguage );
    sg_UpdateLanguage( sg_SecondLanguage );
    tLanguage::Find(sg_FirstLanguage )->SetFirstLanguage();
    tLanguage::Find(sg_SecondLanguage)->SetSecondLanguage();
}

// Language menu
void sg_LanguageMenu()
{
    if (!tLanguage::FirstLanguage())
        return;

    uMenu menu("$language_menu_title", false);
    //  uMenu menu("Sprach Menu", false);

    uMenuItemExit exit(&menu, "$menuitem_accept", "$menuitem_accept_help");

    uMenuItemSelection<tString> second(&menu,
                                       "$language_menu_item_second",
                                       "$language_menu_item_second_help",
                                       sg_SecondLanguage);

    uMenuItemSelection<tString> first(&menu,
                                      "$language_menu_item_fist",
                                      "$language_menu_item_first_help",
                                      sg_FirstLanguage);

    tLanguage* run = tLanguage::FirstLanguage();


    while (run)
    {
        first.NewChoice(run->Name(),"", run->Name());
        second.NewChoice(run->Name(),"", run->Name());
        run = run->Next();
    }

    menu.Enter();

    sg_LanguageInit();
}



