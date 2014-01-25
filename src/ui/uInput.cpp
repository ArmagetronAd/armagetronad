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

#include <stdarg.h>
#include "uInput.h"
#include "tMemManager.h"
#include "rScreen.h"
#include "tInitExit.h"
#include "tConfiguration.h"
#include "rConsole.h"
#include "uMenu.h"
#include "tSysTime.h"

#include <vector>
#include <map>

// Touch devices support (touchscreen or touchpad)
// 0:disabled
// 1:touch control by left/right touches
// 2:touch control by swipes and drawing
static int su_enableTouch = 0;
static tSettingItem< int > su_enableTouchConf( "ENABLE_TOUCH", su_enableTouch );

bool su_mouseGrab = false;

static uAction* su_allActions[uMAX_ACTIONS];
static int     su_allActionsLen = 0;

uAction::uAction(uAction *&anchor,const char* name,
                 int priority_,
                 uInputType t)
        :tListItem<uAction>(anchor),tooltip_(NULL), shouldShowInGenericConfigurationMenu_( true ), type(t),priority(priority_),internalName(name)
{
    globalID = localID = su_allActionsLen++;

    tASSERT(localID < uMAX_ACTIONS);

    su_allActions[localID] = this;

    tString descname;
    descname << "input_" << name << "_text";
    tToLower( descname );

    const_cast<tOutput&>(description).AddLocale(descname);

    tString helpname;
    helpname << "input_" << name << "_help";
    tToLower( helpname );

    const_cast<tOutput&>(helpText).AddLocale(helpname);
}

uAction::uAction(uAction *&anchor,const char* name,
                 const tOutput& desc,
                 const tOutput& help,
                 int priority_,
                 uInputType t)
        :tListItem<uAction>(anchor),tooltip_(NULL), shouldShowInGenericConfigurationMenu_( true ), type(t),priority(priority_),internalName(name), description(desc), helpText(help)
{
    globalID = localID = su_allActionsLen++;

    tASSERT(localID < uMAX_ACTIONS);

    su_allActions[localID] = this;
}

uAction::~uAction(){
    su_allActions[localID] = NULL;
}

uAction * uAction::Find( char const * name )
{
    for (int i=su_allActionsLen-1;i>=0;i--)
        if (!strcmp(name,su_allActions[i]->internalName))
            return su_allActions[i];

    return 0;
}

// ****************************************
// uInput
// ****************************************

typedef std::vector< uInput * > uInputs;
uInputs su_inputs;
std::map< std::string, uInput * > su_inputMap;

uInput::uInput( tString const & persistentID, tString const & name )
        : persistentID_( persistentID ), name_( name )
        , pressed_( 0 )
{
    ID_ = su_inputs.size();
    su_inputs.push_back( this );
    su_inputMap[ persistentID ] = this;
}

uInput::~uInput()
{
    su_inputs[ID_] = 0;
    // su_inputMap.erase( su_inputMap.find( persistentID_ ) );
}

static uInput * su_GetInput( tString const & persistentID )
{
    try
    {
        return su_inputMap[ persistentID ];
    }
    catch (...)
    {
        return NULL;
    }
}

// class that manages unknown input
class uAutoDeleteInput
{
public:
    uAutoDeleteInput()
    {
    }

    ~uAutoDeleteInput()
    {
        for ( uInputs::iterator iter = unknowns.begin(); iter != unknowns.end(); ++iter )
        {
            delete(*iter);
            *iter = 0;
        }
    }

    uInput * Create( tString const & persistentID, tString const & name )
    {
        uInput * input = new uInput( persistentID, name );
        unknowns.push_back( input );
        return input;
    }

    // array of unknown inputs that should be deleted afterwards
    uInputs unknowns;
};

static uAutoDeleteInput & su_GetAutoDeleteInput()
{
    static uAutoDeleteInput filler;
    return filler;
}

static uInput * su_NewInput( tString const & ID, tString const & name )
{
    return su_GetAutoDeleteInput().Create( ID, name );
}

static uInput * su_NewInput( char const * ID, char const * name )
{
    return su_GetAutoDeleteInput().Create( tString( ID ), tString( name ) );
}

// class that manages keyboard input
class uKeyInput
{
public:
    uKeyInput()
    {
#if SDL_VERSION_ATLEAST(2,0,0)
        for ( int i = 0; i <= SDL_NUM_SCANCODES; ++i )
#else
        for ( int i = 0; i <= SDLK_LAST; ++i )
#endif
        {
#ifndef DEDICATED
#if SDL_VERSION_ATLEAST(2,0,0)
            char const * ID_Raw = SDL_GetScancodeName(static_cast< SDL_Scancode >( i ) );
#else
            char const * ID_Raw = SDL_GetKeyName(static_cast< SDLKey >( i ) );
#endif
#else
            char const * ID_Raw = "unknown key";
#endif
            std::ostringstream id;
            id << "KEY_";
            if ( strcmp( ID_Raw, "unknown key" ) == 0 )
            {
                id << "UNKNONW_" << i;
            }
            else
            {
                for ( char const * c = ID_Raw; *c; ++c )
                {
                    if ( isblank( *c ) )
                    {
                        id << "_";
                    }
                    else
                    {
                        id << char(toupper( *c ));
                    }
                }
            }

            // store new input key
            sdl_keys[i] = su_NewInput( id.str(), tString(ID_Raw) );

            tASSERT( sdl_keys[i]->ID() == i );
        }
    }

    ~uKeyInput()
    {
    }

#if SDL_VERSION_ATLEAST(2,0,0)
    uInput * sdl_keys[SDL_NUM_SCANCODES+1];
#else
    uInput * sdl_keys[SDLK_LAST+1];
#endif
};

static uKeyInput const & su_GetKeyInput()
{
    static uKeyInput filler;
    return filler;
}

