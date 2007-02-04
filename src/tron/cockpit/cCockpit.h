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

//! @file
//! @brief contains the central point of the cockpit system
//!
//! This might get moved to src/ui, even though parts have to stay in
//! the higher regions of the sources.

#include "tXmlParser.h"
#include "tSafePTR.h"
#include <deque>
#include <map>
#include <set>

#include <memory>
#include <typeinfo>

#include "tValue.h"

#ifndef ARMAGETRON_COCKPIT_H
#define ARMAGETRON_COCKPIT_H

#ifndef DEDICATED

namespace cWidget {
class Base;
}
class ePlayer;
class ePlayerNetID;
class gCycle;

//! Cockpit class: keeps a list of widgets and delegates rendering and parsing to them
class cCockpit : private tXmlResource {
public:
    enum cameras {
        custom        = 0001,
        follow        = 0002,
        free          = 0004,
        in            = 0010,
        server_custom = 0020,
        smart         = 0040,
        mer           = 0100,
        all           = 0177
    }; //!< the different cameras, can be combined via the | operator

    cCockpit(); //!< default constructor
    ~cCockpit(); //!< destructor, calls ClearWidgets()

    static cCockpit* GetCockpit(); //!< returns a pointer to the instance of this class

    void SetPlayer(ePlayer *player); //!< sets the player the currently rendered viewport belongs to

    // Example how to call this:
    // theHud.SetViewMode(cCockpit::custom
    //                  & cCockpit::server_custom);
    // See the top of this file for all modes
    void SetCam(int Cam) { m_Cam = Cam; }; //!< Sets the camera. The widget's Render() function only gets called if the cameras of the widget and HUD overlap
    void AddEventHandler(int id, cWidget::Base *widget); //!< add a key event handler

    void RenderRootwindow(); //!< Renders the main viewport (all widgets that belong to the entire screen)
    void RenderPlayer(float factor); //!< Renders all widgets that belong to the currently active player

    void RenderCycle(gCycle const &cycle); //!< Render the cockpit elements above a cycle

    void ProcessCockpit(void); //!< Calls ClearWidgets(), then (re-) parses the cockpit file

    ePlayer *GetPlayer(); //!< The player the viewport belongs to
    gCycle *GetFocusCycle(void); //!< the cycle that's being followed in the current view
private:
    static cCockpit* _instance; //!< Stores a pointer to the current instance of the cockpit

    int m_Cam; //!< The currently active camera
    ePlayer* m_Player; //!< The player the viewport belongs to
    ePlayerNetID *m_FocusPlayer; //!< the player currently being watched
    ePlayerNetID *m_ViewportPlayer; //!< the player the viewport belongs to
    gCycle *m_FocusCycle; //!< The cycle currently being watched (the one that belongs to m_ViewportPlayer)
    typedef std::vector<tJUST_CONTROLLED_PTR< cWidget::Base> > widget_list_t;
    widget_list_t m_Widgets_perplayer; //!< All widgets that have to be rendered for every player
    widget_list_t m_Widgets_rootwindow; //!< All widgets that have to be rendered only once for the root window
    widget_list_t m_Widgets_cycles; //!< All widgets that have to be rendered above each cycle

    void ProcessWidgets(node cur); //!< Processes all Widgets within the <Cockpit> node passed to it
    std::auto_ptr<cWidget::Base> ProcessWidgetType(node cur); //!< returns a new instance of the right widget class for the given node

    //this should be done by the cWidget::Base class one day
    void ProcessWidgetCamera(node cur, cWidget::Base &widget); //!< Processes the camera of the widget if given its root node
    void ProcessWidgetCore(node cur, cWidget::Base &widget); //!< Passes each child of a widget root node to its Process() function
    void ProcessWidget(node cur, cWidget::Base &widget); //!< Parses the camer, core and similar settings of a widget

    void ClearWidgets(void); //!< Destroys all widgets owned by this class

public:


