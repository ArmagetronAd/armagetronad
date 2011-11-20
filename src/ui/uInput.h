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

#ifndef ArmageTron_INPUT_H
#define ArmageTron_INPUT_H

#include "rSDL.h"
#include "tString.h"
#include "defs.h"
#include "tLinkedList.h"
#include "tConfiguration.h"
#include "tLocale.h"
#include "tSafePTR.h"

#define uMAX_PLAYERS 4

extern bool su_mouseGrab;         //! true if the mouse cursor is to be confined into the window
extern REAL su_doubleBindTimeout; //! timeout value for double binds; second keypress for the same action gets ignored from a different key when within the timeout

// ***************************
//      player controls
// ***************************

#define uMAX_ACTIONS 300

class uActionTooltip;

// the possible actions; player actions and global actions

class uAction:public tListItem<uAction>{
    friend class uActionTooltip;

    uActionTooltip *tooltip_;
protected:
    int localID;  // unique id on this host
    int globalID; // unique ID send from the server
public:
    typedef enum {uINPUT_DIGITAL,uINPUT_ANALOG} uInputType;

    uInputType     	type;
    int				priority;
    tString        	internalName;
    const tOutput  	description;
    const tOutput  	helpText;

    uActionTooltip * GetTooltip() const
    {
        return tooltip_;
    }

#ifdef SLOPPYLOCALE
    //  uAction(uAction *&anchor,const char *name,const char *desc,const char *help,
    //	 uInputType t=uINPUT_DIGITAL);
#endif
    uAction(uAction *&anchor,const char* name,
            int priority = 0,
            uInputType t=uINPUT_DIGITAL);

    uAction(uAction *&anchor,const char* name,
            const tOutput& desc,
            const tOutput& help,
            int priority = 0,
            uInputType t=uINPUT_DIGITAL);

    virtual ~uAction();

    //! find an action of the given name
    static uAction * Find( char const * name );

    unsigned short ID() const{return globalID;}
};

// tooltips, little help messages popping up, reminding the player
// of keybindings
class uActionTooltip: public tConfItemBase
{
public:
    enum Level
    {
        Level_Essential, // turns
        Level_Basic,     // speed control
        Level_Advanced,  // glancing
        Level_Expert     // switch camera modes, game menu, chat
    };

    typedef bool VETOFUNC(int player);
    uActionTooltip( Level level, uAction & action, int numHelp, VETOFUNC * veto = NULL );
    ~uActionTooltip();

    //! presents help to the specified player, starting counting at 1. 
    //! player = 0 helps on global actions. Returns true if help was given.
    static bool Help( int player = 0 );

    //! call if an action was triggered
    void Count( int player );

    //! call to show the tooltip one more time
    void ShowAgain();
private:
    virtual void WriteVal(std::ostream & s );
    virtual void ReadVal(std::istream & s );

    //! counts how many activations are required to make the tip go away
    int activationsLeft_[uMAX_PLAYERS+1];
    tString help_;        //!< help languate item
    uAction & action_;    //!< action this belongs to
    VETOFUNC * veto_;  //!< function that can block help display
    Level level_;      //!< level of sophistication
};


// player actions (move,shoot) and global actions (stop game, pause..)

class uActionPlayer:public uAction{
public:
    uActionPlayer(const char *name,
                  int priority = 0,
                  uInputType 	t=uINPUT_DIGITAL);

    uActionPlayer(const char *name,
                  const tOutput& desc,
                  const tOutput& help,
                  int priority = 0,
                  uInputType t=uINPUT_DIGITAL);

    virtual ~uActionPlayer();

    bool operator==(const uActionPlayer &x);

    static uActionPlayer *Find(int globalID);
};

class uActionCamera:public uAction{
public:
    uActionCamera(const char *name,
                  int priority = 0,
                  uInputType 	t=uINPUT_DIGITAL);

    virtual ~uActionCamera();

    bool operator==(const uActionCamera &x);
};

// global actions
class uActionGlobal:public uAction{
public:
    uActionGlobal(const char *name,
                  int priority = 0,
                  uInputType 	t=uINPUT_DIGITAL);