#if SDL_VERSION_ATLEAST(2,0,0)
class uTouchInput
{
public:
    uTouchInput() {
        turnLeft  = su_NewInput( "TOUCH_LEFT_TURN", "touch left turn" );
        turnRight = su_NewInput( "TOUCH_RIGHT_TURN", "touch right turn" );
        brake     = su_NewInput( "TOUCH_BRAKE", "touch brake" );

        uAction * actLeft  = uAction::Find( "CYCLE_TURN_LEFT" );
        uAction * actRight = uAction::Find( "CYCLE_TURN_RIGHT" );
        uAction * actBrake = uAction::Find( "CYCLE_BRAKE" );

        tJUST_CONTROLLED_PTR< uBind > bindLeft  = uBindPlayer::NewBind(actLeft, 1);
        tJUST_CONTROLLED_PTR< uBind > bindRight = uBindPlayer::NewBind(actRight, 1);
        tJUST_CONTROLLED_PTR< uBind > bindBrake = uBindPlayer::NewBind(actBrake, 1);

        turnLeft->SetBind(bindLeft);
        turnRight->SetBind(bindRight);
        brake->SetBind(bindBrake);
    }

    uInput* turnLeft;
    uInput* turnRight;
    uInput* brake;
};

static uTouchInput const & su_GetTouchInput()
{
    static uTouchInput filler;
    return filler;
}
#endif

# define MOUSE_BUTTONS 7

// class that manages mouse inout
class uMouseInput
{
public:
    uMouseInput()
    {
        x_plus = su_NewInput( "MOUSE_X_PLUS", "mouse right" );
        x_minus = su_NewInput( "MOUSE_X_MINUS", "mouse left" );
        y_plus = su_NewInput( "MOUSE_Y_PLUS", "mouse up" );
        y_minus = su_NewInput( "MOUSE_Y_MINUS", "mouse down" );
        z_plus = su_NewInput( "MOUSE_Z_PLUS", "mouse z up" );
        z_minus = su_NewInput( "MOUSE_Z_MINUS", "mouse z down" );

        for ( int i = 0; i < MOUSE_BUTTONS; ++i )
        {
            std::ostringstream id;
            id << "MOUSE_BUTTON_" << i+1;
            std::ostringstream name;
            name << "mouse button " << i+1;
            button[i] = su_NewInput( id.str(), name.str() );
        }
    }

    uInput * x_plus;
    uInput * x_minus;
    uInput * y_plus;
    uInput * y_minus;
    uInput * z_plus;
    uInput * z_minus;
    uInput * button[MOUSE_BUTTONS];
};

static uMouseInput const & su_GetMouseInput()
{
    static uMouseInput filler;
    return filler;
}

void su_KeyInit()
{
    su_GetKeyInput();
    su_GetMouseInput();
}

// ****************************************
// a configuration class for keyboard binds
// ****************************************

class tConfItem_key:public tConfItemBase{
public:
    tConfItem_key():tConfItemBase("KEYBOARD"){}
    ~tConfItem_key(){}

    // write the complete keymap
    virtual void WriteVal(std::ostream &s){
        int first=1;
        for ( uInputs::const_iterator i = su_inputs.begin(); i != su_inputs.end(); ++i )
        {
            if ( !(*i)->GetBind() )
                continue;

            std::string const & id = (*i)->PersistentID();

            if (!first)
                s << "\nKEYBOARD\t";
            else
                first=0;

            s << id << '\t';
            (*i)->GetBind()->Write(s);
        }
        if (first)
            s << "-1";
    }

    // read one keybind
    virtual void ReadVal(std::istream &s)
    {
        tString in;
        std::string id;
        s >> id;
        if ( id != "-1" )
        {
            // try to fetch the uInput belonging to the id
            uInput * input = su_GetInput( id );

            if ( !input )
            {
                // if the id is a number, the setting is in a legacy format.
                std::istringstream readValue( id );
                int value = -1;
                readValue >> value;

                if ( value >= 0 && value < (int)su_inputs.size() )
                {
                    input = su_inputs[ value ];
                }
            }

            if ( !input )
            {
                input = su_NewInput( id, tString("") );
            }

            tASSERT( input );

            s >> in;
            if (uBindPlayer::IsKeyWord(in))
            {
                tJUST_CONTROLLED_PTR< uBind > bind = uBindPlayer::NewBind(s);
                if ( bind->act )
                {
                    input->SetBind( bind );
                }
            }
        }
        char c=' ';
        while (c!='\n' && s.good() && !s.eof()) c=s.get();
    }
};

// we need just one
static tConfItem_key x;

static uAction *s_playerActions;
static uAction *s_cameraActions;
static uAction *s_globalActions;

uActionPlayer::uActionPlayer(const char *name,
                             int priority,
                             uInputType t)
    :uAction(s_playerActions,name,priority,t){}

uActionPlayer::uActionPlayer(const char *name,
                             const tOutput& desc,
                             const tOutput& help,
                             int priority,
                             uInputType t)
        :uAction(s_playerActions,name,desc,help,priority,t){}

uActionPlayer::~uActionPlayer(){}

bool uActionPlayer::operator==(const uActionPlayer &x){
    return x.globalID == globalID;
}

uActionPlayer *uActionPlayer::Find(int id){
    uAction *run = s_playerActions;

    while (run){
        if (run->ID() == id)
            return static_cast<uActionPlayer*>(run);
        run = run->Next();
    }

    return NULL;
}


uActionCamera::uActionCamera(const char *name,
                             int priority,
                             uInputType t)
    :uAction(s_cameraActions,name,priority,t){}

uActionCamera::~uActionCamera(){}

bool uActionCamera::operator==(const uActionCamera &x){
    return x.globalID == globalID;
}


// global actions
uActionGlobal::uActionGlobal(const char *name,
                             int priority,
                             uInputType t)
        :uAction(s_globalActions,name,priority,t){}

uActionGlobal::~uActionGlobal(){}

bool uActionGlobal::operator==(const uActionGlobal &x){
    return x.globalID == globalID;
}

bool uActionGlobal::IsBreakingGlobalBind(int sym){
    if ( sym >= (int)su_inputs.size() || sym < 0 )
        return false;

    uInput * input = su_inputs[ sym ];
    if ( !input )
        return false;

    uBind * bind = input->GetBind();
    if ( !bind )
        return false;

    uAction *act = bind->act;
    if (!act)
        return false;

    return uActionGlobalFunc::IsBreakingGlobalBind(act);
}

