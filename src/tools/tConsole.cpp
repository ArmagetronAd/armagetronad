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

#include "tConsole.h"
#include "tLocale.h"
#include "tSysTime.h"
#include <iostream>

// anchor for the linked list of filters; filters are added automatically from the constructor.
static tConsoleFilter* st_filterAnchor=0;

class tConsoleFilterComparator
{
public:
    static int Compare( const tConsoleFilter* A, const tConsoleFilter*B )
    {
        tASSERT( A && B );
        return B->GetPriority() - A->GetPriority();
    }
};

// *******************************************************************************************
// *
// *	tConsoleFilter
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

tConsoleFilter::tConsoleFilter( void )
        : tListItem< tConsoleFilter >( st_filterAnchor )
{
}

// *******************************************************************************************
// *
// *	~tConsoleFilter
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

tConsoleFilter::~tConsoleFilter( void )
{
}

// *******************************************************************************************
// *
// *	DoFilterOutput
// *
// *******************************************************************************************
//!
//!		@param	line	the line to filter
//!		@return			the filtered line
//!
// *******************************************************************************************

void tConsoleFilter::DoFilterLine( tString & line )
{
    // no filtering
}

// *******************************************************************************************
// *
// *	DoFilterElement
// *
// *******************************************************************************************
//!
//!		@param	element	the output item parameter ( i.e. player name ) to filter
//!		@return			the filtered parameter
//!
// *******************************************************************************************

void tConsoleFilter::DoFilterElement( tString & element )
{
    // no filtering
}

// *******************************************************************************************
// *
// *	DoGetPriority
// *
// *******************************************************************************************
//!
//!		@return
//!
// *******************************************************************************************

int tConsoleFilter::DoGetPriority( void ) const
{
    return 0;
}

static void FilterLine( tString& line )
{
    // no filtering to do
    if ( !st_filterAnchor )
        return;

#ifndef WIN32 // MSVC++ does not eat this code, but that's not tragic right now.
    // sort static filters according to priority
    static bool sorted = false;
    if ( !sorted )
        st_filterAnchor->Sort< tConsoleFilterComparator >();
#endif

    // remove end of line
    bool hasEOL = false;
    int eol = line.Size()-1;
    while ( eol >= 0 && line[eol] == 0 )
        eol--;
    if ( eol >= 0 && line[eol] == '\n' )
    {
        hasEOL = true;
        line = line.SubStr( 0, eol );
    }

    // filter string
    tConsoleFilter* filter = st_filterAnchor;
    while ( filter )
    {
        filter->FilterLine( line );
        filter = filter->Next();
    }

    // add end of line
    if ( hasEOL )
        line += '\n';
}

tConsole *tConsole::s_betterConsole=NULL;

tConsole::~tConsole(){
    if (s_betterConsole == this)
        s_betterConsole = NULL;
}

// stored line
static tString line_("");

void tConsole::PrintLine(tString const & line, int repetitions)
{
    if( repetitions > 1 )
    {
        // find the true line end. Sometimes, there are trailing color codes.
        char const * lineEnd = strstr( line, "\n" );
        int length = line.Len()-2;
        if( lineEnd )
        {
            length = lineEnd - (char const *)line;
        }
        std::stringstream combined;
        combined << line.SubStr(0, length) << " 0xffffff" << tOutput("$console_repetition", repetitions );

        PrintLine( combined.str(), 1 );
    }
    else
    {
        // print
        if (s_betterConsole)
            s_betterConsole->DoPrint( line );
        else
            DoPrint( line );
    }
}

tConsole & tConsole::Print(tString const & s)
{
    // append to line
    line_ += s;

    // determine if a newline was received
    bool newline = false;
    for ( int i = s.Size()-1; i>=0; --i )
        if ( s(i) == '\n' )
            newline = true;

    if ( newline )
    {
#ifdef DEDICATED
        tDelay(1000);
#endif

        // filter
        FilterLine( line_ );

        // check for repetitions
        static tString lastLine("not the last line");
        static int repetitions = 0;
        static int threshold = 1;
        if( lastLine != line_ )
        {
            if( repetitions > 0 )
            {
                PrintLine( lastLine, repetitions );
            }

            repetitions = 0;
            threshold = 1;
            lastLine = line_;
        }
        else
        {
            repetitions++;
            if( threshold < 10 || repetitions >= threshold )
            {
                PrintLine( line_, repetitions );
 
                if( threshold < 10 )
                {
                    threshold++;
                }
                else
                {
                    threshold*=10;
                }
                repetitions = 0;
            }

            line_ = "";
            return *this;
        }

        PrintLine( line_, 1 );

        line_ = "";
    }

    return *this;
}

void tConsole::CenterDisplay(tString s,REAL timeout,REAL r,REAL g,REAL b)
{
    FilterLine(s);

    if (s_betterConsole)
        s_betterConsole->DoCenterDisplay(s,timeout,r,g,b);
    else
        DoCenterDisplay(s,timeout,r,g,b);
}

tConsole & tConsole::DoPrint(const tString& s){
    std::cout << tColoredString::RemoveColors(s);
    std::cout.flush();
    return *this;
}

void tConsole::DoCenterDisplay(const tString& s,REAL timeout,REAL r,REAL g,REAL b){
    std::cout << s;
    std::cout.flush();
}

void tConsole::RegisterBetterConsole(tConsole *better){
    s_betterConsole = better;
}

tString tConsole::ColorString(REAL r, REAL g, REAL b) const{
    if (s_betterConsole)
        return s_betterConsole->ColorString(r,g,b);
    else
        return tString("");
}


static tConsole::MessageCallback *s_messageCallback = NULL;
void tConsole::RegisterMessageCallback(MessageCallback *a_callback)
{
    s_messageCallback = a_callback;
}

bool tConsole::Message(const tOutput& message, const tOutput& interpretation, REAL timeout){
    if (s_messageCallback)
        return (*s_messageCallback)(message, interpretation, timeout);
    else
    {
        con << tString(message) << ":\n";
        con << tString(interpretation) << '\n';
        return true;
    }
}

static tConsole::IdleCallback *s_idleCallback = NULL;
void tConsole::RegisterIdleCallback(IdleCallback *a_callback)
{
    s_idleCallback = a_callback;
}

bool tConsole::Idle( bool processInput ){
    if (s_idleCallback)
    {
        return (*s_idleCallback)( processInput );
    }
    else
    {
        return false;
    }
}

tConsole con;


