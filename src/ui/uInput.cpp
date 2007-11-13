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

bool su_mouseGrab = false;

static uAction* su_allActions[uMAX_ACTIONS];
static int     su_allActionsLen = 0;

static int sdlk_dynlast = SDLK_NEWLAST;

#ifndef DEDICATED
static const char *sdlk_dynnames[SDLK_MAX - SDLK_NEWLAST];
#endif

#ifndef NOJOYSTICK
#define JOYSTICK_MAX 16
static int joystick_sdlk[JOYSTICK_MAX][4];
#endif

uAction::uAction(uAction *&anchor,const char* name,
                 int priority_,
                 uInputType t)
        :tListItem<uAction>(anchor),type(t),priority(priority_),internalName(name){
    globalID = localID = su_allActionsLen++;

    int i;

    tASSERT(localID < uMAX_ACTIONS);

    su_allActions[localID] = this;

    tString descname;
    descname << "input_" << name << "_text";
    for(i=descname.Size()-1;i>=0;i--)
        descname[i]=tolower(descname[i]);

    const_cast<tOutput&>(description).AddLocale(descname);

    tString helpname;
    helpname << "input_" << name << "_help";
    for(i=helpname.Size()-1;i>=0;i--)
        helpname[i]=tolower(helpname[i]);

    const_cast<tOutput&>(helpText).AddLocale(helpname);
}

uAction::uAction(uAction *&anchor,const char* name,
                 const tOutput& desc,
                 const tOutput& help,
                 int priority_,
                 uInputType t)
        :tListItem<uAction>(anchor),type(t),priority(priority_),internalName(name), description(desc), helpText(help){
    globalID = localID = su_allActionsLen++;

    tASSERT(localID < uMAX_ACTIONS);

    su_allActions[localID] = this;
}

uAction::~uAction(){
    su_allActions[localID] = NULL;
}

uAction * uAction::Find( char const * name )
{
    for(int i=su_allActionsLen-1;i>=0;i--)
        if(!strcmp(name,su_allActions[i]->internalName))
            return su_allActions[i];

    return 0;
}

// ***************************
//    the generic keymaps
// ***************************

tJUST_CONTROLLED_PTR< uBind > keymap[SDLK_MAX];
bool                          pressed[SDLK_MAX];

// ****************************************
// a configuration class for keyboard binds
// ****************************************

class tConfItem_key:public tConfItemBase{
public:
    tConfItem_key():tConfItemBase("KEYBOARD"){}
    ~tConfItem_key(){};

    // write the complete keymap
    virtual void WriteVal(std::ostream &s){
        int first=1;
        for(int keysym=sdlk_dynlast-1;keysym>=0;keysym--){
            if (keymap[keysym]){

                if (!first)
                    s << "\nKEYBOARD\t";
                else
                    first=0;

                s << keysym << '\t';
                keymap[keysym]->Write(s);
            }
        }
        if (first)
            s << "-1";
    }