static void keyboard_init()
{
}

static void keyboard_exit()
{
}

#ifndef NOJOYSTICK

#ifndef DEDICATED
class uJoystick
{
public:
    int id;

    // joystick name
    tString name, internalName;

    // number of compotents
    int numAxes, numButtons, numBalls, numHats;

    enum Direction
    {
        Left = 0,
        Right = 1,
        Up = 2,
        Down = 3
    };

    uJoystick( int id_ ):id(id_)
    {
        tASSERT( id >= 0 && id < SDL_NumJoysticks() );

        SDL_Joystick * stick = SDL_JoystickOpen( id );
        name = SDL_JoystickName( stick );

        std::ostringstream iName;
        iName << "JOYSTICK_";
        for ( char const * c = name.c_str(); *c; ++c )
        {
            if ( isblank( *c ) )
            {
                iName << "_";
            }
            else
            {
                iName << char(toupper( *c ));
            }
        }

        internalName = iName.str();

        numAxes = SDL_JoystickNumAxes( stick );
        numButtons = SDL_JoystickNumButtons( stick );
        numBalls = SDL_JoystickNumBalls( stick );
        numHats = SDL_JoystickNumHats( stick );

        // populate input types
        if ( numAxes >= 1 )
        {
            axes[0].push_back( su_NewInput( GetPersistentID( "AXIS", 0, "-" ), GetName( "left" ) ) );
            axes[1].push_back( su_NewInput( GetPersistentID( "AXIS", 0, "+" ), GetName( "right" ) ) );
        }

        if ( numAxes >= 2 )
        {
            axes[0].push_back( su_NewInput( GetPersistentID( "AXIS", 1, "-" ), GetName( "up" ) ) );
            axes[1].push_back( su_NewInput( GetPersistentID( "AXIS", 1, "+" ), GetName( "down" ) ) );
        }

        for ( int axis = 2; axis < numAxes; ++axis )
        {
            axes[0].push_back( su_NewInput( GetPersistentID( "AXIS", axis, "-" ), GetName( "axis", axis, "-" ) ) );
            axes[1].push_back( su_NewInput( GetPersistentID( "AXIS", axis, "+" ), GetName( "axis", axis, "+" ) ) );
        }

        for ( int button = 0; button < numButtons; ++button )
        {
            buttons.push_back( su_NewInput( GetPersistentID( "BUTTON", button ), GetName( "button", button ) ) );
        }

        char const * directionInternal[] = { "LEFT", "RIGHT", "UP", "DOWN" };
        char const * directionHuman[] = { "left", "right", "up", "down" };

        for ( int dir = 0; dir < 4; ++dir )
        {
            char const * internal = directionInternal[dir];
            char const * human = directionHuman[dir];

            for ( int ball = 0; ball < numBalls; ++ball )
            {
                balls[dir].push_back( su_NewInput( GetPersistentID( "BALL", ball, internal ), GetName( "ball", ball, human ) ) );
            }

            for ( int hat = 0; hat < numHats; ++hat )
            {
                hats[dir].push_back( su_NewInput( GetPersistentID( "HAT", hat, internal ), GetName( "hat", hat, human ) ) );
            }
        }

        hatDirection.resize( numHats );
    }

    uInput * GetAxis( int index, int dir )
    {
        tASSERT( index >= 0 && index < numAxes );
        tASSERT( 0 == dir || 1 == dir );

        axes[1-dir][index]->SetPressed(0);
        return axes[dir][index];
    }

    uInput * GetButton( int index )
    {
        tASSERT( index >= 0 && index < numButtons );

        return buttons[index];
    }

    uInput * GetBall( int index, Direction dir )
    {
        tASSERT( index >= 0 && index < numBalls );

        balls[dir ^ 1][index]->SetPressed(0);
        return balls[dir][index];
    }

    uInput * GetHat( int index, Direction dir )
    {
        tASSERT( index >= 0 && index < numHats );

        hats[dir ^ 1][index]->SetPressed(0);
        return hats[dir][index];
    }

    int & GetHatDirection( int index, int axis )
    {
        tASSERT( index >= 0 && index < numHats );
        tASSERT( 0 == axis || 1 == axis );

        return hatDirection[index].dir[axis];
    }
private:
    // various input arrays
    uInputs axes[2], buttons, balls[4], hats[4];

    struct HatDirections
    {
        int dir[2];
        HatDirections(){
            dir[0] = dir[1] = 0;
        }
    };

    std::vector< HatDirections > hatDirection;

    // returns an uInput unique name
    tString GetPersistentID( char const * type, int subID, char const * suffix = NULL ) const
    {
        std::ostringstream o;
        o << internalName << "_" << type << "_" << subID;
        if ( suffix )
        {
            o << "_" << suffix;
        }
        return o.str();
    }

    // same for the public name
    tString GetName( char const * type, int subID, char const * suffix = NULL ) const
    {
        std::ostringstream o;
        o << "Joystick " << id+1 << " " << type << " " << subID+1;
        if ( suffix )
        {
            o << " " << suffix;
        }
        return o.str();
    }

    // special axes: x and y
    tString GetName( char const * type ) const
    {
        std::ostringstream o;
        o << "Joystick " << id+1 << " " << type;
        return o.str();
    }
};

// class that manages joysticks
class uJoystickInput
{
public:
    uJoystickInput()
    {
        // create joysticks
        int numJoysticks = SDL_NumJoysticks();
        for ( int i = 0; i < numJoysticks; ++i )
        {
            joysticks.push_back( new uJoystick( i ) );
        }
    }

    ~uJoystickInput()
    {
        // delete joysticks
        for ( std::vector< uJoystick * >::iterator iter = joysticks.begin(); iter != joysticks.end(); ++iter )
        {
            delete(*iter);
            *iter = 0;
        }
    }

    // array of joysticks
    std::vector< uJoystick * > joysticks;
};

static uJoystickInput & su_GetJoystickInput()
{
    static uJoystickInput filler;
    return filler;
}