    virtual ~uActionGlobal();

    bool operator==(const uActionGlobal &x);

    // binding that should interrupt chat/console input
    static bool IsBreakingGlobalBind(int sym);
};




// *********************************************
// generic keypress/mouse movement binding class
// *********************************************

class uInput;

class uBind: public tReferencable< uBind >
{
    friend class uMenuItemInput;

    virtual bool Delayable()=0;
    virtual bool DoActivate(REAL x)=0;
    REAL lastValue_;
    REAL delayedValue_;

    uInput const * lastInput_;     //! the last input used to activate this
    double lastTime_;              //! the time of the last usage

public:
    uAction *act;

    uBind(uAction *a );
    uBind(std::istream &s);
    virtual ~uBind();

    virtual void Write(std::ostream &s);

    virtual bool CheckPlayer(int p)=0;

    bool Activate(REAL x, bool delayed );
    void HanldeDelayed();

    //! checks if the sym is used for double binding, return true
    bool IsDoubleBind( uInput const * input );
};

//! possible input channels (keys, mouse movement/buttons, joystick)
class uInput
{
public:
    uInput( tString const & persistentID, tString const & name );
    ~uInput();

    int ID(){ return ID_; }
    tString const & PersistentID(){ return persistentID_; }

    // the binding of this key
    void SetBind( uBind * bound ){ bound_ = bound; }
    uBind * GetBind() const { return bound_; }

    // the pressed status
    void SetPressed( float pressed ){ pressed_ = pressed; }
    float GetPressed() const { return pressed_; }

    // a human readable name
    std::string const Name(){ return name_; }
private:
    int ID_;               //!< local ID for this program run
    tString persistentID_; //!< global ID that does not change over program runs
    tString name_;         //!< human readable name

    tJUST_CONTROLLED_PTR< uBind > bound_; //!< the binding
    float pressed_;                       //!< analogue flag indicating whether the key/button was pressed
};

// *****************
// Player binds
// *****************

class uBindPlayer:public uBind{
    int            ePlayer;

    virtual bool Delayable();
    virtual bool DoActivate(REAL x);

    uBindPlayer(uAction *a,int p);
public:
    virtual ~uBindPlayer();

    static uBindPlayer * NewBind( uAction * action, int player );
    static uBindPlayer * NewBind( std::istream & s );

    static bool IsKeyWord(const char *n);

    virtual bool CheckPlayer(int p);

    virtual void Write(std::ostream &s);
};


// *****************
// Global actions
// *****************

typedef bool ACTION_FUNC(REAL x);

class uActionGlobalFunc: public tListItem<uActionGlobalFunc>{
    ACTION_FUNC   *func;
    uActionGlobal *act;
    bool           rebindable;

public:
    uActionGlobalFunc(uActionGlobal *a, ACTION_FUNC *f, bool rebind = true );
    static bool IsBreakingGlobalBind(uAction *act);
    static bool GlobalAct(uAction *act, REAL x);
};


// ***************************
//      player prototype
// ***************************

class uPlayerPrototype{
protected:
    static uPlayerPrototype* playerConfig[uMAX_PLAYERS];
    int id;
public:
    uPlayerPrototype();
    virtual ~uPlayerPrototype();

    virtual bool         Act(uAction *act, REAL x)=0;
    virtual const char * Name() const            =0;

    static uPlayerPrototype* PlayerConfig(int i);
    static int Num();
};



// the input configuration menu
void su_InputConfig(int player);
void su_InputConfigCamera(int player);
void su_InputConfigGlobal();
bool su_HandleEvent(SDL_Event &e, bool delayed );	// handle event during gameplay
void su_HandleDelayedEvents( );				// set menu state

void su_InputSync(); // tells the input system that a new frame has been drawn;
// autorepeat functions may be called.
void su_ClearKeys(); // clears all keys into the unpressed state.

// initialize keys (and mouse)
void su_KeyInit();

// initialize joysticks
void su_JoystickInit();

#endif

