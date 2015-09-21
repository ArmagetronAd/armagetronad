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

#ifndef		TRECORDERINTERNAL_H_INCLUDED
#define		TRECORDERINTERNAL_H_INCLUDED

// self include
#ifndef		TRECORDINGINTERNAL_H_INCLUDED
#include	"tRecorderInternal.h"
#endif

#include    "tString.h"

#include    <iostream>
#include    <string>

//! recording class
class tRecording
{
    friend class tRecordingBlockBase;

public:
    tRecording              ()        ;   //!< default constructor

    typedef std::ostream STREAM       ;   //!< stream typedef
protected:
    virtual ~tRecording     ()        ;   //!< destructor
private:
    // forbid copying
    tRecording                      ( tRecording const &            other       ) ;   //!< copy constructor
    tRecording&     operator =      ( tRecording const &            other       ) ;   //!< copy operator

    void BeginSection( char const * name )      ; //!< begins a new section
    virtual std::ostream& DoGetStream ()     =0 ; //!< returns the stream to write to

    static tRecording * currentRecording_;        //!< the currently running recording
};

//! playback class
class tPlayback
{
    friend class tPlaybackBlockBase;

public:
    tPlayback              ()        ;   //!< default constructor

    typedef std::istream STREAM      ;   //!< stream typedef
protected:
    virtual ~tPlayback     ()        ;   //!< destructor

    void    AdvanceSection ()        ;   //!< begin reading next section
private:
    // forbid copying
    tPlayback                      ( tPlayback const &            other       ) ;   //!< copy constructor
    tPlayback&     operator =      ( tPlayback const &            other       ) ;   //!< copy operator

    std::string const & GetNextSection()    const       ; //!< name of the next section
    virtual std::istream& DoGetStream ()           = 0  ; //!< returns the stream to read from

    std::string         nextSection_;           //!< the name of the next section
    static tPlayback *  currentPlayback_;       //!< the currently running playback
};


//! recording and playback implementation ( nothing but a stream container )
template< class BASE, class STREAM_IMP > class tRecorderImp: public BASE
{
public:
    template< class A > tRecorderImp( A a ): stream_( a ){ stream_.precision( 10 ); }
    template< class A, class B > tRecorderImp( A a, B b ): stream_( a, b ){ stream_.precision( 10 ); }
    ~tRecorderImp(){}

    //! init function for playback
    void InitPlayback()
    {
        BASE::AdvanceSection();
    }
private:
    //! Returns the stream
    virtual typename BASE::STREAM & DoGetStream()
    {
        return stream_;
    }

    STREAM_IMP stream_;
};

#endif // TRECORDING_H_INCLUDED