static uJoystick * su_GetJoystick( int id )
{
    uJoystickInput & joysticks = su_GetJoystickInput();
    if(id >= 0 && id < (int)joysticks.joysticks.size() )
        return joysticks.joysticks[id];
    else
        return NULL;
}

void su_JoystickInit()
{
    su_GetJoystickInput();
    SDL_JoystickEventState( SDL_ENABLE );
}
#endif
#endif

static tInitExit keyboard_ie(&keyboard_init, &keyboard_exit);

// *********************************************
// generic keypress/mouse movement binding class
// *********************************************

uBind::~uBind(){}

uBind::uBind(uAction *a ):lastValue_(0), delayedValue_(0), lastInput_(0), lastTime_(-1), act(a){}

uBind::uBind(std::istream &s): lastValue_(0), delayedValue_(0), lastInput_(0), lastTime_(-1), act(NULL)
{
    std::string name;
    s >> name;
    act = uAction::Find( name.c_str() );
}

void uBind::Write(std::ostream &s){
    s << act->internalName << '\t';
}

bool GlobalAct(uAction *act,REAL x){
    return uActionGlobalFunc::GlobalAct(act,x);
}

bool uBind::Activate(REAL x, bool delayed )
{
    delayedValue_ = x;

    if ( !delayed || !Delayable() )
    {
        lastValue_ = x;
        return this->DoActivate( x );
    }

    return true;
}

void uBind::HanldeDelayed()
{
    if ( lastValue_ != delayedValue_ )
    {
        lastValue_ = delayedValue_;
        this->DoActivate( delayedValue_ );
    }
}

REAL su_doubleBindTimeout=-10.0f;

bool uBind::IsDoubleBind( uInput const * input )
{
    double currentTime = tSysTimeFloat();

    // if a different key was used for this action a short while ago, give alarm.
    bool ret = ( su_doubleBindTimeout > 0 && input != lastInput_ && currentTime - lastTime_ < su_doubleBindTimeout );

    // store last usage
    lastInput_ = input;
    lastTime_ = currentTime;

    // return result
    return ret;
}

// *******************
// player config
// *******************

static int nextid = 0;

uPlayerPrototype* uPlayerPrototype::PlayerConfig(int i){
    tASSERT(i>=0 && i<uMAX_PLAYERS);
    return playerConfig[i];
}


uPlayerPrototype::uPlayerPrototype(){
    static bool inited=false;
    if (!inited)
    {
        for (int i=uMAX_PLAYERS-1; i >=0; i--)
            playerConfig[i] = NULL;

        inited = true;
    }

    id = nextid++;
    tASSERT(id < uMAX_PLAYERS);
    playerConfig[id] = this;


}

uPlayerPrototype::~uPlayerPrototype(){
    playerConfig[id] = NULL;
}

uPlayerPrototype* uPlayerPrototype::playerConfig[uMAX_PLAYERS];

int uPlayerPrototype::Num(){
    return nextid;
}

// *******************
// Input configuration
// *******************

REAL mouse_sensitivity=REAL(.1);

// number of transformed events per SDL event
#define su_TRANSFORM_PASSES 2

struct uTransformEventInfo
{
    uInput * input;   // the input event type received
    float value;      // the strenght of the event
    bool needsRepeat; // if the event should trigger a continuous action (camera movement), should it be repeated?

    uTransformEventInfo(): input(0), value(0), needsRepeat(false){}
    uTransformEventInfo( uInput * input_, float value_ = 1 , bool needsRepeat_ = true ): input(input_), value(value_), needsRepeat(needsRepeat_){}
};

