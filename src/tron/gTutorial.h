/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2005  by 
and the AA DevTeam (see the file AUTHORS(.txt) in the main source directory)

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

#ifndef ArmageTron_GTUTORIAL_H
#define ArmageTron_GTUTORIAL_H

class tString;
struct gGameSettings;
class eTeam;

//! opens the tutorial menu
void sg_TutorialMenu();

//! returns whether all tutorials have been completed
bool sg_TutorialsCompleted();

class gTutorialBase
{
public:
    virtual ~gTutorialBase(){}

    // analyzes the game every second, sets uMenu::quickexit if 
    // the tutorial was failed or passed
    virtual void Analysis() = 0;

    // called before objects are spawned
    virtual void BeforeSpawn();

    // called after objects are spawned
    virtual void AfterSpawn();
    
    // called the moment a team won
    virtual void OnWin( eTeam * winner );

    // called when the round really ends
    virtual void RoundEnd( eTeam * winner );

    virtual tString const & Name() const = 0;
};

// defined in gGame.cpp
void sg_TutorialGame( gGameSettings & settings, gTutorialBase & tutorial );

#endif
