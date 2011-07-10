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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
  
***************************************************************************

*/

#ifndef ArmageTron_MENU_H
#define ArmageTron_MENU_H

#include "rFont.h"
#include "tList.h"
#include "tString.h"
#include "tCallback.h"
#include "tLocale.h"

#include "rSDL.h"
#ifndef DEDICATED
#include <SDL_events.h>
#endif

#include <deque>

#include "defs.h"

class uMenuItem;

class uMenu{
    friend class uMenuItem;

protected:
    tList<uMenuItem>      items;
    static FUNCPTR       idle;

    REAL menuTop;
    REAL menuBot;
    REAL yOffset;

    bool                 exitFlag;

    REAL                 spaceBelow;
    REAL                 center;

    int                  selected;

    REAL YPos(int num);
public:
    static bool          wrap;
    
    // different quick exit types
    enum QuickExit
    {
        QuickExit_Off  = 0, // no quick exit, keep going
        QuickExit_Game = 1, // quit to game selection menu
        QuickExit_Total = 2 // quit the program
    };

    static QuickExit     quickexit;
    static bool          exitToMain;

    tOutput              title;

    FUNCPTR IdleFunc(){return idle;}
    static void SetIdle(FUNCPTR idle_func) {idle=idle_func;}

    // poll input, return true if ESC was pressed
    static bool IdleInput();

    void SetCenter(REAL c) {center=c;}
    void SetTop(REAL t) {menuTop=t;}
    void SetBot(REAL b) {menuBot=b;spaceBelow=1+menuBot;}
    REAL GetTop() const {return menuTop;}
    REAL GetBot() const {return menuBot;}
    void SetSelected(int s) {selected = s;}
    int  NumItems()         {return items.Len();}
    uMenuItem* Item(int i)  { return items[i]; }
    void AddItem(uMenuItem* item);
    void RemoveItem(uMenuItem* item);

#ifdef SLOPPYLOCALE
    uMenu(const char *t,bool exit_item=true);
#endif
    uMenu(const tOutput &t,bool exit_item=true);
    virtual ~uMenu();

    //! enters the menu; calls idle_func before rendering every frame
    inline void Enter(){OnEnter();}

    void ReverseItems();

    // paints a nice background
    static void GenericBackground(REAL top=1.0);

    // marks the menu for exit
    inline void Exit(){OnExit();}

    void RequestSpaceBelow(REAL x){
        if (spaceBelow<x)
            spaceBelow=x;
    }

    // print a big message and a small interpretation
    static bool Message(const tOutput& message, const tOutput& interpretation, REAL timeout = -1);

    //! returns whether there is currently an active menu
    static bool MenuActive();
protected:
    //! handles a key press
    virtual void HandleEvent( SDL_Event event );

    //! enters the menu; calls idle_func before rendering every frame
    virtual void OnEnter();

    //! marks the menu for exit
    virtual void OnExit();

    int GetNextSelectable(int start);
    int GetPrevSelectable(int start);

    //! called every frame before the menu is rendered
    virtual void OnRender();
};


// *****************************************************

class uMenuItem{
    friend class uMenu;

    int idnum;
    uMenuItem(){}
protected:
    uMenu  *menu;
    tOutput helpText;

    void DisplayText(REAL x,REAL y,const char *text,bool selected,
                     REAL alpha=1,int center=0,int cursor=0,int cursorPos=0, rTextField::ColorMode colorMode = rTextField::COLOR_USE, float maxWidth=2. );
    void DisplayTextSpecial(REAL x,REAL y,const char *text,bool selected,
                            REAL alpha=1,int center=0);
public:
    uMenuItem(uMenu *M,const tOutput &help)
            :idnum(-1),menu(M),helpText(help){
        menu->items.Add(this,idnum);
    }

#ifdef SLOPPYLOCALE
    uMenuItem(uMenu *M,const char *help)
            :idnum(-1),menu(M),helpText(help){
        menu->items.Add(this,idnum);
    }
#endif

    virtual ~uMenuItem()
    {
        if (menu && idnum >= 0)
            menu->items.Remove(this,idnum);
    }

virtual tString Help(){return tString(helpText);}
    // displays the menuitem at position x,y. set selected to true
    // if the item is currently under the cursor
    virtual void Render(REAL ,REAL ,REAL =1,bool =0){}

    virtual void RenderBackground(){
        menu->GenericBackground();
    }

    // if the user presses left/right on menuitem
    virtual void LeftRight(int ){} //lr=-1:left lr=+1: right
    virtual void LeftRightRelease(){}

    virtual void Enter(){} // if the user presses enter/space on menu

    virtual bool Event(SDL_Event &){return false;} // if the key c is
    // pressed,mouse moved ...
    // while menuitem is active; return value: key was used