// transform SDL event into vector of abstract events
#ifndef DEDICATED
static void su_TransformEvent( SDL_Event & e, std::vector< uTransformEventInfo > & info )
{
    switch (e.type)
    {
    case SDL_MOUSEMOTION:
        if (!su_enableTouch)
        {
            // ignore events generated by mouse grabbing
            if ( su_mouseGrab &&
                    e.motion.x==sr_screenWidth/2 && e.motion.x==sr_screenHeight/2)
            {
                return;
            }

            // we generate up to four events per mouse movement
            info.reserve(4);

            REAL xrel=e.motion.xrel;
            if (xrel > 0) // right
            {
                info.push_back( uTransformEventInfo(
                                    su_GetMouseInput().x_minus,
                                    0,
                                    false ) );
                info.push_back( uTransformEventInfo(
                                    su_GetMouseInput().x_plus,
                                    xrel * mouse_sensitivity,
                                    false ) );
            }

            if (xrel < 0) // left
            {
                info.push_back( uTransformEventInfo(
                                    su_GetMouseInput().x_plus,
                                    0,
                                    false ) );
                info.push_back( uTransformEventInfo(
                                    su_GetMouseInput().x_minus,
                                    -xrel * mouse_sensitivity,
                                    false ) );
            }

            // invert mouse movement
            REAL yrel=-e.motion.yrel;
            if (yrel>0) // up
            {
                info.push_back( uTransformEventInfo(
                                    su_GetMouseInput().y_minus,
                                    0,
                                    false ) );
                info.push_back( uTransformEventInfo(
                                    su_GetMouseInput().y_plus,
                                    yrel * mouse_sensitivity,
                                    false ) );
            }
            if (yrel<0) // down
            {
                info.push_back( uTransformEventInfo(
                                    su_GetMouseInput().y_plus,
                                    0,
                                    false ) );
                info.push_back( uTransformEventInfo(
                                    su_GetMouseInput().y_minus,
                                    -yrel * mouse_sensitivity,
                                    false ) );
            }
        }
        break;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
        {
            int button=e.button.button;
            if (button<=MOUSE_BUTTONS)
            {
                info.push_back( uTransformEventInfo(
                                    su_GetMouseInput().button[ button ],
                                    ( e.type == SDL_MOUSEBUTTONDOWN ) ? 1 : 0 ) );
            }
        }
        break;
#if SDL_VERSION_ATLEAST(2,0,0)
    case SDL_FINGERDOWN:
    case SDL_FINGERUP:
    case SDL_FINGERMOTION:
        if (su_enableTouch==1)
        {
            static SDL_FingerID finger = 0;

            if (e.type == SDL_FINGERDOWN) {
                float px = e.tfinger.x;
                if (px<0.33)      info.push_back( uTransformEventInfo( su_GetTouchInput().turnLeft, 1 ) );
                else if (px>0.67) info.push_back( uTransformEventInfo( su_GetTouchInput().turnRight, 1 ) );
                else if (!finger) {
                    finger = e.tfinger.fingerId;
                    info.push_back( uTransformEventInfo( su_GetTouchInput().brake, 1 ) );
                }
            } else if (e.type == SDL_FINGERUP) {
                if (finger == e.tfinger.fingerId) {
                    finger = 0;
                    info.push_back( uTransformEventInfo( su_GetTouchInput().brake, 0 ) );
                }
            }
        }
        else if (su_enableTouch==2)
        {
            static SDL_FingerID finger = 0;
            static float dx=0, dy=0;
            static char count = 0;
            static int previous_dir = -1;
            static int last_time = 0;
            static const int axes = 4;      // to be replaced by actual real axes number...

            if (e.type == SDL_FINGERDOWN) {
                // if new finger tracking, reset current state
                if (!finger) {
                    finger = e.tfinger.fingerId;
                    count = 0;
                    dx = 0; dy = 0;
                    last_time = e.tfinger.timestamp;
                    previous_dir = -1;
                }
            } else if (e.type == SDL_FINGERUP) {
                // release finger tracking if tracked finger is up
                if (finger == e.tfinger.fingerId) finger = 0;
            } else if (e.type == SDL_FINGERMOTION) {
                // if this finger is tracked, process touch motion analysis
                if (finger == e.tfinger.fingerId) {
                    // check motion timeout reset current state
                    if (e.tfinger.timestamp-last_time>50) {
                        count = 0;
                        dx = 0; dy = 0;
                        last_time = e.tfinger.timestamp;
                    }
                    last_time = e.tfinger.timestamp;
                        
                    // cumulate some motion to be more accurate
                    dx+=e.tfinger.dx;
                    dy+=e.tfinger.dy;
                    ++count;
                    
                    if (count==4) {
                        // if move is big enough, process it
                        if (dx*dx+dy*dy>0.0001) {
                            float a = atan2f(dx, -dy);
                            a = (a<0?fabs(a+M_PI*2.0f):a) * axes / (M_PI*2.0f);
                            int dir = static_cast<int>(round(a)) % axes; // direction of this move
                            int side = (a-round(a))<0?0:1;               // from which side
                            // if direction changed, generate 1 or more turns
                            if (previous_dir!=dir) {
                                int diff_dir = previous_dir==-1 ? dir : dir - previous_dir;
                                diff_dir = diff_dir>axes/2 ? diff_dir - axes : diff_dir<-axes/2 ? axes + diff_dir : diff_dir;
                                // handling the annoying case where you're trying to do a u-turn.
                                // we need to take care from which side we are coming from (really dangerous move as
                                // detection might failed).
                                if (diff_dir!=0) {
                                    if ((diff_dir==axes/2 && side==1)||(diff_dir==-axes/2 && side==0)) diff_dir=-diff_dir;
                                    uInput* input = diff_dir<0 ? su_GetTouchInput().turnLeft : su_GetTouchInput().turnRight;
                                    for(int i=abs(diff_dir); i==1; --i) {
                                        info.push_back( uTransformEventInfo( input, 1 ) );
                                    }
                                }
                                previous_dir=dir;
                            }
                        }
                        count = 0;
                        dx = 0; dy = 0;
                    }
                }
            }
        }
        break;
#endif
    case SDL_KEYDOWN:
    case SDL_KEYUP:
        {
#if SDL_VERSION_ATLEAST(2,0,0)
            SDL_Keysym &c = e.key.keysym;
#else
            SDL_keysym &c = e.key.keysym;
#endif

            info.push_back( uTransformEventInfo(
#if SDL_VERSION_ATLEAST(2,0,0)
                                su_GetKeyInput().sdl_keys[ c.scancode ],
#else
                                su_GetKeyInput().sdl_keys[ c.sym ],
#endif
                                ( e.type == SDL_KEYDOWN ) ? 1 : 0 ) );

            break;
        }
#ifndef NOJOYSTICK
    case SDL_JOYAXISMOTION:
        {
            uJoystick * joystick = su_GetJoystick( e.jaxis.which );
            if(!joystick) break;
            int dir = e.jaxis.value > 0 ? 1 : 0;

            info.push_back( uTransformEventInfo(
                                joystick->GetAxis( e.jaxis.axis, 1-dir ),
                                0 ) );
            info.push_back( uTransformEventInfo(
                                joystick->GetAxis( e.jaxis.axis, dir ),
                                fabs( e.jaxis.value )/32768 ) );

            break;
        }
    case SDL_JOYBUTTONDOWN:
    case SDL_JOYBUTTONUP:
    {
        uJoystick * joystick = su_GetJoystick( e.jbutton.which );
        if(!joystick) break;
        info.push_back( uTransformEventInfo(
                            joystick->GetButton( e.jbutton.button ),
                            ( e.type == SDL_JOYBUTTONDOWN ) ? 1 : 0 ) );
    }
    break;
    case SDL_JOYHATMOTION:
        {
            info.reserve(4);

            uJoystick * joystick = su_GetJoystick( e.jhat.which );
            if(!joystick) break;
            int hat = e.jhat.hat;
            int hatDirection = e.jhat.value;

            // left/right hat motion
            {
                int & lastDir = joystick->GetHatDirection( hat, 0 );
                int newDir =
                    ( ( hatDirection & SDL_HAT_LEFT ) ? -1 : 0 ) +
                    ( ( hatDirection & SDL_HAT_RIGHT ) ? +1 : 0 );

                // negate previous events
                if ( lastDir < 0 && newDir >= 0 )
                {
                    info.push_back( uTransformEventInfo(
                                        joystick->GetHat( hat, uJoystick::Left ),
                                        0 ) );
                }

                if ( lastDir > 0 && newDir <= 0 )
                {
                    info.push_back( uTransformEventInfo(
                                        joystick->GetHat( hat, uJoystick::Right ),
                                        0 ) );
                }

                // create new events
                if ( lastDir >= 0 && newDir < 0 )
                {
                    info.push_back( uTransformEventInfo(
                                        joystick->GetHat( hat, uJoystick::Left ),
                                        1 ) );
                }

                if ( lastDir <= 0 && newDir > 0 )
                {
                    info.push_back( uTransformEventInfo(
                                        joystick->GetHat( hat, uJoystick::Right ),
                                        1 ) );
                }

                lastDir = newDir;
            }

            // up/down hat motion
            {
                int & lastDir = joystick->GetHatDirection( hat, 1 );
                int newDir =
                    ( ( hatDirection & SDL_HAT_UP ) ? -1 : 0 ) +
                    ( ( hatDirection & SDL_HAT_DOWN ) ? +1 : 0 );

                // negate previous events
                if ( lastDir < 0 && newDir >= 0 )
                {
                    info.push_back( uTransformEventInfo(
                                        joystick->GetHat( hat, uJoystick::Up ),
                                        0 ) );
                }

                if ( lastDir > 0 && newDir <= 0 )
                {
                    info.push_back( uTransformEventInfo(
                                        joystick->GetHat( hat, uJoystick::Down ),
                                        0 ) );
                }

                // create new events
                if ( lastDir >= 0 && newDir < 0 )
                {
                    info.push_back( uTransformEventInfo(
                                        joystick->GetHat( hat, uJoystick::Up ),
                                        1 ) );
                }

                if ( lastDir <= 0 && newDir > 0 )
                {
                    info.push_back( uTransformEventInfo(
                                        joystick->GetHat( hat, uJoystick::Down ),
                                        1 ) );
                }

                lastDir = newDir;
            }
        }
        break;
    case SDL_JOYBALLMOTION:
        {
            uJoystick * joystick = su_GetJoystick( e.jball.which );
            if(!joystick) break;

            int ball = e.jball.ball;
            info.reserve(4);


            REAL xrel=e.jball.xrel;
            if (xrel > 0) // right
            {
                info.push_back( uTransformEventInfo(
                                    joystick->GetBall( ball, uJoystick::Left ),
                                    0,
                                    false ) );
                info.push_back( uTransformEventInfo(
                                    joystick->GetBall( ball, uJoystick::Right ),
                                    xrel,
                                    false ) );
            }

            if (xrel < 0) // left
            {
                info.push_back( uTransformEventInfo(
                                    joystick->GetBall( ball, uJoystick::Right ),
                                    0,
                                    false ) );
                info.push_back( uTransformEventInfo(
                                    joystick->GetBall( ball, uJoystick::Left ),
                                    -xrel,
                                    false ) );
            }

            // invert ball movement
            REAL yrel=-e.jball.yrel;
            if (yrel>0) // up
            {
                info.push_back( uTransformEventInfo(
                                    joystick->GetBall( ball, uJoystick::Down ),
                                    0,
                                    false ) );
                info.push_back( uTransformEventInfo(
                                    joystick->GetBall( ball, uJoystick::Up ),
                                    yrel,
                                    false ) );
            }
            if (yrel<0) // down
            {
                info.push_back( uTransformEventInfo(
                                    joystick->GetBall( ball, uJoystick::Up ),
                                    0,
                                    false ) );
                info.push_back( uTransformEventInfo(
                                    joystick->GetBall( ball, uJoystick::Down ),
                                    -yrel,
                                    false ) );
            }
        }
        break;
#endif
    default:
        break;
    }
}
#endif // DEDICATED


