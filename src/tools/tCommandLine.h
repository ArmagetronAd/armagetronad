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

#ifndef		TCOMMANDLINE_H_INCLUDED
#define		TCOMMANDLINE_H_INCLUDED

// self include
#ifndef		TCOMMANDLINE_H_INCLUDED
#include	"tCommandLine.h"
#endif

#include "tString.h"

#include "tLinkedList.h"

//! entry point for command line parsing
struct tCommandLineData
{
    tString* programVersion_;
    tString  name_;

    bool Analyse(int argc,char **argv ); //!< analyse the command line
    bool Execute();                      //!< execute the parsed commands

    //! constructor
    tCommandLineData()
            : doc_(false)
    {
        programVersion_ = NULL;
    }

private:
    bool doc_; //!< flag indicating whether documentation should be printed
};

//! command line data
struct tCommandLineParser
{
public:
    bool GetSwitch( char const * option, char * option_short=NULL );                      //! Tests whether the current argument is the given switch
    bool GetOption( tString & target, char const * option, char * option_short=NULL );    //! Tests whether the current argument is the given option and extracts the value

    bool End() const;                                                                     //! Tests whether the command line parsing is done

    const char * Executable() const;                                                      //! Returns the full path to the executable
    const char * Current() const;                                                         //! Returns the current option
    void Advance();                                                                       //! Advances to the next option

    tCommandLineParser( int argc, char ** argv );                                         //! constructor
private:
    tCommandLineParser();         //! constructor

    const int argc;              //! total number of arguments
    char * * argv;               //! pointer array to arguments
    int index;                   //! the index currently analyzed option
};


//! command line analyzing object
class tCommandLineAnalyzer: public tListItem< tCommandLineAnalyzer >
{
public:
    tCommandLineAnalyzer(); //!default constructor
    ~tCommandLineAnalyzer(); //!destructor

    inline void Initialize( tCommandLineParser & parser ); //! Analyzes the command line
    inline bool Analyze( tCommandLineParser & parser );    //! Analyzes the command line option
    inline void Help( std::ostream & s );                  //! Prints option help
private:
    virtual void DoInitialize( tCommandLineParser & parser );     //! Analyzes the command line option
    virtual bool DoAnalyze( tCommandLineParser & parser ) = 0;    //! Analyzes the command line option
    virtual void DoHelp( std::ostream & s ) = 0;                  //! Prints option help
};

// *******************************************************************************************
// *
// *   Initialize
// *
// *******************************************************************************************
//!
//!        @param  parser  the parser containing the command line information
//!
// *******************************************************************************************

void tCommandLineAnalyzer::Initialize( tCommandLineParser & parser )
{
    DoInitialize( parser );
}

// *******************************************************************************************
// *
// *   Analyze
// *
// *******************************************************************************************
//!
//!        @param  parser  the parser containing the command line information
//!      @return         true if an option was read
//!
// *******************************************************************************************

bool tCommandLineAnalyzer::Analyze( tCommandLineParser & parser )
{
    return DoAnalyze( parser );
}

// *******************************************************************************************
// *
// *   Help
// *
// *******************************************************************************************
//!
//!        @param  s   stream to write the help to
//!
// *******************************************************************************************

void tCommandLineAnalyzer::Help( std::ostream & s )
{
    DoHelp( s );
}

#endif // TCOMMANDLINE_H_INCLUDED