    virtual REAL SpaceRight(){return 0;}

    int GetID(){return idnum;}

    virtual bool IsSelectable(){return true;};

protected:
    void SetColor( bool selected, REAL alpha );            //!< Sets the color of text output for this menuitem
};


// *****************************************************
// Menu Exit
// *****************************************************

class uMenuItemExit:public uMenuItem{
    tOutput t;

    static const tOutput& ExitText();
    static const tOutput& ExitHelp();

public:
    uMenuItemExit(uMenu *M,
                  const tOutput &text = ExitText(),
                  const tOutput &help = ExitHelp())
            :uMenuItem(M,help)
    ,t(text){}

    // displays the menuitem at position x,y. set selected to true
    // if the item is currently under the cursor
    virtual void Render(REAL x,REAL y,REAL alpha=1,bool selected=0){
        DisplayTextSpecial(x,y,tString(t),selected,alpha);
    }

    virtual void Enter(){menu->Exit();}
    // if the user presses enter/space on menu
};

// *****************************************************
// Selection
// *****************************************************

template<class T> class uSelectItem{
public:
    int idnum;

    tOutput description;
    tOutput helpText;
    T value;

    uSelectItem(const tOutput& desc,const tOutput& help,T val)
            :idnum(-1),description(desc),helpText(help),value(val){}
};

template<class T> class uMenuItemSelection:public uMenuItem{
protected:
    tList<uSelectItem<T> >	choices;
    tOutput               	title;
    int                   	select;
    T *                   	target;
public:
#ifdef SLOPPYLOCALE
    uMenuItemSelection(uMenu *m,
                       const char* tit,const char *help,
                       T &targ)
            :uMenuItem(m,help),title(tit),select(0),target(&targ){}
#endif

    uMenuItemSelection(uMenu *m,
                       const tOutput &tit,const tOutput &help,
                       T &targ)
            :uMenuItem(m,help),title(tit),select(0),target(&targ){}

    ~uMenuItemSelection(){
        Clear();
    }

    void Clear(){
        for(int i=choices.Len()-1;i>=0;i--){
            uSelectItem<T> *x=choices(i);
            choices.Remove(x,x->idnum);
            delete x;
        }
        select=0;
    }

    void NewChoice(uSelectItem<T> *c){
        choices.Add(c,c->idnum);
    }

    void NewChoice(const tOutput& desc,const tOutput& help,T val){
        uSelectItem<T> *x=new uSelectItem<T>(desc,help,val);
        NewChoice(x);
    }

    virtual void LeftRight(int lr){
        select+=lr;
        if(select>=choices.Len())
            select=choices.Len()-1;
        if(select<0)
            select=0;
        if (choices.Len())
            *target=choices(select)->value;
    }

    virtual void Render(REAL x,REAL y,REAL alpha=1,bool selected=0){
        for(int i=choices.Len()-1;i>=0;i--)
            if (choices(i)->value==*target)
                select=i;

        DisplayText(REAL(x-.02),y,title,selected,alpha,1);
        if (choices.Len()>0)
            DisplayText(REAL(x+.02),y,choices(select)->description,selected,alpha,-1);
    }

    virtual tString Help(){
        tString ret;
        ret << helpText;
        ret << "\n";
        ret << choices(select)->helpText;
        return ret;
    }

};

template<class T> class uSelectEntry{
public:
    uSelectEntry(uMenuItemSelection<T> &sel,
                 const char *desc,
                 const char *help,T val){
        sel.NewChoice(desc,help,val);
    }
};

// *****************************************
//               Toggle
// *****************************************

class uMenuItemToggle: public uMenuItemSelection<bool>{
    void NewChoice(uSelectItem<bool> *c);
    void NewChoice(const char *desc,bool val);
public:
#ifdef SLOPPYLOCALE
    uMenuItemToggle(uMenu *m,const char *tit,
                    const char *help,bool &targ);
#endif
    uMenuItemToggle(uMenu *m,const tOutput& title,
                    const tOutput& help,bool &targ);
    ~uMenuItemToggle();

    virtual void LeftRight(int);
    virtual void Enter();
};


// *****************************************
//               Integer Choose
// *****************************************

class uMenuItemInt:public uMenuItem{
protected:
    tOutput title;
    int &target;
    int Min,Max;
    int Step;
public:
    /*
      uMenuItemInt(uMenu *m,const char *tit,
      const char *help,int &targ,
      int mi,int ma,int step=1);
    */
    uMenuItemInt(uMenu *m,const tOutput &title,
                 const tOutput &help,int &targ,
                 int mi,int ma,int step=1);

    ~uMenuItemInt(){}

    virtual void LeftRight(int);