namespace
{
class Input_Comparator
{
public:
    static int Compare( const uAction* a, const uAction* b )
    {
        if ( a->priority < b->priority )
            return 1;
        else if ( a->priority > b->priority )
            return -1;
        return tString::CompareAlphaNumerical( a->internalName, b->internalName );
    }
};
}

static int Input_Compare( const tListItemBase* a, const tListItemBase* b)

{
    return Input_Comparator::Compare( (const uAction*)a, (const uAction*)b );
}


static void s_InputConfigGeneric(int ePlayer, uAction *&actions,const tOutput &title){
    uMenu input_menu(title);

    actions->tListItemBase::Sort(&Input_Compare);

    std::vector< uMenuItemInput * > inputs;
    for ( uAction *a = actions; a; a = a->Next() )
    {
        if ( a->ShowInGenericConfigurationMenu() )
            inputs.push_back( new uMenuItemInput( &input_menu, a, ePlayer + 1 ) );
    }

    input_menu.ReverseItems();
    input_menu.Enter();

    for ( std::vector< uMenuItemInput * >::const_iterator it = inputs.begin(); it != inputs.end(); ++it )
        delete *it;
}

void su_InputConfig(int ePlayer){

    tOutput name;
    name.SetTemplateParameter(1, ePlayer+1);
    name.SetTemplateParameter(2, uPlayerPrototype::PlayerConfig(ePlayer)->Name());
    name << "$input_for_player";

    s_InputConfigGeneric(ePlayer,s_playerActions,name);
}

void su_InputConfigCamera(int player){

    tOutput name;
    name.SetTemplateParameter(1, uPlayerPrototype::PlayerConfig(player)->Name());
    name << "$camera_controls";

    s_InputConfigGeneric(player,s_cameraActions,name);
}

void su_InputConfigGlobal(){
    s_InputConfigGeneric(-1,s_globalActions,"$input_items_global");
}


REAL key_sensitivity=40;
static double lastTime=0;
static REAL ts=0;

static bool su_delayed = false;

void su_HandleDelayedEvents ()
{
    // nothing to do
    if ( !su_delayed )
    {
        return;
    }

    su_delayed = false;

    for ( uInputs::const_iterator i = su_inputs.begin(); i != su_inputs.end(); ++i )
    {
        uBind * bind = (*i)->GetBind();
        if ( bind )
        {
            bind->HanldeDelayed();
        }
    }
}