    //callback functions for subwidgets
    tValue::BasePtr cb_CurrentRubber(void);           //!< Gets the used rubber for the currently watched cycle
    tValue::BasePtr cb_CurrentAcceleration(void);           //!< Gets the current acceleration for the currently watched cycle
    tValue::BasePtr cb_CurrentPing(void);             //!< Gets the current ping for the player the viewport belongs to in ms
    tValue::BasePtr cb_CurrentSpeed(void);            //!< Gets the speed of the currently watched cycle in m/s
    tValue::BasePtr cb_MaxSpeed(void);                //!< Gets the maximum possible speed on the server
    tValue::BasePtr cb_CurrentBrakingReservoir(void); //!< Gets the available brakes for the currently watched cycle

    tValue::BasePtr cb_AliveEnemies(void);            //!< Gets the number of enemies alive (from the viewport owner's perspective)
    tValue::BasePtr cb_AliveTeammates(void);          //!< Gets the number of Teammates alive (from the viewport owner's perspective)

    tValue::BasePtr cb_Framerate(void);               //!< Gets the current framerate in fps
    tValue::BasePtr cb_RunningTime(void);             //!< Gets the time the game is running in seconds
    tValue::BasePtr cb_CurrentTimeMinutes(void);      //!< Gets the current time in minutes
    tValue::BasePtr cb_CurrentTimeHours(void);        //!< Gets the current time in hours
    tValue::BasePtr cb_CurrentTimeHours12h(void);     //!< Gets the current time in hours, 12h format
    tValue::BasePtr cb_CurrentTimeSeconds(void);      //!< Gets the current time in seconds

    tValue::BasePtr cb_CurrentScore(void);            //!< Gets the viewport owner's score
    tValue::BasePtr cb_TopScore(void);                //!< Gets the top personal score
    tValue::BasePtr cb_CurrentScoreTeam(void);            //!< Gets the viewport owner's team's score
    tValue::BasePtr cb_TopScoreTeam(void);                //!< Gets the top team score
    tValue::BasePtr cb_FastestSpeed(void);            //!< Gets the speed of the player who's currently the fastest in m/s
    tValue::BasePtr cb_FastestName(void);             //!< Gets the name of the player who's currently the fastest
    tValue::BasePtr cb_FastestSpeedRound(void);            //!< Gets the speed of the player who's been the fastest during the round in m/s
    tValue::BasePtr cb_FastestNameRound(void);             //!< Gets the name of the player who's been the fastest during the round

    tValue::BasePtr cb_TimeToImpactFront(void);             //!< Gets the time it will take the cycle to reach the next wall in front of it
    tValue::BasePtr cb_TimeToImpactRight(void);             //!< Gets the time it will take the cycle to reach the next wall right of it
    tValue::BasePtr cb_TimeToImpactLeft(void);             //!< Gets the time it will take the cycle to reach the next wall left of it
    tValue::BasePtr cb_CurrentSong(void);		    //!< Gets the file name of the song currently played. May be swiched to the title embedded in the music file later
    tValue::BasePtr cb_CurrentName(void);		    //!< Gets the name of the player that is being watched
    tValue::BasePtr cb_CurrentColoredName(void);		    //!< Gets the colored name of the player that is being watched
    tValue::BasePtr cb_CurrentPosX(void);		    //!< Gets the position of the cycle as x- coordinate
    tValue::BasePtr cb_CurrentPosY(void);		    //!< Gets the position of the cycle as y- coordinate

    static bool ProcessKey1(float i=0);
    static bool ProcessKey2(float i=0);
    static bool ProcessKey3(float i=0);
    static bool ProcessKey4(float i=0);
    static bool ProcessKey5(float i=0);
    std::multimap<int, cWidget::Base *> m_EventHandlers;
    bool HandleEvent(int id, bool state);

    void Readjust(void); //!< Readjusts the cockpit to a new window height
};

extern const std::map<tString, tValue::Callback<cCockpit>::cb_ptr> stc_callbacks;
extern std::set<tString> stc_forbiddenCallbacks;

#endif
#endif
