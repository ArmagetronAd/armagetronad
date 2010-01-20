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
    uActionTooltip( uAction & action, char const * help, int numHelp );
    ~uActionTooltip();

    //! presents help to the specified player, starting counting at 1. 
    //! player = 0 helps on global actions. Returns true if help was given.
    static bool Help( int player = 0 );

    //! call if an action was triggered
    void Count( int player );
private:
    virtual void WriteVal(std::ostream & s );
    virtual void ReadVal(std::istream & s );

    int activationsLeft_[uMAX_PLAYERS+1];
    tString const help_;
    uAction & action_;
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

class uBind: public tReferencable< uBind >
{
    friend class uMenuItemInput;

    virtual bool Delayable()=0;
    virtual bool DoActivate(REAL x)=0;
    REAL lastValue_;
    REAL delayedValue_;

    int lastSym_;     //! the last keysym used to activate this
    double lastTime_; //! the time of the last usage

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
    bool IsDoubleBind( int sym );
};

// extra binds for mouse (and joystick, as soon as SDL supports it) movement:
#define SDLK_MOUSE_X_PLUS   (SDLK_LAST+1)
#define SDLK_MOUSE_X_MINUS  (SDLK_LAST+2)
#define SDLK_MOUSE_Y_PLUS   (SDLK_LAST+3)
#define SDLK_MOUSE_Y_MINUS  (SDLK_LAST+4)
#define SDLK_MOUSE_Z_PLUS   (SDLK_LAST+5)
#define SDLK_MOUSE_Z_MINUS  (SDLK_LAST+6)
#define SDLK_MOUSE_BUTTON_1 (SDLK_LAST+7)
#define SDLK_MOUSE_BUTTON_2 (SDLK_LAST+8)
#define SDLK_MOUSE_BUTTON_3 (SDLK_LAST+9)
#define SDLK_MOUSE_BUTTON_4 (SDLK_LAST+10)
#define SDLK_MOUSE_BUTTON_5 (SDLK_LAST+11)
#define SDLK_MOUSE_BUTTON_6 (SDLK_LAST+12)
#define SDLK_MOUSE_BUTTON_7 (SDLK_LAST+13)
#define SDLK_NEWLAST        (SDLK_LAST+14)
#define MOUSE_BUTTONS 7

// one key_action for every keysym
extern tJUST_CONTROLLED_PTR< uBind > keymap[SDLK_NEWLAST];

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
#endif