bool su_HandleEvent(SDL_Event &e, bool delayed ){
#ifndef DEDICATED
    if ( su_delayed && !delayed )
    {
        su_HandleDelayedEvents();
    }

    su_delayed = delayed;

    // transform events
    bool ret = false;

    std::vector< uTransformEventInfo > events;
    su_TransformEvent( e, events );

    for ( std::vector< uTransformEventInfo >::const_iterator i = events.begin(); i != events.end(); ++i )
    {
        uTransformEventInfo info = *i;

        if ( info.input )
        {
            // store input state for later repeat
            if ( info.needsRepeat )
            {
                info.input->SetPressed( info.value > 0 ? info.value : 0 );
            }

            // activate binding
            uBind * bind = info.input->GetBind();
            if ( bind )
            {
                if ( info.value > 0 && bind->IsDoubleBind( info.input ) )
                {
                    return true;
                }

                if ( info.needsRepeat && bind->act && bind->act->type == uAction::uINPUT_ANALOG )
                {
                    info.value *= ts*key_sensitivity;
                }

                ret |= bind->Activate( info.value, delayed );
            }
        }
    }

    return ret;
#endif
    return false;
}

void su_InputSync(){
    double time=tSysTimeFloat();
    ts=REAL(time-lastTime);

    //static REAL tsSmooth=0;
    //tsSmooth+=REAL(ts*.1);
    //tsSmooth/=REAL(1.1);
    lastTime=time;

    // key repeat
    for ( uInputs::const_iterator i = su_inputs.begin(); i != su_inputs.end(); ++i )
    {
        uInput * input = *i;
        uBind * bind = input->GetBind();
        if ( input->GetPressed() > 0 && bind &&
                bind->act && bind->act->type == uAction::uINPUT_ANALOG )
        {
            bind->Activate( ts * key_sensitivity * input->GetPressed(), su_delayed );
        }
    }
}

void su_ClearKeys()
{
    // clears stored keypresses
    for ( uInputs::const_iterator i = su_inputs.begin(); i != su_inputs.end(); ++i )
    {
        uInput * input = *i;
        uBind * bind = input->GetBind();
        if ( input->GetPressed() > 0 && bind )
        {
            bind->Activate( -1, su_delayed );
            input->SetPressed( 0 );
        }
    }
}

// *****************
// Player binds
// *****************

static char const * Player_keyword="PLAYER_BIND";

uBindPlayer::uBindPlayer(uAction *a,int p):uBind(a),ePlayer(p){}

uBindPlayer::~uBindPlayer(){}

uBindPlayer * uBindPlayer::NewBind(std::istream &s)
{
    // read action
    std::string actionName;
    s >> actionName;
    uAction * act = uAction::Find( actionName.c_str() );

    // read player ID
    int player;
    s >> player;

    // delegate
    return NewBind( act, player );
}

uBindPlayer * uBindPlayer::NewBind( uAction * action, int player )
{
    // see if the bind has an alias
    for ( uInputs::const_iterator i = su_inputs.begin(); i != su_inputs.end(); ++i )
    {
        uInput * input = *i;
        uBind * old = input->GetBind();

        // compare action
        if ( old && old->act == action )
        {
            uBindPlayer * oldPlayer = dynamic_cast< uBindPlayer * >( old );
            if ( oldPlayer && oldPlayer->ePlayer == player )
                return oldPlayer;
        }
    }

    // no alias found, return new bind
    return tNEW(uBindPlayer)( action, player );
}

bool uBindPlayer::IsKeyWord(const char *n){
    return !strcmp(n,Player_keyword);
}

bool uBindPlayer::CheckPlayer(int p){
    return p==ePlayer;
}

void uBindPlayer::Write(std::ostream &s){
    s << Player_keyword << '\t';
    uBind::Write(s);
    s << ePlayer;
}

bool uBindPlayer::Delayable()
{
    return ( ePlayer!=0 );
}

bool uBindPlayer::DoActivate(REAL x){
    bool ret = false;
    if (ePlayer==0)
        ret = GlobalAct(act,x);
    else
        ret = uPlayerPrototype::PlayerConfig(ePlayer-1)->Act(act,x);

    if( ret && act && act->GetTooltip() && x > 0 )
    {
        act->GetTooltip()->Count(ePlayer);
    }
    
    return ret;
}


// *****************
// Global actions
// *****************

static uActionGlobalFunc *uActionGlobal_anchor=NULL;

uActionGlobalFunc::uActionGlobalFunc(uActionGlobal *a, ACTION_FUNC *f,
                                     bool rebind )
        :tListItem<uActionGlobalFunc>(uActionGlobal_anchor), func (f), act(a),
rebindable(rebind){}

bool uActionGlobalFunc::IsBreakingGlobalBind(uAction *act){
    for (uActionGlobalFunc *run = uActionGlobal_anchor; run ; run = run->Next())
        if (run->act == act && !run->rebindable)
            return true;

    return false;
}

bool uActionGlobalFunc::GlobalAct(uAction *act, REAL x){
    for (uActionGlobalFunc *run = uActionGlobal_anchor; run ; run = run->Next())
        if (run->act == act && run->func(x))
            return true;

    return false;
}

static uActionGlobal mess_up("MESS_UP",1);

static uActionGlobal mess_down("MESS_DOWN",2);

static uActionGlobal mess_end("MESS_END",3);

static bool messup_func(REAL x){
    if (x>0){
        sr_con.Scroll(-1);
    }
    return true;
}

static bool messdown_func(REAL x){
    if (x>0){
        sr_con.Scroll(1);
    }
    return true;
}

static bool messend_func(REAL x){
    if (x>0){
        sr_con.End(2);
    }
    return true;
}

static uActionGlobalFunc mu(&mess_up,&messup_func);
static uActionGlobalFunc md(&mess_down,&messdown_func);
static uActionGlobalFunc me(&mess_end,&messend_func);

// ********
// tooltips
// ********

uActionTooltip::Level su_helpLevel = uActionTooltip::Level_Expert;