    virtual void Render(REAL x,REAL y,REAL alpha=1,bool selected=0);
};

// *****************************************
//               Float Choose
// *****************************************

class uMenuItemReal:public uMenuItem{
protected:
    tOutput title;
    REAL &target;
    REAL Min,Max;
    REAL Step;
public:
    /*
      uMenuItemInt(uMenu *m,const char *tit,
      const char *help,int &targ,
      int mi,int ma,int step=1);
    */
    uMenuItemReal(uMenu *m,const tOutput &title,
                 const tOutput &help,REAL &targ,
                 REAL mi,REAL ma,REAL step=1);

    ~uMenuItemReal(){}

    virtual void LeftRight(int);

    virtual void Render(REAL x,REAL y,REAL alpha=1,bool selected=0);
};

// *****************************************************
//  String query
// *****************************************************


class uMenuItemString: public uMenuItem{
protected:
    tOutput  description;
    tString *content;
    size_t realCursorPos; // position of cursor as byte offset into string
    int      maxLength_;

    bool InsertChar(int unicode, bool convert = true);

    // color mode used for rendering
    rTextField::ColorMode colorMode_;
public:
    uMenuItemString(uMenu *M,const tOutput& desc,
                    const tOutput& help,tString &c, int maxLength = 1024 );
    virtual ~uMenuItemString(){}

    virtual void Render(REAL x,REAL y,REAL alpha=1,bool selected=0);

    virtual bool Event(SDL_Event &e);

    uMenu *MyMenu(){return menu;}

    void SetColorMode( rTextField::ColorMode colorMode )
    {
        colorMode_ = colorMode;
    }

    rTextField::ColorMode GetColorMode() const
    {
        return colorMode_;
    }
};

//! A class that can provide auto- completion and supports overwriting of parts for special cases.
//!
//! The default implementation implements the typical bash- like auto- completion, if you need more than that derive your own class.
class uAutoCompleter {
protected:
    std::deque<tString> &m_PossibleWords; //!< The words that can be used for completion
    int m_LastCompletion;
    bool m_ignorecase;

    virtual int FindLengthOfLastWord(tString &string, unsigned pos); //!< Finds the space to the last delimiter
    virtual void FindPossibleWords(tString word, std::deque<tString> &results); //!< Finds the possible completions for a word
    virtual tString FindClosestMatch(tString &word, std::deque<tString> &results); //!< Attempts to complete as much of the word as possible
    virtual void ShowPossibilities(std::deque<tString> &results, tString &word); //!< Prints the possible completions to the console
    virtual int DoCompletion(tString &string, int pos, int len, tString &match); //!< Replaces the word the cursor is on by the closest match
    virtual int DoFullCompletion(tString &string, int pos, int len, tString &match); //!< Replaces the word the cursor is on by the given match and a space
    virtual int TryCompletion(tString &string, unsigned pos, unsigned len); //!< Attempt completion with a certain word length

    virtual tString Simplify(tString const &str); //!< Simplifies a string, by default converts it to lowercase
public:
    uAutoCompleter(std::deque<tString> &words); //!< Constructor
    virtual ~uAutoCompleter() {}
    virtual int Complete(tString &string, unsigned pos); //!< Attempts the completion

    void SetIgnorecase(bool ignorecase); //!< Enable or disable case ignoring?
};

//! uMenuItemString extended by a simple history function and tab completion
class uMenuItemStringWithHistory : protected uMenuItemString {
public:
    //! handles loading and saving the contents of the history to a file
class history_t : public std::deque<tString> {
        char const *m_filename;
    public:
        history_t(char const *filename); //!< this will load the history from the file
        ~history_t(); //!< this will save the history to the file
    };
protected:
    history_t &m_History; //!< The saved history lines
    unsigned int m_HistoryPos; //!< The current position within the history
    unsigned int m_HistoryLimit; //!< The maximal length of the history
    uAutoCompleter *m_Completer; //!< The object used for completion
    bool m_Searchmode; //!< Are we in search mode?
    bool m_SearchFailing; //!< Is the seach turning up any useful results?
public:
    uMenuItemStringWithHistory(uMenu *M,const tOutput& desc, const tOutput& help,tString &c, int maxLength, history_t &history, int limit, uAutoCompleter *completer = 0); //!< Consructor

    ~uMenuItemStringWithHistory(); //!< Destructor

    virtual bool Event(SDL_Event &e); //!< Handles an event
    virtual void Render(REAL x,REAL y,REAL alpha=1,bool selected=0); //!< Renders the search suggestion if needed and calls uMenuItemString::Render()
    virtual void RenderBackground(){
        menu->GenericBackground(menu->GetTop());
    }
};

// *****************************************************
//  Submenu
// *****************************************************


