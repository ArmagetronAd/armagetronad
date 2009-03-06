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

#ifndef ArmageTron_AI_NAVIGATOR_H
#define ArmageTron_AI_NAVIGATOR_H

#include "gSensor.h"
#include "eCoord.h"

class gCycle;

//! AI helper class that knows the basics of staying alive.
class gAINavigator
{
    gAINavigator();
public:
    //! settings used by the idler bot
    struct Settings
    {
        REAL newWallBlindness; //!< number of seconds new walls are invisible to the idler
        REAL range; //!< seconds to plan ahead
        
        Settings();
    };

    Settings settings_; // settings to use

    //! turn wish passed from outside
    struct Wish
    {
        int turn;             //!< direction to turn into (values >1 turn multiple times)

        REAL maxDisadvantage; //!< maximal disadvantage of the wish direction over others to have it still accepted
        REAL minDistance;     //!< minimal 'distance' value of the wish direction to have it accepted

        Wish( gAINavigator const & idler );
    private:
        Wish();
    };

    // sensor with additional data
    class Sensor: public gSensor
    {
    public:
        Sensor(gAINavigator & ai,const eCoord &start,const eCoord &d);

        virtual void PassEdge(const eWall *ww,REAL time,REAL a,int r);

        // check how far the hit wall extends straight into the given direction
        REAL HitWallExtends( eCoord const & dir, eCoord const & origin );

        gAINavigator & ai_;          // AI using this sensor
        gCycle * hitOwner_;     // the owner of the hit wall
        REAL     hitTime_;      // the time the hit wall was built at
        REAL     hitDistance_;  // the distance of the wall to the cycle that built it
        short    lrSuggestion_; // sensor's oppinon on whether moving to the left or right of the hit wall is recommended (-1 for left, +1 for right)
        int      windingNumber_; // the number of turns (with sign) the cycle has taken

    private:
        bool DoExtraDetectionStuff();
    };

    gAINavigator( gCycle * owner );

    // describes walls we like. We like enemy walls. We like to go between them.
    class WallHug
    {
    public:
        gCycle const * owner_;  // the cycle the walls we like belong to
        REAL lastTimeSeen_;    // the last time we saw such a wall

        WallHug();
    };

    // returns the controlled cycle
    gCycle * Owner() const
    {
        return owner_;
    }

    // promote seen walls to possible wallhug replacements
    void FindHugReplacement( Sensor const & sensor );

    // determines the distance between two sensors; the size should give the likelyhood
    // to survive if you pass through a gap between the two selected walls
    REAL Distance( Sensor const & a, Sensor const & b );

    bool CanMakeTurn( uActionPlayer * action );

    //! does the main thinking at the current time, knowing the next thought can't be sooner than minstep
    REAL Activate( REAL currentTime, REAL minstep, REAL penalty = 0, Wish * wish = 0 );

private:
    short lastTurn_;         //!< the last turn the chat AI made
    REAL nextTurn_;          //!< the next turn if one is planned
    bool turnedRecently_;    //!< whether the cycle was turned or almost turned recently
    gCycle * owner_;         //!< owner of chatbot

    WallHug hugLeft_;              //!< the wall we like to have on our left side
    WallHug hugRight_;             //!< the wall we like to have on our right side
    WallHug hugReplacement_;       //!< a possible replacement candidate for one of the hugged walls
};

#endif