uActionTooltip::uActionTooltip( Level level, uAction & action, int numHelp, VETOFUNC * veto )
: tConfItemBase(action.internalName + "_TOOLTIP")
  , action_( action )
  , veto_(veto)
  , level_( level )
{
    help_ = tString("$input_") + action.internalName + "_tooltip";
    tToLower( help_ );

    // initialize array holding the number of help attempts to give left
    for( int i = uMAX_PLAYERS; i >= 0; --i )
    {
        activationsLeft_[i] = 0; // numHelp;
    }

    action.tooltip_ = this;
}

uActionTooltip::~uActionTooltip()
{
    if( action_.tooltip_ == this )
        action_.tooltip_ = NULL;
        
}

bool uActionTooltip::Help( int player )
{
#ifndef DEDICATED
    if( rConsole::CenterDisplayActive() )
    {
        return false;
    }

    // find most needed tooltip
    uActionTooltip * mostWanted = NULL;

    // keys bound to the action of the tooltip that needs help
    tString maps;
    tString last;

    // run through binds
    for ( uInputs::const_iterator i = su_inputs.begin(); i != su_inputs.end(); ++i )
    {
        uBind * bind = (*i)->GetBind();
        if( !bind ||!bind->CheckPlayer(player) )
            continue;
        uAction * action = bind->act;
        if( !action )
            continue;
        uActionTooltip * tooltip = action->GetTooltip();
        if( !tooltip || ( tooltip->veto_ && (*tooltip->veto_)(player) ) || ( su_helpLevel < tooltip->level_ ) )
        {
            continue;
        }
        
        int activationsLeft = tooltip->activationsLeft_[player];
        if( activationsLeft > 0 && 
            ( !mostWanted || mostWanted->activationsLeft_[player] < activationsLeft ) )
        {
            mostWanted = tooltip;
            maps = "";
            last = "";
        }

        // build up key list
        if( mostWanted == tooltip )
        {
            if ( maps.Len() > 1 )
            {
                maps << ", ";
            }
            if ( last.Len() > 1 )
            {
                maps << last;
            }
            last = tString("<") + (*i)->Name() + ">";
        }
    }

    if( mostWanted )
    {
        if( last.Len() > 1 )
        {
            if( maps.Len() > 1 )
                maps << " " << tOutput("$input_or") << " " << last;
            else
                maps = last;
        }

        con.CenterDisplay(tString(tOutput(mostWanted->help_, maps)));

        return true;
    }
#endif
    return false;
}

void uActionTooltip::Count( int player )
{
    if ( activationsLeft_[player] > 0 )
    {
        activationsLeft_[player]--;
        Help(player);
    }
}

//! call to show the tooltip one more time
void uActionTooltip::ShowAgain()
{
    for( int i = uMAX_PLAYERS; i >= 0; --i )
    {
        if( 0 == activationsLeft_[i] )
        {
            activationsLeft_[i] = 1;
        }
    }
}

void uActionTooltip::WriteVal(std::ostream & s )
{
    for( int i = 0; i <= uMAX_PLAYERS; ++i )
    {
        s << activationsLeft_[i] << " ";
    }
}

void uActionTooltip::ReadVal(std::istream & s )
{
    for( int i = 0; i <= uMAX_PLAYERS; ++i )
    {
        s >> activationsLeft_[i];
    }
}

// *****************************************************
//  Menuitem for input selection
// *****************************************************

uMenuItemInput::uMenuItemInput(uMenu *M,uAction *a,int p)
    :uMenuItem(M,a->helpText),act(a),ePlayer(p),active(0)
{
}

void uMenuItemInput::Render(REAL x,REAL y,REAL alpha,bool selected)
{
    DisplayText(REAL(x-.02),y,act->description,selected,alpha,1);

    if (active)
    {
        tString s;
        s << tOutput("$input_press_any_key");
        DisplayText(REAL(x+.02),y,s,selected,alpha,-1);
    }
    else
    {
        tString s;

        bool first=1;

        for ( uInputs::const_iterator i = su_inputs.begin(); i != su_inputs.end(); ++i )
        {
            uBind * bind = (*i)->GetBind();
            if ( bind && (*i)->Name().size() > 0 &&
                    bind->act==act &&
                    bind->CheckPlayer(ePlayer) )
            {
                if (!first)
                    s << ", ";
                else
                    first=0;

                s << (*i)->Name();
            }
        }
        if (!first)
        {
            DisplayText(REAL(x+.02),y,s,selected,alpha,-1);
        }
        else
        {
            DisplayText(REAL(x+.02),y,tOutput("$input_items_unbound"),selected,alpha,-1);
        }
    }
}

void uMenuItemInput::Enter()
{
    active=1;
}


bool uMenuItemInput::Event(SDL_Event &e)
{
#ifndef DEDICATED
    if ( e.type == SDL_KEYDOWN )
    {
#if SDL_VERSION_ATLEAST(2,0,0)
        SDL_Keysym &c = e.key.keysym;
#else
        SDL_keysym &c = e.key.keysym;
#endif
        if (!active)
        {
            if (c.sym==SDLK_DELETE || c.sym==SDLK_BACKSPACE)
            {
                // clear all bindings
                for ( uInputs::const_iterator i = su_inputs.begin(); i != su_inputs.end(); ++i )
                {
                    uBind * bind = (*i)->GetBind();
                    if ( bind &&
                            bind->act==act &&
                            bind->CheckPlayer(ePlayer) )
                    {
                        (*i)->SetBind( NULL );
                    }
                }
                return true;
            }
            return false;
        }

        // ignore escape
        if ( c.sym == SDLK_ESCAPE )
        {
            return false;
        }
    }

    // transform events
    std::vector< uTransformEventInfo > events;
    su_TransformEvent( e, events );

    for ( std::vector< uTransformEventInfo >::const_iterator i = events.begin(); i != events.end(); ++i )
    {
        uTransformEventInfo const & info = *i;
        if ( info.input && info.value > 0.5 && active )
        {
            uBind * bind = info.input->GetBind();
            if ( bind &&
                    bind->act==act &&
                    bind->CheckPlayer(ePlayer))
            {
                info.input->SetBind( NULL );
            }
            else
            {
                info.input->SetBind( uBindPlayer::NewBind(act,ePlayer) );
            }

            active = false;
            return true;
        }
    }
#endif
    return false;
}
