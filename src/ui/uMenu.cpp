/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2000  Manuel Moos (manuel@moosnet.de)
Copyright (C) 2004  Armagetron Advanced Team (http://sourceforge.net/projects/armagetronad/)

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

#include "tSysTime.h"
#include "uMenu.h"
#include "rSysdep.h"
#include "rScreen.h"
#include "rViewport.h"
#include "tString.h"
#include "math.h"
#include "uInputQueue.h"
#include "rConsole.h"
#include "uInput.h"
#include "tDirectories.h"
//#include "tRecording.h"
#include "tToDo.h"
#include "tException.h"

#ifndef DEDICATED
#include "rRender.h"
#include "rSDL.h"
#endif

#include <vector>

FUNCPTR  uMenu::idle(NULL);

bool uMenu::wrap=true;
bool uMenu::quickexit=false;
bool uMenu::exitToMain=false;

// *****************************************************

#ifdef SLOPPYLOCALE
uMenu::uMenu(const char *t="",bool exit_item)
        :exitFlag(0),spaceBelow(.4),title(t){
    if (exit_item) new uMenuItemExit(this);
    center=0;
    menuTop=.7;
    menuBot=-.7;
    yOffset=0;
    selected = 10000000;
}
#endif

uMenu::uMenu(const tOutput &t,bool exit_item)
        :exitFlag(0),spaceBelow(.4),title(t){
    if (exit_item) new uMenuItemExit(this);
    center=0;
    menuTop=.7;
    menuBot=-.7;
    yOffset=0;
    selected = 100000000;
}

uMenu::~uMenu(){
    for(int i=items.Len()-1;i>=0;i--)
        delete items[i];
}

void uMenu::ReverseItems(){
    tList<uMenuItem> dummy = items;
    items.SetLen(0);

    for (int i=dummy.Len()-1; i>=0; i--){
        uMenuItem *x = dummy[i];
        dummy.Remove(x, x->idnum);
        items.Add  (x, x->idnum);
    }
}

//static REAL text_height=rCHEIGHT_NORMAL;
//static REAL text_width=rCWIDTH_NORMAL;

static REAL text_height=.11;
#ifndef DEDICATED
static REAL text_width=.05;
#endif

#ifndef DEDICATED
static REAL titlefac=1.2;
#endif
int menuentries=0;

REAL uMenu::YPos(int num){
    return yOffset-text_height*(menuentries-num);
}


static inline void arrow(REAL x,REAL y,REAL dy,REAL size){
#ifndef DEDICATED
    if (sr_glOut){
        BeginLineLoop();
        Vertex(x,y+2*dy*size);
        Vertex(x+size,y);
        Vertex(x+.3*size,y);
        Vertex(x+.3*size,y-2*dy*size);
        Vertex(x-.3*size,y-2*dy*size);
        Vertex(x-.3*size,y);
        Vertex(x-size,y);
        RenderEnd();
    }
#endif
}

static bool repeat = false;

#ifndef DEDICATED
static bool disphelp=false;
static REAL lastkey;
#endif

// inhibit console newline display while in a menu, it causes flickering
static bool su_inMenu = false;
bool uMenu::MenuActive()
{
    return su_inMenu;
}
static rNoAutoDisplayAtNewlineCallback su_noNewline( uMenu::MenuActive );
// static rSmallConsoleCallback su_smallConsole( su_InMenu );

void uMenu::OnEnter(){
#ifndef DEDICATED
    float nextrepeat = 0.0f;
    static const float repeatdelay = 0.3f;
    static const float repeatrate  = 0.05f;
    SDL_Event tEventRepeat;
#else
    return;
#endif

    // delete stuck keys, maybe a menu item catches key release events.
    su_ClearKeys();

    uCallbackMenuEnter::MenuEnter();
    su_inMenu = true;

    if (items.Len()<=0)
        return;

    exitFlag=0;
    yOffset=menuTop;
    REAL lastt=0;
    REAL ts=0;

#ifndef DEDICATED  
    lastkey=tSysTimeFloat();
    static const REAL timeout=3;
#endif

    while (!exitFlag && !quickexit && !exitToMain){
        st_DoToDo();
        tAdvanceFrame();

        ts=tSysTimeFloat()-lastt;
        lastt=tSysTimeFloat();
        if (ts>.2) ts=.2;

        menuentries=items.Len();


#ifndef DEDICATED
        {
            SDL_Event tEvent;
            uInputProcessGuard inputProcessGuard;
            while(su_GetSDLInput(tEvent))
            {
                REAL entertime = tSysTimeFloat();

                switch (tEvent.type)
                {
                case SDL_KEYDOWN:
                    repeat = true;
                    memcpy( &tEventRepeat, &tEvent, sizeof( SDL_Event ) );
                    nextrepeat = tSysTimeFloat() + repeatdelay;
                    break;
                case SDL_KEYUP:
                    repeat = false;
                    break;
                }

                this->HandleEvent( tEvent );

                // quit shortcut
                if ( quickexit )
                    break;

                if ( tSysTimeFloat() - entertime > 1 )
                {
                    repeat = false;
                }
            }

            if ( repeat && tSysTimeFloat() > nextrepeat )
            {
                this->HandleEvent( tEventRepeat );
                nextrepeat = tSysTimeFloat() + repeatrate;
            }
        }

        // we're about to render, last chance to make changes to the menu
        OnRender();

        // clamp cursor
        if (selected < 0 )
            selected = 0;
        if ( selected >= items.Len())
            selected = items.Len()-1;
#endif
        // quit shortcut
        if ( quickexit )
            break;


        menuBot=-1+spaceBelow;

        const REAL border=.3;
        const REAL smallborder=.1;

        menuentries=items.Len();

        REAL ysel=YPos(selected);

        if (ysel<menuBot+border)
            yOffset+=(menuBot+border-ysel)*6*ts;

        if (ysel>menuTop-border)
            yOffset+=(menuTop-border-ysel)*6*ts;

        if (ysel<menuBot)
            yOffset+=(menuBot-ysel);

        if (ysel>menuTop-smallborder)
            yOffset+=(menuTop-smallborder-ysel);

        if (YPos(0)>menuBot+smallborder)
            yOffset+=menuBot+smallborder-YPos(0);

        if (YPos(menuentries-1)<menuTop-smallborder)
            yOffset+=menuTop-smallborder-YPos(menuentries-1);

#ifndef DEDICATED
        sr_ResetRenderState(true);
        items[selected]->RenderBackground();

        if (selected >= items.Len()) selected = items.Len()-1;
        if (items.Len() <= 0)
            return;

        if (sr_glOut && !exitFlag && !quickexit){
            items[selected]->Render(center,YPos(selected),1,true);

            for(int i=items.Len()-1;i>=0;i--)
                if(i!=selected){
                    REAL y=YPos(i);
                    REAL alpha=1;
                    const REAL b=.1;
                    if(y<menuBot+b)
                        alpha=(y-menuBot)/b;
                    if(y>menuTop-b)
                        alpha=(menuTop-y)/b;
                    if (y>menuBot && y<menuTop)
                    {
                        rTextField::SetDefaultColor( tColor(1,1,1,1) );
                        rTextField::SetBlendColor( tColor(1,1,1,1) );
                        items[i]->Render(center,y,alpha,false);
                    }
                }

            rTextField::SetDefaultColor( tColor(1,1,1,1) );
            rTextField::SetBlendColor( tColor(1,1,1,1) );

            Color(.6,.6,1,1);
            ::DisplayText(0,menuTop+text_height*titlefac
                          ,text_width*titlefac,text_height*titlefac,
                          title,0);

            glDisable(GL_TEXTURE_2D);
            //glDisable(GL_TEXTURE);
            Color(1,.2,.2,.5);
            if (YPos(0)<menuBot+smallborder && (int(tSysTimeFloat()))%2)
                arrow(.9,menuBot+.1,-1,.05);
            if (YPos(menuentries-1)>menuTop && (int(tSysTimeFloat())+1)%2)
                arrow(.9,menuTop,1,.05);

            if (tSysTimeFloat()-lastkey>timeout){
                disphelp=true;
                if (sr_alphaBlend)
                    glColor4f(1,.8,.8,tSysTimeFloat()-lastkey-timeout);
                else
                    Color(tSysTimeFloat()-lastkey-timeout,
                          .8*(tSysTimeFloat()-lastkey-timeout),
                          .8*(tSysTimeFloat()-lastkey-timeout));

                rTextField c(-.95f,menuBot-.04f);
                c.SetWidth(static_cast<int>((1.9f-items[selected]->SpaceRight())/c.GetCWidth()));
                c << items[selected]->Help();
            }
            else disphelp=false;
        }
        else
#endif
            if ( !sr_glOut )
            {
                tDelay( 10000 );
            }

#ifndef DEDICATED
        rSysDep::SwapGL();
        rSysDep::ClearGL();
#endif
    }

    repeat = false;

    uCallbackMenuLeave::MenuLeave();
    su_inMenu = false;
}

void uMenu::HandleEvent( SDL_Event event )
{
#ifndef DEDICATED
    if (!items[selected]->Event(event))
    {
        switch (event.type){
        case SDL_KEYDOWN:
            {
                if (!disphelp)
                    lastkey=tSysTimeFloat();
                switch (event.key.keysym.sym){

                case(SDLK_ESCAPE):
                                repeat = false;
                    lastkey=tSysTimeFloat();
                    Exit();
                    break;

                case(SDLK_UP):
                                lastkey=tSysTimeFloat();
                    selected++;
                    if (selected>=items.Len())
                        if (wrap)
                            selected=0;
                        else
                            selected=items.Len()-1;
                    break;

                case(SDLK_DOWN):
                                lastkey=tSysTimeFloat();
                    selected--;
                    if (selected<0)
                        if(wrap)
                            selected=items.Len()-1;
                        else
                            selected=0;

                    break;

                case(SDLK_LEFT):
                                items[selected]->LeftRight(-1);
                    break;
                case(SDLK_RIGHT):
                                items[selected]->LeftRight(1);
                    break;

                case(SDLK_SPACE):
                            case(SDLK_KP_ENTER):
                                case(SDLK_RETURN):
                                        repeat = false;
                    try
            {
                        su_inMenu = false;
                        items[selected]->Enter();
                    }
                    catch (tException const & e)
                    {
                        uMenu::SetIdle(NULL);

                        // inform user of generic errors
                        tConsole::Message( e.GetName(), e.GetDescription(), 20 );
                    }
#ifdef _MSC_VER
#pragma warning ( disable : 4286 )
                    // GRR. Visual C++ dones not handle generic exceptions with the above general statement.
                    // A specialized version is needed. The best part: it warns about the code below being redundant.
                    catch( tGenericException const & e )
                    {
                        try
                        {
                            tConsole::Message( e.GetName(), e.GetDescription(), 20 );
                        }
                        catch(...)
                        {
                        }
                    }
#endif

                    su_inMenu = true;

                    repeat = false;
                    lastkey=tSysTimeFloat();
                    break;

                default:
                    // let the input subsystem handle events for later processing
                    su_HandleEvent( event, true );
                    break;
                }
            }
            break;
        default:
            // let the input subsystem handle events for later processing
            su_HandleEvent( event, true );
            break;
        }
    }

    su_inMenu = true;
#endif
}


// paints a nice background
void uMenu::GenericBackground(){
#ifndef DEDICATED
    if (idle)
    {
        try
        {
            // throw tGenericException("test"); // (test exception throw to see if error handling works right)
            (*idle)();
        }
        catch( ... )
        {
            // the idle background function is broken. Disable it and rethrow.
            idle = 0;
            throw;
        }
    }
    else if (sr_glOut){
        uCallbackMenuBackground::MenuBackground();
    }
    else
        tDelay(100000);
#endif
    sr_ResetRenderState(true);
}

// marks the menu for exit
void uMenu::OnExit(){
    exitFlag=1;
}

//! called every frame before the menu is rendered
void uMenu::OnRender()
{
}

// *****************************************************

// *******************************************************************************************
// *
// *   SetColor
// *
// *******************************************************************************************
//!
//!        @param  selected    flag indicating whether the menu item is currently selected
//!        @param  alpha       transparency to use
//!
// *******************************************************************************************

void uMenuItem::SetColor( bool selected, REAL alpha )
{
    //   rTextField::SetBlendColor( tColor(.8+.2*sin(time),.3-.1*sin(time),.3-.1*sin(time),alpha) );
    rTextField::SetDefaultColor( tColor(1,1,1,alpha) );

    if(selected)
    {
        REAL time=tSysTimeFloat()*10;
        REAL intensity = 1+.3*sin(time);
        rTextField::SetDefaultColor( tColor(.8,.3,.3,alpha) );
        rTextField::SetBlendColor( tColor(intensity,intensity,intensity,alpha) );
    }
}

void uMenuItem::DisplayText(REAL x,REAL y,const char *text,
                            bool selected,REAL alpha,
                            int center,int c,int cp){
#ifndef DEDICATED
    if (sr_glOut){
        SetColor( selected, alpha );

        REAL tw = text_width;
        REAL th = text_height;

        REAL availw = 1.9f;
        if (center < 0) availw = (.9f-x);
        if (center > 0) availw = (x + .9f);

        int len = strlen(text);
        if (len * tw > availw)
        {
            th *= availw/(len * tw);
            tw  = availw/len;
        }

        ::DisplayText(x,y,tw,th,text,center,c,cp);
    }
#endif
}

void uMenuItem::DisplayTextSpecial(REAL x,REAL y,const char *text,
                                   bool selected,
                                   REAL alpha,int center){
    /*
     if(selected)
       glColor3f(.9,.3,.3);
     else
       glColor3f(.7,.7,1);

     ::DisplayText(x,y,text_width,text_height,text,center);
     */

    DisplayText(x,y,text,selected,alpha,center);
}

// *************************************

const tOutput& uMenuItemExit::ExitText()
{
    static tOutput exitText("$menuitem_exit_text");

    return exitText;
}

const tOutput& uMenuItemExit::ExitHelp()
{
    static tOutput exitHelp("$menuitem_exit_help");

    return exitHelp;
}

// *************************************

void uMenuItemToggle::NewChoice(uSelectItem<bool> *){}
void uMenuItemToggle::NewChoice(const char *,bool ){}

#ifdef SLOPPYLOCALE
uMenuItemToggle::uMenuItemToggle(uMenu *m,
                                 const char *tit,
                                 const char *help,
                                 bool &targ)
        :uMenuItemSelection<bool>(m,tit,help,targ){
    uMenuItemSelection<bool>::NewChoice("$menuitem_toggle_on","",true);
    uMenuItemSelection<bool>::NewChoice("$menuitem_toggle_off","",false);
}
#endif

uMenuItemToggle::uMenuItemToggle(uMenu *m,
                                 const tOutput& tit,
                                 const tOutput& help,
                                 bool &targ)
        :uMenuItemSelection<bool>(m,tit,help,targ){
    uMenuItemSelection<bool>::NewChoice("$menuitem_toggle_on","",true);
    uMenuItemSelection<bool>::NewChoice("$menuitem_toggle_off","",false);
}

uMenuItemToggle::~uMenuItemToggle(){}

void uMenuItemToggle::LeftRight(int){
    select=1-select;
    *target=!(*target);
}

void uMenuItemToggle::Enter(){
    LeftRight(0);
}
// *****************************************
//               Integer Choose
// *****************************************

#ifdef SLOPPYLOCALE
uMenuItemInt::uMenuItemInt
(uMenu *m,const char *tit,const char *help,int &targ,
 int mi,int ma,int step)
        :uMenuItem(m,help),title(tit),target(targ),Min(mi),Max(ma),
Step(step){
    if (target<Min) target=Min;
    if (target>Max) target=Max;
}
#endif

uMenuItemInt::uMenuItemInt
(uMenu *m,const tOutput &tit,const tOutput &help,int &targ,
 int mi,int ma,int step)
        :uMenuItem(m,help),title(tit),target(targ),Min(mi),Max(ma),
Step(step){
    if (target<Min) target=Min;
    if (target>Max) target=Max;
}


void uMenuItemInt::LeftRight(int dir){
    target+=dir*Step;
    if (target<Min) target=Min;
    if (target>Max) target=Max;
}

void uMenuItemInt::Render(REAL x,REAL y,REAL alpha,
                          bool selected){
    DisplayText(x-.02,y,title,selected,alpha,1);

    tString s;
    s << target;
    DisplayText(x+.02,y,s,selected,alpha,-1);
}


// *****************************************************

uMenuItemString::uMenuItemString(uMenu *M,
                                 const tOutput& de,
                                 const tOutput& help,
                                 tString &c,
                                 int maxLength )
        :uMenuItem(M,help),description(de),content(&c),cursorPos(0), maxLength_( maxLength ){
    int len=content->Len();
    if (len==0 || (*content)(len-1)!=0)
        (*content)[len]=0;
    cursorPos=content->Len()-1;
}

void uMenuItemString::Render(REAL x,REAL y,
                             REAL alpha,bool selected){
#ifndef DEDICATED
    static int counter=0;
    counter++;

    int cmode=0;
    if (selected){
        cmode=1;
        if (counter & 32) cmode=2;
    }

    DisplayText(x-.02,y,description,selected,alpha,1);
    DisplayText(x+.02,y,&((*content)[0]),selected,alpha,-1,cmode,cursorPos);
#endif
}

bool uMenuItemString::Event(SDL_Event &e){
#ifndef DEDICATED
    if (e.type!=SDL_KEYDOWN)
        return false;
    bool ret=true;
    SDL_keysym &c=e.key.keysym;
    SDLMod mod = c.mod;
    bool moveWordLeft, moveWordRight, deleteWordLeft, deleteWordRight, moveBeginning, moveEnd, killForwards;
    moveWordLeft = moveWordRight = deleteWordLeft = deleteWordRight = moveBeginning = moveEnd = killForwards = false;

#if defined (MACOSX)
    // For moving over/deleting words
    if (mod & KMOD_ALT) {
        if (c.sym == SDLK_LEFT) {
            moveWordLeft = true;
        }
        else if (c.sym == SDLK_RIGHT) {
            moveWordRight = true;
        }
        else if (c.sym == SDLK_DELETE) {
            deleteWordRight = true;
        }
        else if (c.sym == SDLK_BACKSPACE) {
            deleteWordLeft = true;
        }
    }
    // For moving to extremes of the line
    else if (mod & KMOD_META) {
        if (c.sym == SDLK_LEFT) {
            moveBeginning = true;
        }
        else if (c.sym == SDLK_RIGHT) {
            moveEnd = true;
        }
    }
    // Linux and Windows
#else
    // Word operations
    if (mod & KMOD_CTRL) {
        if (c.sym == SDLK_LEFT) {
            moveWordLeft = true;
        }
        else if (c.sym == SDLK_RIGHT) {
            moveWordRight = true;
        }
        else if (c.sym == SDLK_DELETE) {
            deleteWordRight = true;
        }
        else if (c.sym == SDLK_BACKSPACE) {
            deleteWordLeft = true;
        }
    }
    else if (c.sym == SDLK_HOME) {
        moveBeginning = true;
    }
    else if (c.sym == SDLK_END) {
        moveEnd = true;
    }
#endif
    // "bash" keys
    if (mod & KMOD_CTRL) {
        if (c.sym == SDLK_a) {
            moveBeginning = true;
        }
        else if (c.sym == SDLK_e) {
            moveEnd = true;
        }
        else if (c.sym == SDLK_k) {
            killForwards = true;
        }
    }
    // moveWordLeft = moveWordRight = deleteWordLeft = deleteWordRight = moveBeginning = moveEnd = killForwards

    if (moveWordLeft) {
        cursorPos += content->PosWordLeft(cursorPos);
    }
    else if (moveWordRight) {
        cursorPos += content->PosWordRight(cursorPos);
    }
    else if (deleteWordLeft) {
        cursorPos += content->RemoveWordLeft(cursorPos);
    }
    else if (deleteWordRight) {
        content->RemoveWordRight(cursorPos);
    }
    else if (moveBeginning) {
        cursorPos = 0;
    }
    else if (moveEnd) {
        cursorPos = content->Len()-1;
    }
    else if (killForwards) {
        content->RemoveSubStr(cursorPos,content->Len()-1-cursorPos);
    }
    else if (c.sym == SDLK_LEFT) {
        if (cursorPos > 0) {
            cursorPos--;
        }
    }
    else if (c.sym == SDLK_RIGHT) {
        if (cursorPos < content->Len()-1) {
            cursorPos++;
        }
    }
    else if (c.sym == SDLK_DELETE) {
        if (cursorPos < content->Len()-1) {
            content->RemoveSubStr(cursorPos,1);
        }
    }
    else if (c.sym == SDLK_BACKSPACE) {
        if (cursorPos > 0) {
            content->RemoveSubStr(cursorPos,-1);
            cursorPos--;
        }
    }
    else if (c.sym == SDLK_KP_ENTER || c.sym == SDLK_RETURN) {
        ret = false;
        //        c.sym = SDLK_DOWN;
    }
    else {
        if (32 <= c.unicode  && c.unicode < 256)
        {
            ret=true;

            // insert character if there is room
            if (content->Len() < maxLength_)
            {
                for(int i=content->Len()-1;i>=cursorPos;i--)
                    (*content)[i+1]=(*content)[i];

                // guarantee proper null termination
                (*content)[content->Len()-1]='\0';
                (*content)[cursorPos]=c.unicode;
                cursorPos++;
            }
        }
        else {
            ret=false;
        }
    }

    if(cursorPos<0)    cursorPos=0;
    if(cursorPos > content->Len()-1) cursorPos=content->Len()-1;

    return ret;
#else  
    return false;
#endif
}

uMenuItemStringWithHistory::uMenuItemStringWithHistory(uMenu *M,const tOutput& desc, const tOutput& help,tString &c, int maxLength, std::deque<tString> &history, int limit ):
        uMenuItemString(M, desc,help,c, maxLength ),
        m_History(history),
        m_HistoryPos(0),
        m_HistoryLimit(limit)
{
    m_History.push_front(tString());
}

uMenuItemStringWithHistory::~uMenuItemStringWithHistory()
{
    if (content->Len() > 1)
    {
        for (std::deque<tString>::iterator i=m_History.begin(); i!=m_History.end(); ++i)
        {
            if (*i == *content)
            {
                m_History.erase(i);
                break;
            }
        }
        m_History.front() = *content;
    }
    else
    {
        m_History.pop_front();
    }
    if (m_History.size() > m_HistoryLimit)
        m_History.pop_back();
}

bool uMenuItemStringWithHistory::Event(SDL_Event &e)
{
    // flag indicating that the event was handled
    bool ret = false;
#ifndef DEDICATED
    SDLMod mod = e.key.keysym.mod;

    if (e.type == SDL_KEYDOWN
            && ((e.key.keysym.sym == SDLK_UP)
                || (e.key.keysym.sym == SDLK_p && mod & KMOD_CTRL)))
    {
        if (m_History.size() - 1 > m_HistoryPos)
        {
            // the new entry... save it before overwriting it
            if (m_HistoryPos == 0)
                m_History.front() = *content;
            m_HistoryPos++;
            *content = m_History[m_HistoryPos];
            cursorPos = content->Len() - 1;
        }

        ret = true;
    }
    else if (e.type == SDL_KEYDOWN
             && ((e.key.keysym.sym == SDLK_DOWN)
                 || (e.key.keysym.sym == SDLK_n && mod & KMOD_CTRL)))
    {
        if (m_HistoryPos > 0)
        {
            m_HistoryPos--;
            *content = m_History[m_HistoryPos];
            cursorPos = content->Len() - 1;
        }

        ret = true;
    }

    // clamp cursor position
    if (cursorPos<0)
        cursorPos=0;
    if (cursorPos > content->Len() - 1)
        cursorPos=content->Len() - 1;
#endif

    // return result or delegate
    return ret || uMenuItemString::Event(e);
}

// *****************************************************
//  Submenu
// *****************************************************


uMenuItemSubmenu::uMenuItemSubmenu(uMenu *M,
                                   uMenu *s,
                                   const tOutput& help)
    :uMenuItem(M,help),submenu(s){}


void uMenuItemSubmenu::Render(REAL x,REAL y,REAL alpha,bool selected){
    DisplayTextSpecial(x,y,submenu->title,selected,alpha,0);
}

void uMenuItemSubmenu::Enter(){
    submenu->Enter();
}

// *****************************************************
//  action
// *****************************************************


uMenuItemAction::uMenuItemAction(uMenu *M,
                                 const tOutput& n, const tOutput& help )
        :uMenuItem(M,help),name_(n){}


void uMenuItemAction::Render(REAL x,REAL y,REAL alpha,bool selected){
    DisplayTextSpecial(x,y,name_,selected,alpha,0);
}


void uMenuItemAction::Enter()
{
    tASSERT( 0 )
}


// *****************************************************
//  function
// *****************************************************


uMenuItemFunction::uMenuItemFunction(uMenu *M,
                                     const tOutput& n, const tOutput& help,
                                     FUNCPTR f)
        :uMenuItemAction(M,n,help),func(f){}

void uMenuItemFunction::Enter(){
    (*func)();
}



uMenuItemFunctionInt::uMenuItemFunctionInt(uMenu *M,
        const tOutput& n,
        const tOutput& help,
        INTFUNCPTR f,int a)
        :uMenuItemAction(M,n,help),func(f),arg(a){}


void uMenuItemFunctionInt::Enter(){
    (*func)(arg);
}

// *****************************************************
//  File Selection (added by k)
// *****************************************************

void uMenuItemFileSelection::NewChoice( uSelectItem<bool> * ) {}
void uMenuItemFileSelection::NewChoice( char *, bool ) {}

void uMenuItemFileSelection::Reload()
{
    Clear();
    if ( defaultFileName_.Len() > 1 && defaultFilePath_.Len() > 1 )
        AddFile( defaultFileName_, defaultFilePath_, formatName_ );
    LoadDirectory( dir_, fileSpec_, formatName_ );
}

void uMenuItemFileSelection::LoadDirectory( const char *dir, const char *fileSpec,
        bool formatName /*= true*/ )
{
    tArray <tString> files;
    tString filePath ( dir );
    tDirectories::GetFiles( tString( dir ), tString( fileSpec ), files, getFilesFlag_ );
    for ( int i = 0; i < files.Len(); i++ )
    {
        AddFile( files( i ), filePath + files( i ), formatName );
    }
}

void uMenuItemFileSelection::AddFile( const char *fileName, const char *filePath,
                                      bool formatName /*= true*/ )
{
    tString menuName ( fileName );
    if ( formatName )
        tDirectories::FileNameToMenuName( fileName, menuName );
    uMenuItemSelection<tString>::NewChoice( menuName, "", tString( filePath ) );
}

// *****************************************************
// Menu Enter/Leave-Callback
// *****************************************************

static tCallback *enter_anchor=NULL,*leave_anchor=NULL, *background_anchor=NULL;

uCallbackMenuEnter::uCallbackMenuEnter(VOIDFUNC *f)
    :tCallback(enter_anchor,f){}

void uCallbackMenuEnter::MenuEnter(){
    Exec(enter_anchor);
}

uCallbackMenuLeave::uCallbackMenuLeave(VOIDFUNC *f)
        :tCallback(leave_anchor,f){}

void uCallbackMenuLeave::MenuLeave(){
    Exec(leave_anchor);
}

uCallbackMenuBackground::uCallbackMenuBackground(VOIDFUNC *f)
        :tCallback(background_anchor,f){}

void uCallbackMenuBackground::MenuBackground(){
    Exec(background_anchor);
}



void uMenu::Message(const tOutput& message, const tOutput& interpretation, REAL to){
#ifdef DEDICATED
    con << message << ":\n";
    con << interpretation << '\n';
#else

    // reload textures (just in case)
    rITexture::UnloadAll();

    bool textOutBack = sr_textOut;
    sr_textOut = false;

    FUNCPTR idle_back = idle;
    uMenu::SetIdle(NULL);

    rTextField::SetDefaultColor( tColor(1,1,1,1) );
    rTextField::SetBlendColor( tColor(1,1,1,1) );

    rSysDep::ClearGL();
    rSysDep::SwapGL();
    if (sr_glOut)
    {
        rFont::s_defaultFont.Select();
        rFont::s_defaultFontSmall.Select();
    }
    rSysDep::ClearGL();
    rSysDep::SwapGL();

    REAL timeout = tSysTimeFloat() + to;
    SDL_Event tEvent;

    // catch some keyboard input
    {
        uInputProcessGuard inputProcessGuard;
        while(su_GetSDLInput(tEvent));
    }

    {
        uInputProcessGuard inputProcessGuard;

        unsigned offset = 0; //amount of scrolling taking place
        //convert to an array for scrolling
        tString interpretationString;
        interpretationString << interpretation << "\n";
        std::vector<tString> lines;
        int lastNewline = 0;
        for(int i = 0; i < interpretationString.Len() - 1; ++i) {
            if (interpretationString[i] == '\n' && i != 0) {
                lines.push_back(interpretationString.SubStr(lastNewline, i - lastNewline));
                lastNewline = i + 1;
            }
        }
        while(  !quickexit &&
                (to < 0 || tSysTimeFloat() < timeout)){
            //while(  !quickexit && ( !su_GetSDLInput(tEvent) || tEvent.type!=SDL_KEYDOWN) &&
            //        (to < 0 || tSysTimeFloat() < timeout)){
            if( su_GetSDLInput(tEvent) && tEvent.type==SDL_KEYDOWN) {
                switch(tEvent.key.keysym.sym) {
                case SDLK_UP:
                    if(offset > 0)
                        offset -= 1;
                    continue;
                case SDLK_DOWN:
                    offset += 1;
                    continue;
                default:
                    break;
                }
                break;
            }
            if ( sr_glOut )
            {
                sr_ResetRenderState(true);
                rViewport::s_viewportFullscreen.Select();

                rSysDep::ClearGL();

                GenericBackground();

                REAL w=16*3/640.0;
                REAL h=32*3/480.0;


                //REAL middle=-.6;

                tString m(message);
                int len = m.Len();
                if (w * len > 1.8)
                {
                    h = h * 1.8 / (w * len);
                    w = 1.8 / len;
                }

                Color(1,1,1);
                DisplayText(0,.8,w,h, message);

                w = 16/640.0;
                h = 32/480.0;

                if(offset >= lines.size()) offset = lines.size() - 1;
                {
                    rTextField c(-.8,.6, w, h);

                    for(unsigned i = offset; i < lines.size(); ++i)
                        c << lines[i] << "\n";
                }
            }
            rSysDep::SwapGL();
            tAdvanceFrame();
        }
    }

    // catch some keyboard input
    {
        uInputProcessGuard inputProcessGuard;
        while (su_GetSDLInput(tEvent));
    }

    uMenu::SetIdle(idle_back);

    // reload textures (just in case)
    rITexture::UnloadAll();

    sr_textOut = textOutBack;
#endif
}

