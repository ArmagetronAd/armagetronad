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

#ifndef ArmageTron_consoleBase_H
#define ArmageTron_consoleBase_H

#include "tString.h"
#include "defs.h"
#include "tLinkedList.h"

class tOutput;

//! filter for console messages
class tConsoleFilter: public tListItem< tConsoleFilter >
{
public:
    tConsoleFilter();
    ~tConsoleFilter();

    //! filter an output line
    inline void FilterLine( tString& line )
    {
        DoFilterLine( line );
    }

    //! filter a localizable output paramenter
    inline void FilterElement( tString& element )
    {
        DoFilterElement( element );
    }

    inline int GetPriority() const
    {
        return DoGetPriority();
    }
private:
    // the functions that do the real work
    virtual void DoFilterLine	( tString& line 	);		//!< filter an output line
    virtual void DoFilterElement( tString& element 	);		//!< filter a localizable output paramenter

    virtual int DoGetPriority() const;						//!< return filter priority
};

// console class logging various messages
class tConsole
{
public:
    // callback for messages the user should read
    typedef bool MessageCallback(const tOutput& message, const tOutput& interpretation, REAL timeout);

    // idle callback; called from various spots when the progam is waiting.
    // return true if the waiting should be aborted
    typedef bool IdleCallback();

    virtual ~tConsole();

    tConsole & Print(tString const & s);

    template<class T> tConsole & operator<<(const T&x){
        tColoredString s;
        s << x;
        return Print(s);
    }

    void CenterDisplay(tString s,REAL timeout=5,REAL r=1,REAL g=1,REAL b=1);

    // give a message to the user
    static bool Message(const tOutput& message, const tOutput& interpetation, REAL timeout = -1);

    // idle around a bit (return value true: abort whatever you're doing)
    static bool Idle();

    virtual tString ColorString(REAL r, REAL g, REAL b) const;

    static void RegisterMessageCallback(MessageCallback *callback);
    static void RegisterIdleCallback(IdleCallback *callback);

protected:
    static void RegisterBetterConsole(tConsole *better);

private:
    static tConsole *s_betterConsole;

    void PrintLine(tString const & s, int repetitions);
    virtual tConsole & DoPrint(const tString& s);
    virtual void DoCenterDisplay(const tString& s,REAL timeout=5,REAL r=1,REAL g=1,REAL b=1);
};

extern tConsole con;

//! Silences console output for the lifetime the instance
class tSuppressConsole : public tConsole
{
public:
    tSuppressConsole() { RegisterBetterConsole( this ); }
private:
    virtual tConsole & DoPrint( const tString &s ) { return *this; }
};

#endif