class uMenuItemSubmenu: public uMenuItem{
    uMenu *submenu;
public:
    uMenuItemSubmenu(uMenu *M,uMenu *s,
                     const tOutput& help);
    virtual ~uMenuItemSubmenu(){}

    virtual void Render(REAL x,REAL y,REAL alpha=1,bool selected=0);

    virtual void Enter();
};


// *****************************************************
//  generic action
// *****************************************************

class uMenuItemAction: public uMenuItem{
public:
    uMenuItemAction(uMenu *M,const tOutput& name,
                    const tOutput& help );

    virtual ~uMenuItemAction(){}

    virtual void Render(REAL x,REAL y,REAL alpha=1,bool selected=0);

    virtual void Enter() = 0;
protected:
    tOutput  name_;
};

// *****************************************************
//  Execute function
// *****************************************************


class uMenuItemFunction: public uMenuItemAction{
    FUNCPTR  func;
public:
    uMenuItemFunction(uMenu *M,const tOutput& name,
                      const tOutput& help,FUNCPTR f);

    virtual ~uMenuItemFunction(){}

    virtual void Enter();
};


class uMenuItemFunctionInt: public uMenuItemAction{
    INTFUNCPTR  func;
    int         arg;
public:
    uMenuItemFunctionInt(uMenu *M,const tOutput& name,
                         const tOutput& help,INTFUNCPTR f,int arg);

    virtual ~uMenuItemFunctionInt(){}

    virtual void Enter();
};

// *****************************************************
//  File Selection (added by k)
// *****************************************************

class uMenuItemFileSelection: public uMenuItemSelection<tString>
{
    void NewChoice( uSelectItem<bool> * );
    void NewChoice( char *, bool );
protected:
    tString dir_;
    tString fileSpec_;
    int     getFilesFlag_;
    bool    formatName_;
    tString defaultFileName_;
    tString defaultFilePath_;
public:
    uMenuItemFileSelection( uMenu *m, char *tit, const char *help, tString& targ,
                            const char *dir, const char *fileSpec, int getFilesFlag = 0, bool formatName = true )
            :uMenuItemSelection<tString>( m, tit, help, targ )
    {
        SetParams( dir, fileSpec, getFilesFlag, formatName, "", "" );
        Reload();
    }

    uMenuItemFileSelection( uMenu *m, char *tit, const char *help, tString& targ,
                            const char *dir, const char *fileSpec,
                            const char *defaultFileName, const char *defaultFilePath,
                            int getFilesFlag = 0, bool formatName = true )
            :uMenuItemSelection<tString>( m, tit, help, targ )
    {
        SetParams( dir, fileSpec, getFilesFlag, formatName, defaultFileName, defaultFilePath );
        Reload();
    }

    virtual ~uMenuItemFileSelection() {}

    void SetDir( const char *dir ) { dir_ = dir; }
    void SetFileSpec( const char *fileSpec ) { fileSpec_ = fileSpec; }
    void SetFormatName( bool formatName ) { formatName_ = formatName; }
    void SetGetFilesFlag( int getFilesFlag ) { getFilesFlag_ = getFilesFlag; }
    void SetDefaultFileName( const char *defaultFileName ) { defaultFileName_ = defaultFileName; }
    void SetDefaultFilePath( const char *defaultFilePath ) { defaultFilePath_ = defaultFilePath; }

    void SetParams( const char *dir, const char *fileSpec, int getFilesFlag,
                    bool formatName, const char *defaultFileName, const char *defaultFilePath )
    {
        SetDir( dir );
        SetFileSpec( fileSpec );
        SetGetFilesFlag( getFilesFlag );
        SetFormatName( formatName );
        SetDefaultFileName( defaultFileName );
        SetDefaultFilePath( defaultFilePath );
    }

    void Reload();

    void LoadDirectory( const char *dir, const char *fileSpec, bool formatName = true );

    void AddFile( const char *fileName, const char *filePath, bool formatName = true );
};

// *****************************************************
// Menu Enter/Leave-Callback
// *****************************************************

class uCallbackMenuEnter: public tCallback{
public:
    uCallbackMenuEnter(AA_VOIDFUNC *f);
    static void MenuEnter();
};

class uCallbackMenuLeave: public tCallback{
public:
    uCallbackMenuLeave(AA_VOIDFUNC *f);
    static void MenuLeave();
};

class uCallbackMenuBackground: public tCallback{
public:
    uCallbackMenuBackground(AA_VOIDFUNC *f);
    static void MenuBackground();
};


inline void uMenu::AddItem(uMenuItem* item)     { items.Add(item, item->idnum); }
inline void uMenu::RemoveItem(uMenuItem* item)  { items.Remove(item, item->idnum); }

#endif