    // read one keybind
    virtual void ReadVal(std::istream &s){
        tString in;
        int keysym;
        s >> keysym;
        if (keysym>=0){
            // ignore invalid keysyms
#ifndef DEDICATED
            tASSERT( keysym < sdlk_dynlast);
#endif
            if (keysym >= sdlk_dynlast)
                return;
            s >> in;
            if (uBindPlayer::IsKeyWord(in))
            {
                keymap[keysym] = NULL;
                keymap[keysym]=uBindPlayer::NewBind(s);
                if (!keymap[keysym]->act)
                {
                    keymap[keysym]=NULL;
                }
                /* if (global_bind::IsKeyWord(in))
                   keymap[keysym]=new global_bind(s); */
            }
        }
        char c=' ';
        while(c!='\n' && s.good() && !s.eof()) c=s.get();
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
    return x.globalID == globalID;}

uActionPlayer *uActionPlayer::Find(int id){
    uAction *run = s_playerActions;

    while (run){
        if (run->ID() == id)
            return static_cast<uActionPlayer*>(run);
    }

    return NULL;
}


uActionCamera::uActionCamera(const char *name,
                             int priority,
                             uInputType t)
    :uAction(s_cameraActions,name,priority,t){}

uActionCamera::~uActionCamera(){}

bool uActionCamera::operator==(const uActionCamera &x){
    return x.globalID == globalID;}


// global actions
uActionGlobal::uActionGlobal(const char *name,
                             int priority,
                             uInputType t)
        :uAction(s_globalActions,name,priority,t){}

uActionGlobal::~uActionGlobal(){}

bool uActionGlobal::operator==(const uActionGlobal &x){
    return x.globalID == globalID;}

bool uActionGlobal::IsBreakingGlobalBind(int sym){
    if (!keymap[sym])
        return false;
    uAction *act=keymap[sym]->act;
    if (!act)
        return false;

    return uActionGlobalFunc::IsBreakingGlobalBind(act);
}

static void keyboard_init(){
    for(int i=0;i<sdlk_dynlast;i++)
        keymap[i]=NULL;
    //uBindPlayer::Init();
    //global_bind::Init();
}

static void keyboard_exit(){
    for(int i=0;i<sdlk_dynlast;i++)
        keymap[i] = 0;
    //uBindPlayer::Init();
    //global_bind::Init();
}

#ifndef NOJOYSTICK
static int add_sdlk(const char *desc, ...) {
    char *mycopy;
    va_list va;
    int mclen;

    if (sdlk_dynlast == SDLK_MAX)
        return -1; /* Too many already! */

    va_start(va, desc);
    mycopy = (char *)malloc(mclen = 1 + vsnprintf(NULL, 0, desc, va));
    va_end(va);
    va_start(va, desc);
    vsnprintf(mycopy, mclen, desc, va);
    va_end(va);
    sdlk_dynnames[sdlk_dynlast - SDLK_NEWLAST] = mycopy;
    return sdlk_dynlast++;
}

void su_JoystickInit() {
    int joycount = SDL_NumJoysticks(), i, j, na;
    SDL_Joystick *cur;

    if (joycount > JOYSTICK_MAX)
        joycount = JOYSTICK_MAX;

    SDL_JoystickEventState(SDL_ENABLE);
    for (i = 0; i < joycount; ++i) {
        cur = SDL_JoystickOpen(i);
        na = SDL_JoystickNumAxes(cur);
        joystick_sdlk[i][0] = sdlk_dynlast;
        for (j = 0; j < na; ++j) {
            add_sdlk("Joystick %d Axis %d Decrease", i, j);
            add_sdlk("Joystick %d Axis %d Increase", i, j);
        }
        na = SDL_JoystickNumButtons(cur);
        joystick_sdlk[i][1] = sdlk_dynlast;
        for (j = 0; j < na; ++j) {
            add_sdlk("Joystick %d Button %d", i, j);
        }
        /* TODO: Balls, Hats, Buttons */
    }
#ifdef DEBUG
    std::cout << joycount << " joystick" << (joycount == 1 ? "" : "s") << " initialized\n";
#endif
}
#endif

static tInitExit keyboard_ie(&keyboard_init, &keyboard_exit);

// *********************************************
// generic keypress/mouse movement binding class
// *********************************************

uBind::~uBind(){}

uBind::uBind(uAction *a ):lastValue_(0), delayedValue_(0), lastSym_(-1), lastTime_(-1), act(a){}

uBind::uBind(std::istream &s): lastValue_(0), delayedValue_(0), lastSym_(-1), lastTime_(-1), act(NULL)
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

bool uBind::IsDoubleBind( int sym )
{
    double currentTime = tSysTimeFloat();

    // if a different key was used for this action a short while ago, give alarm.
    bool ret = ( su_doubleBindTimeout > 0 && sym != lastSym_ && currentTime - lastTime_ < su_doubleBindTimeout );

    // store last usage
    lastSym_ = sym;
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
        for(int i=uMAX_PLAYERS-1; i >=0; i--)
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

int uPlayerPrototype::Num(){return nextid;}

// *******************
// Input configuration
// *******************


// *****************************************************
//  Menuitem for input selection
// *****************************************************

static const char *keyname(int sym){
#ifndef DEDICATED
    if (sym<=SDLK_LAST)
        return SDL_GetKeyName(static_cast<SDLKey>(sym));
    else if (sym > sdlk_dynlast)
        return "Unknown";
    else switch (sym){
        case SDLK_MOUSE_X_PLUS: return "Mouse right";
        case SDLK_MOUSE_X_MINUS: return "Mouse left";
        case SDLK_MOUSE_Y_PLUS: return "Mouse up";
        case SDLK_MOUSE_Y_MINUS: return "Mouse down";
        case SDLK_MOUSE_Z_PLUS: return "Mouse z up";
        case SDLK_MOUSE_Z_MINUS: return "Mouse z down";
        case SDLK_MOUSE_BUTTON_1: return "Mousebutton 1";
        case SDLK_MOUSE_BUTTON_2: return "Mousebutton 2";
        case SDLK_MOUSE_BUTTON_3: return "Mousebutton 3";
        case SDLK_MOUSE_BUTTON_4: return "Mousebutton 4";
        case SDLK_MOUSE_BUTTON_5: return "Mousebutton 5";
        case SDLK_MOUSE_BUTTON_6: return "Mousebutton 6";
        case SDLK_MOUSE_BUTTON_7: return "Mousebutton 7";
        default: return sdlk_dynnames[sym - SDLK_NEWLAST];
        }
#endif
    return "";
}

class uMenuItemInput: uMenuItem{
    uAction      *act;
    int         ePlayer;
    bool        active;
public:
    uMenuItemInput(uMenu *M,uAction *a,int p)
            :uMenuItem(M,a->helpText),act(a),ePlayer(p),active(0){
    }

    virtual ~uMenuItemInput(){}

    virtual void Render(REAL x,REAL y,REAL alpha=1,bool selected=0){
        DisplayText(REAL(x-.02),y,act->description,selected,alpha,1);

        if (active)
        {
            tString s;
            s << tOutput("$input_press_any_key");
            DisplayText(REAL(x+.02),y,s,selected,alpha,-1);
        }
        else{
            tString s;

            bool first=1;

            for(int keysym=sdlk_dynlast-1;keysym>=0;keysym--)
                if(keymap[keysym] &&
                        keymap[keysym]->act==act &&
                        keymap[keysym]->CheckPlayer(ePlayer)){
                    if (!first)
                        s << ", ";
                    else
                        first=0;

                    s << keyname(keysym);
                }
            if(!first)
                DisplayText(REAL(x+.02),y,s,selected,alpha,-1);
            else
                DisplayText(REAL(x+.02),y,tOutput("$input_items_unbound"),selected,alpha,-1);
        }
    }

    virtual void Enter(){
        active=1;
    }

#define MTHRESH 5
#define MREL    2

#ifndef DEDICATED

    virtual bool Event(SDL_Event &e){
        int sym=-1;
        switch (e.type){
        case SDL_MOUSEMOTION:
            if(active){
                REAL xrel=e.motion.xrel;
                REAL yrel=-e.motion.yrel;

                if (fabs(xrel)>MREL*fabs(yrel)){ // x motion
                    if (xrel>MTHRESH) // left
                        sym=SDLK_MOUSE_X_PLUS;
                    if (xrel<-MTHRESH) // left
                        sym=SDLK_MOUSE_X_MINUS;
                }

                if (fabs(yrel)>MREL*fabs(xrel)){ // x motion
                    if (yrel>MTHRESH) // left
                        sym=SDLK_MOUSE_Y_PLUS;
                    if (yrel<-MTHRESH) // left
                        sym=SDLK_MOUSE_Y_MINUS;
                }

                if (sym>0)
                    active=0;
            }

            break;
        case SDL_MOUSEBUTTONDOWN:
            if(active){
                int button=e.button.button;
                if (button<=MOUSE_BUTTONS)
                    sym=SDLK_MOUSE_BUTTON_1+button-1;

                active=0;
            }
            break;

        case SDL_KEYDOWN:{
                SDL_keysym &c=e.key.keysym;
                if(!active){
                    if (c.sym==SDLK_DELETE || c.sym==SDLK_BACKSPACE)
                    {
                        for(int keysym=sdlk_dynlast-1;keysym>=0;keysym--)
                            if(keymap[keysym] &&
                                    keymap[keysym]->act==act &&
                                    keymap[keysym]->CheckPlayer(ePlayer)){
                                keymap[keysym]=NULL;
                            }
                        return true;
                    }
                    return false;
                }

                active=0;

                if (c.sym!=SDLK_ESCAPE)
                    sym=c.sym;
                else
                    return true;
            }
            break;
#ifndef NOJOYSTICK
        case SDL_JOYAXISMOTION:
            if (e.jaxis.value > -16384 && e.jaxis.value < 16384) {
#if DEBUG
                std::cout << "JS: Ignoring value of " << e.jaxis.value << "\n";
#endif
                return false;
            }
            sym = joystick_sdlk[e.jaxis.which][0] + (e.jaxis.axis * 2) + ((e.jaxis.value > 0) ? 1 : 0);
            active = 0;
#if DEBUG
            printf("JS: %d,%d,%d is %s\n", e.jaxis.which, e.jaxis.axis, e.jaxis.value, keyname(sym));
            fflush(stdout);
#endif
            break;
        case SDL_JOYBUTTONDOWN:
            sym = joystick_sdlk[e.jbutton.which][1] + e.jbutton.button;
            active = 0;
            break;
#endif
        default:
            return(false);
        }

        if(sym>=0){
            if(keymap[sym] &&
                    keymap[sym]->act==act &&
                    keymap[sym]->CheckPlayer(ePlayer)){
                keymap[sym]=NULL;
            }
            else{
                keymap[sym]=NULL;
                keymap[sym]=uBindPlayer::NewBind(act,ePlayer);
            }
            return true;
        }
        return false;
    }
#endif

    virtual tString Help(){
        tString ret;
        ret << helpText << "\n";
        ret << tOutput("$input_item_help");
        return ret;
    }
};

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
    uMenuItemInput **input;

    uMenu input_menu(title);

    actions->tListItemBase::Sort(&Input_Compare);

    int len = actions->Len();

    input=tNEW(uMenuItemInput*)[len];
    int a=0;
    for(uAction *A=actions;A; A = A->Next()){
        input[a++]=new uMenuItemInput(&input_menu,
                                      A,
                                      ePlayer+1);

    }

    input_menu.ReverseItems();
    input_menu.Enter();

    for(int b=a-1;b>=0;b--)
        delete input[b];
    delete[] input;
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


REAL mouse_sensitivity=REAL(.1);
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

    for ( int i = sdlk_dynlast - 1; i>=0; --i )
    {
        if ( keymap[i] )
        {
            keymap[i]->HanldeDelayed();
        }
    }
}

bool su_HandleEvent(SDL_Event &e, bool delayed ){
#ifndef DEDICATED
    int sym=-1;
    REAL pm=0;

    if ( su_delayed && !delayed )
    {
        su_HandleDelayedEvents();
    }

    su_delayed = delayed;

    // there is nearly alllways a mouse motion tEvent:
    int xrel=e.motion.xrel;
    int yrel=-e.motion.yrel;


    switch (e.type){
    case SDL_MOUSEMOTION:
        if ( !su_mouseGrab ||
                e.motion.x!=sr_screenWidth/2 || e.motion.x!=sr_screenHeight/2)
        {
            if (keymap[SDLK_MOUSE_X_PLUS])
                keymap[SDLK_MOUSE_X_PLUS]->Activate(xrel*mouse_sensitivity, delayed );

            if (keymap[SDLK_MOUSE_X_MINUS])
                keymap[SDLK_MOUSE_X_MINUS]->Activate(-xrel*mouse_sensitivity, delayed );

            if (keymap[SDLK_MOUSE_Y_PLUS])
                keymap[SDLK_MOUSE_Y_PLUS]->Activate(yrel*mouse_sensitivity, delayed );

            if (keymap[SDLK_MOUSE_Y_MINUS])
                keymap[SDLK_MOUSE_Y_MINUS]->Activate(-yrel*mouse_sensitivity, delayed );
        }


        return true; // no fuss: allways pretend to have handled this.
        break;

    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:{
            int button=e.button.button;
            if (button<=MOUSE_BUTTONS){
                sym=SDLK_MOUSE_BUTTON_1+button-1;
            }
        }
        if (e.type==SDL_MOUSEBUTTONDOWN)
            pm=1;
        else
            pm=-1;
        break;

    case SDL_KEYDOWN:
        sym=e.key.keysym.sym;
        pm=1;
        break;

    case SDL_KEYUP:
        sym=e.key.keysym.sym;
        pm=-1;
        break;

#ifndef NOJOYSTICK
    case SDL_JOYAXISMOTION:
        pm = 1;
        if (e.jaxis.value > -16384 && e.jaxis.value < 16384)
            pm = -1;
        sym = joystick_sdlk[e.jaxis.which][0] + (e.jaxis.axis * 2) + ((e.jaxis.value > 0) ? 1 : 0);
        break;

    case SDL_JOYBUTTONDOWN:
    case SDL_JOYBUTTONUP:
        pm = (e.type == SDL_JOYBUTTONDOWN) ? 1 : -1;
        sym = joystick_sdlk[e.jbutton.which][1] + e.jbutton.button;
        break;
#endif

    default:
        break;
    }
    if (sym>=0 && keymap[sym]){
        REAL realpm=pm;
        if (keymap[sym]->act->type==uAction::uINPUT_ANALOG)
            pm*=ts*key_sensitivity;
        pressed[sym]=(realpm>0);
        if ( keymap[sym]->IsDoubleBind( sym ) )
            return true;
        return (keymap[sym]->Activate(pm, delayed ));

    }
    else
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

    for(int sym=sdlk_dynlast-1;sym>=0;sym--)
        if (pressed[sym] && keymap[sym] &&
                keymap[sym]->act->type==uAction::uINPUT_ANALOG)
            keymap[sym]->Activate(ts*key_sensitivity, su_delayed );
}

void su_ClearKeys()
{
    for(int sym=sdlk_dynlast-1;sym>=0;sym--)
    {
        if (pressed[sym] && keymap[sym] )
            keymap[sym]->Activate(-1, su_delayed );
        pressed[sym] = false;
    }
}

// *****************
// Player binds
// *****************

static char *Player_keyword="PLAYER_BIND";

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
    for ( int i = sdlk_dynlast-1; i >= 0; --i )
    {
        // compare action
        uBind * old = keymap[i];
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
    if (ePlayer==0)
        return  GlobalAct(act,x);
    else
        return uPlayerPrototype::PlayerConfig(ePlayer-1)->Act(act,x);
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
    for(uActionGlobalFunc *run = uActionGlobal_anchor; run ; run = run->Next())
        if (run->act == act && !run->rebindable)
            return true;

    return false;
}

bool uActionGlobalFunc::GlobalAct(uAction *act, REAL x){
    for(uActionGlobalFunc *run = uActionGlobal_anchor; run ; run = run->Next())
        if (run->act == act && run->func(x))
            return true;

    return false;
}

static uActionGlobal mess_up("MESS_UP");

static uActionGlobal mess_down("MESS_DOWN");

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

static uActionGlobalFunc mu(&mess_up,&messup_func);
static uActionGlobalFunc md(&mess_down,&messdown_func);
