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

#ifndef     TRECORDER_H_INCLUDED
#define     TRECORDER_H_INCLUDED

#ifdef DEBUG
#ifndef DEBUG_DIFFERENCE
#define DEBUG_DIFFERENCE
#endif
#endif

// self include
#ifndef     TRECORDER_H_INCLUDED
#include    "tRecorder.h"
#endif

#include    "tString.h"

// #include    "tRecorderInternal.h"

// *****************************************************************************
// Helpers required to work around VisualC's iability to handle out-of-line
// template members. Just ignore them.
// *****************************************************************************

//! recording funtions that take one piece of data
template< class BLOCK >
class tRecorderTemplate1
{
public:
    static bool Archive( bool strict, char const * section );                  //!< Archives an empty section
};

//! recording funtions that take two pieces of data
template< class BLOCK, typename DATA >
class tRecorderTemplate2
{
public:
    static bool Archive( bool strict, char const * section, DATA data );                  //!< Archives a section with one piece of data
};

//! recording funtions that take two pieces of data and a block
template< class BLOCK, typename DATA1, typename DATA2 >
class tRecorderTemplate3
{
public:
    static bool Archive( bool strict, char const * section, DATA1 data1, DATA2 data2 );  //!< Archives a section with two pieces of data
};

// *****************************************************************************
// tRecorder: simple recording interface. Most users will only need this.
// *****************************************************************************

class tRecordingBlock;
class tPlaybackBlock;

//! non-templated base class of recorder
class tRecorderBase
{
public:
    static bool IsRecording();                          //!< returns whether there is a recording running
    static bool IsPlayingBack();                        //!< returns whether there is a playback running
    static bool IsRunning();                            //!< returns whether recording or playback are running
    static void StopRecording();                        //!< stops recording
};

//! simple interface to recording functionality
class tRecorder: public tRecorderBase
{
public:
    static bool Record( char const * section );         //!< Records an empty section
    static bool Playback( char const * section );       //!< Plays back an empty section
    static bool PlaybackStrict( char const * section ); //!< Plays back an empty section, making sure it exists in the recording

    //! Records a section with one piece of data
    template< class DATA >
    static bool Record  ( char const * section, DATA const & data )
    { return tRecorderTemplate2< tRecordingBlock, DATA const & >::Archive( false, section, data ); }

    //! Plays back a section with one piece of data
    template< class DATA >
    static bool Playback( char const * section, DATA & data )
    { return tRecorderTemplate2< tPlaybackBlock, DATA & >::Archive( false, section, data ); }

    //! Plays back a section with one piece of data, making sure it exists in the recording
    template< class DATA >
    static bool PlaybackStrict( char const * section, DATA & data )
    { return tRecorderTemplate2< tPlaybackBlock, DATA & >::Archive( true, section, data ); }

    //! Records a section with two pieces of data
    template< class DATA1, class DATA2 >
    static bool Record( char const * section, DATA1 const & data1, DATA2 const & data2 )
    { return tRecorderTemplate3< tRecordingBlock, DATA1 const & , DATA2 const & >::Archive( false, section, data1, data2 ); }

    //!< Plays back a section with two pieces of data
    template< class DATA1, class DATA2 >
    static bool Playback( char const * section, DATA1 & data1, DATA2 & data2 )
    { return tRecorderTemplate3< tPlaybackBlock, DATA1 &, DATA2 & >::Archive( false, section, data1, data2 ); }

    //! Plays back a section with two pieces of data, making sure it exists in the recording
    template< class DATA1, class DATA2 >
    static bool PlaybackStrict( char const * section, DATA1 & data1, DATA2 & data2 )
    { return tRecorderTemplate3< tPlaybackBlock, DATA1 &, DATA2 & >::Archive( true, section, data1, data2 ); }
};

class tPath;

//! text file loading recorder
class tTextFileRecorder: public tRecorderBase
{
public:
    tTextFileRecorder();
    ~tTextFileRecorder();

    //! opens a text file for reading
    bool Open( tPath const & searchPath, char const * fileName );

    //! constructor, opening a file directly (status can be queried later with EndOfFile)
    tTextFileRecorder( tPath const & searchPath, char const * fileName );

    //! end of file query
    bool EndOfFile() const;

    //! read one line, give it back as a stream
    std::string GetLine();
private:
    // disable copying
    tTextFileRecorder(tTextFileRecorder const &);
    tTextFileRecorder & operator = (tTextFileRecorder const &);

    std::ifstream * stream_; //! the real stream to read from
    bool eof_;               //! flag indicating end of file
};

//! typedef for easier handling
// typedef tRecorderTemplate<int> tRecorder;

// *****************************************************************************
// helper classes for converting data before it gets archived
// *****************************************************************************

//! type modifying class mapping types in memory to types to stream
template< class T > struct tTypeToStream
{
    typedef T TOSTREAM;             //!< type to put into the stream
    typedef int DUMMYREQUIRED;      //!< change this type to "int *" to indicate the conversion is required
};

//! macro declaring that type TYPE should be converted to type STREAM before
// recording (and back after playback) by specializing the tTypeToStream class template
#define tRECORD_AS( TYPE, STREAM ) \
template<> struct tTypeToStream< TYPE > \
{ \
    typedef STREAM TOSTREAM; \
  typedef int * DUMMYREQUIRED; \
} \

//! macro for recording enums: convert them to int.
#define tRECORDING_ENUM( TYPE ) tRECORD_AS( TYPE, int )

// record chars as ints for human readability
tRECORD_AS( char, int );
tRECORD_AS( unsigned char, int );

//! persistable string class
class tLineString: public tString
{
public:
    tLineString( tString const & ); //! copy constructor
    tLineString();                  //! default constructor
    ~tLineString();                 //! destructor
};

//! persistent string writing operator
std::ostream & operator << ( std::ostream & s, tLineString const & line );

//! persistent string reading operator
std::istream & operator >> ( std::istream & s, tLineString & line );

//! record strings as line strings
tRECORD_AS( tString, tLineString );

// *****************************************************************************
// support for debugging only recording (for intermediate data)
// *****************************************************************************

//! debug recording synchronization test class
class tRecorderSyncBase
{
public:
    //! returns the current debug level of the playback
    static int GetDebugLevelPlayback();

    //! returns the current debug level of the recording
    static int GetDebugLevelRecording();
};

//! debug recording synchronization test class
template< class DATA >
class tRecorderSync: public tRecorderSyncBase
{
public:
    //! Archives a bit of data for reference
    static void Archive( char const * section, int debugLevel, DATA & data );

private:
    //! Calculates a suitable difference between two bits of data
    static float GetDifference( DATA const & a, DATA const & b );
};

// *****************************************************************************
// another helper class
// *****************************************************************************

//! class taking the ugly implementation part of tRecordingBlock and tPlaybackBlock
template < class DATA > class tRecorderBlockHelper
{
public:
    static void Write ( std::ostream & stream, DATA const & data, int nodummyrequired );        //!< writes a piece of data using a dummy
    static void Write ( std::ostream & stream, DATA const & data, int * dummyrequired );        //!< writes a piece of data using a dummy

    static void Read  ( std::istream & stream, DATA & data, int nodummyrequired );        //!< reads a piece of data using a dummy
    static void Read  ( std::istream & stream, DATA & data, int * dummyrequired );        //!< reads a piece of data using a dummy
};

// *****************************************************************************
// recording blocks for stuffing more data inside a section than two elements
// *****************************************************************************

class tRecording;

//! a block of data to record (for more advanced data storage ), base class
class tRecordingBlockBase
{
public:
    bool Initialize( char const * section, tRecording * recording );  //!< initializes this for recording
    bool Initialize( char const * section );                          //!< initializes this for recording

    void Separator();                                                 //!< separates two recorded elements (more than usual)
    static tRecording * GetArchive();                                 //!< returns the active recording
protected:
    tRecordingBlockBase();                                            //!< default constructor
    ~tRecordingBlockBase();                                           //!< ends a recording block

    std::ostream & GetRecordingStream() const;                        //!< returns the stream to record to

    bool separate_;                                                   //!< flag indicating whether a separation is needed before the next data element
private:
    tRecording * recording_;                                          //!< the recording to record to
};

//! a block of data to record (for more advanced data storage ), implementation
class tRecordingBlock: public tRecordingBlockBase
{
public:
    tRecordingBlock();                                                     //!< default constructor
    ~tRecordingBlock();                                                    //!< ends a recording block

    //! writes a piece of data
    template< class T > tRecordingBlock & operator << ( T const & data ) { return Write( data ); }

    //! writes a piece of data
    template< class T > tRecordingBlock & Archive     ( T const & data ) { return Write( data ); }

    //! writes a piece of data
    template< class T > tRecordingBlock & Write       ( T const & data )
    {
        // get stream
        std::ostream & stream = GetRecordingStream();

        // add separator
        if ( separate_ )
            stream << ' ';
        separate_ = true;

        // delegate to dummy using or dummyless function
        typename tTypeToStream< T >::DUMMYREQUIRED dummyRequired = 0;
        tRecorderBlockHelper< T >::Write( stream, data, dummyRequired );

        return *this;
    }
};

// *****************************************************************************

class tPlayback;

//! a block of data to play back.
class tPlaybackBlockBase
{
public:
    bool Initialize( char const * section, tPlayback * playback );    //!< initializes this for recording
    bool Initialize( char const * section );                          //!< initializes this for recording

    void Separator() const;                                           //!< separate output (Nothing done here while playing back)
    static tPlayback * GetArchive();                                  //!< returns the active playback
protected:
    tPlaybackBlockBase();                                             //!< default constructor
    ~tPlaybackBlockBase();                                            //!< ends a playback block

    std::istream & GetPlaybackStream() const;                         //!< returns the stream to playback from

private:
    tPlayback * playback_;                                             //!< the playback to read from
};

//! a block of data to play back.
class tPlaybackBlock: public tPlaybackBlockBase
{
public:
    tPlaybackBlock();                                         //!< default constructor
    ~tPlaybackBlock();                                        //!< ends a playback block

    //!< reads a piece of data
    template< class T > tPlaybackBlock & operator >> ( T & data ){ return Read( data ); }

    //!< reads a piece aof data
    template< class T > tPlaybackBlock & Archive     ( T & data ){ return Read( data ); }

    //!< reads a piece of data
    template< class T > tPlaybackBlock & Read        ( T & data )
    {
        // delegate to dummy using or dummyless function
        typename tTypeToStream< T >::DUMMYREQUIRED dummyrequired = 0;
        tRecorderBlockHelper< T >::Read( GetPlaybackStream(), data, dummyrequired );

        return *this;
    }
};

//! typedefs for easier handling
// typedef tRecordingBlockTemplate< int > tRecordingBlock;
// typedef tPlaybackBlockTemplate< int > tPlaybackBlock;

// Note: both tRecordingBlock and tPlaybackBlock share common mebe fuctions. See uInputQueue.cpp
// (or the tRecorderTemplate::Archive implementation) how to
// exploit this with templates to make sure you read exaclty the same data as you write.

// *****************************************************************************
// * Implementation
// *****************************************************************************

/*

// *******************************************************************************************
// *
// *    Record
// *
// *******************************************************************************************
//!
//!     @param  section the name of the section to record or play back
//!     @param  data            the data to archive
//!     @return   true on success
//!
// *******************************************************************************************

template< class DUMMY  >
template<  class DATA >
bool tRecorderTemplate< DUMMY >::Record( char const * section, DATA const & data )
{
    // delegate
    return Archive< tRecordingBlock >( false, section, data );
}

// *******************************************************************************************
// *
// *    Playback
// *
// *******************************************************************************************
//!
//!     @param  section the name of the section to record or play back
//!     @param  data            the data to archive
//!     @return   true on success
//!
// *******************************************************************************************

template< class DUMMY  >
template<  class DATA >
bool tRecorderTemplate< DUMMY >::Playback( char const * section, DATA & data )
{
    // delegate
    return Archive< tPlaybackBlock >( false, section, data );
}

// *******************************************************************************************
// *
// *    PlaybackStrict
// *
// *******************************************************************************************
//!
//!     @param  section the name of the section to record or play back
//!     @param  data            the data to archive
//!     @return   true on success
//!
// *******************************************************************************************

template< class DUMMY  >
template<  class DATA >
bool tRecorderTemplate< DUMMY >::PlaybackStrict( char const * section, DATA & data )
{
    // delegate
    return Archive< tPlaybackBlock >( true, section, data );
}

// *******************************************************************************************
// *
// *    Record
// *
// *******************************************************************************************
//!
//!     @param  section the name of the section to record or play back
//!     @param  data1    first bit of data to archive
//!     @param  data2    second bit of data to archive
//!     @return   true on success
//!
// *******************************************************************************************

template< class DUMMY  >
template<  class DATA1, class DATA2 >
bool tRecorderTemplate< DUMMY >::Record( char const * section, DATA1 const & data1, DATA2 const & data2 )
{
    // delegate
    return Archive< tRecordingBlock >( false, section, data1, data2 );
}

// *******************************************************************************************
// *
// *    Playback
// *
// *******************************************************************************************
//!
//!     @param  section the name of the section to record or play back
//!     @param  data1    first bit of data to archive
//!     @param  data2    second bit of data to archive
//!     @return   true on success
//!
// *******************************************************************************************

template< class DUMMY  >
template<  class DATA1, class DATA2 >
bool tRecorderTemplate< DUMMY >::Playback( char const * section, DATA1 & data1, DATA2 & data2 )
{
    // delegate
    return Archive< tPlaybackBlock >( false, section, data1, data2 );
}

// *******************************************************************************************
// *
// *    PlaybackStrict
// *
// *******************************************************************************************
//!
//!     @param  section the name of the section to record or play back
//!     @param  data1    first bit of data to archive
//!     @param  data2    second bit of data to archive
//!     @return   true on success
//!
// *******************************************************************************************

template< class DUMMY  >
template<  class DATA1, class DATA2 >
bool tRecorderTemplate< DUMMY >::PlaybackStrict( char const * section, DATA1 & data1, DATA2 & data2 )
{
    // delegate
    return Archive< tPlaybackBlock >( true, section, data1, data2 );
}

// *******************************************************************************************
// *    
// *    Archive
// *    
// *******************************************************************************************
//!     
//!     @param  strict  
//!     @param  section the name of the section to record or play back  
//!     @return   true on success       
//!     
// *******************************************************************************************

template< class DUMMY  >
template<  class BLOCK >
bool tRecorderTemplate< DUMMY >::Archive( bool strict, char const * section )
{
    // create recording/playback block
    BLOCK block;

    // initialize
    if ( block.Initialize( section ) )
    {
        // return success
        return true;
    }

    // report failure
    tASSERT( !strict  || !BLOCK::GetArchive() );
    return false;
}

// *******************************************************************************************
// *    
// *    Archive
// *    
// *******************************************************************************************
//!     
//!     @param  strict  
//!     @param  section the name of the section to record or play back  
//!     @param  data    
//!     @return   true on success       
//!     
// *******************************************************************************************

template< class DUMMY  >
template<  class BLOCK, class DATA >
bool tRecorderTemplate< DUMMY >::Archive( bool strict, char const * section, DATA & data )
{
    // create recording/playback block
    BLOCK block;

    // initialize
    if ( block.Initialize( section ) )
    {
        // successfully initialized: archive data
        block.Archive( data );

        // return success
        return true;
    }

    // report failure
    tASSERT( !strict || !BLOCK::GetArchive() );
    return false;
}

// *******************************************************************************************
// *    
// *    Archive
// *    
// *******************************************************************************************
//!     
//!     @param  strict  
//!     @param  section the name of the section to record or play back  
//!     @param  data1    first bit of data to archive   
//!     @param  data2    second bit of data to archive   
//!     @return   true on success       
//!     
// *******************************************************************************************

template< class DUMMY  >
template<  class BLOCK, class DATA1, class DATA2 >
bool tRecorderTemplate< DUMMY >::Archive( bool strict, char const * section, DATA1 & data1, DATA2 & data2 )
{
    // create recording/playback block
    BLOCK block;

    // initialize
    if ( block.Initialize( section ) )
    {
        // successfully initialized: archive data
        block.Archive( data1 ).Archive( data2 );

        // return success
        return true;
    }

    // report failure
    tASSERT( !strict  || !BLOCK::GetArchive() );
    return false;
}

// *******************************************************************************************
// *******************************************************************************************
// *******************************************************************************************
// *******************************************************************************************

// *******************************************************************************************
// *
// *    tRecordingBlockTemplate
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

template< class DUMMY >
tRecordingBlockTemplate< DUMMY >::tRecordingBlockTemplate( void )
{
}

// *******************************************************************************************
// *
// *    ~tRecordingBlockTemplate
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

template< class DUMMY >
tRecordingBlockTemplate< DUMMY >::~tRecordingBlockTemplate( void )
{
}

// *******************************************************************************************
// *
// *    operator <<
// *
// *******************************************************************************************
//!
//!     @param  data            the data to archive
//!     @return     reference to this for chaining
//!
// *******************************************************************************************

template< class DUMMY  >
template<  class T >
tRecordingBlockTemplate< DUMMY > & tRecordingBlockTemplate< DUMMY >::operator <<( T const & data )
{
    // delegate
    return Write( data );
}

// *******************************************************************************************
// *
// *    Archive
// *
// *******************************************************************************************
//!
//!     @param  data            the data to archive
//!     @return     reference to this for chaining
//!
// *******************************************************************************************

template< class DUMMY  >
template<  class T >
tRecordingBlockTemplate< DUMMY > & tRecordingBlockTemplate< DUMMY >::Archive( T const & data )
{
    // delegate
    return Write( data );
}

// *******************************************************************************************
// *
// *    Write
// *
// *******************************************************************************************
//!
//!     @param  data            the data to archive
//!     @return     reference to this for chaining
//!
// *******************************************************************************************

template< class DUMMY  >
template<  class T >
tRecordingBlockTemplate< DUMMY > & tRecordingBlockTemplate< DUMMY >::Write( T const & data )
{
    // get stream
    std::ostream & stream = GetRecordingStream();

    // add small separator
    if (separate_)
        stream << ' ';
    separate_ = true;

    // delegate to dummy using or dummyless function
    typename tTypeToStream< T >::DUMMYREQUIRED dummyrequired = 0;
    Write( data, dummyrequired );

    return *this;
}

// *******************************************************************************************
// *
// *    Write
// *
// *******************************************************************************************
//!
//!     @param  data            the data to archive
//!     @param  nodummyrequired dummy parameter indicating by type that no conversion is required
//!
// *******************************************************************************************

template< class DUMMY  >
template<  class T >
void tRecordingBlockTemplate< DUMMY >::Write( T const & data, int nodummyrequired )
{
    // get stream
    std::ostream & stream = GetRecordingStream();

    // add small separator
    stream << ' ';
    
    // write
    stream << data;

}

// *******************************************************************************************
// *
// *    Write
// *
// *******************************************************************************************
//!
//!     @param  data            the data to archive
//!     @param  dummyrequired   dummy parameter indicating by type that conversion is required
//!
// *******************************************************************************************

template< class DUMMY  >
template<  class T >
void tRecordingBlockTemplate< DUMMY >::Write( T const & data, int * dummyrequired )
{
    // get stream
    std::ostream & stream = GetRecordingStream();
    
    // write ( converted )
    typedef typename tTypeToStream< T >::TOSTREAM TOSTREAM;
    TOSTREAM dummy = static_cast< TOSTREAM >( data );
    stream << dummy;
}

// *******************************************************************************************
// *******************************************************************************************
// *******************************************************************************************
// *******************************************************************************************

// *******************************************************************************************
// *
// *    tPlaybackBlockTemplate
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

template< class DUMMY >
tPlaybackBlockTemplate< DUMMY >::tPlaybackBlockTemplate( void )
{
}

// *******************************************************************************************
// *
// *    ~tPlaybackBlockTemplate
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

template< class DUMMY >
tPlaybackBlockTemplate< DUMMY >::~tPlaybackBlockTemplate( void )
{
}

// *******************************************************************************************
// *
// *    operator >>
// *
// *******************************************************************************************
//!
//!     @param  data            the data to archive
//!     @return     reference to this for chaining
//!
// *******************************************************************************************

template< class DUMMY  >
template<  class T >
tPlaybackBlockTemplate< DUMMY > & tPlaybackBlockTemplate< DUMMY >::operator >>( T & data )
{
    // delegate
    return Read( data );
}

// *******************************************************************************************
// *
// *    Archive
// *
// *******************************************************************************************
//!
//!     @param  data            the data to archive
//!     @return     reference to this for chaining
//!
// *******************************************************************************************

template< class DUMMY  >
template<  class T >
tPlaybackBlockTemplate< DUMMY > & tPlaybackBlockTemplate< DUMMY >::Archive( T & data )
{
    // delegate
    return Read( data );
}

// *******************************************************************************************
// *
// *    Read
// *
// *******************************************************************************************
//!
//!     @param  data            the data to archive
//!     @return     reference to this for chaining
//!
// *******************************************************************************************

template< class DUMMY  >
template<  class T >
tPlaybackBlockTemplate< DUMMY > & tPlaybackBlockTemplate< DUMMY >::Read( T & data )
{
    // delegate to dummy using or dummyless function
    typename tTypeToStream< T >::DUMMYREQUIRED dummyrequired = 0;
    Read( data, dummyrequired );

    return *this;
}

// *******************************************************************************************
// *
// *    Read
// *
// *******************************************************************************************
//!
//!     @param  data            the data to archive
//!     @param  nodummyrequired dummy parameter indicating by type that no conversion is required
//!
// *******************************************************************************************

template< class DUMMY  >
template<  class T >
void tPlaybackBlockTemplate< DUMMY >::Read( T & data, int nodummyrequired )
{
    // get stream
    std::istream & stream = GetPlaybackStream();
    tASSERT( stream.good() );

    // read
    stream >> data;
}

// *******************************************************************************************
// *
// *    Read
// *
// *******************************************************************************************
//!
//!     @param  data            the data to archive
//!     @param  dummyrequired   dummy parameter indicating by type that conversion is required
//!
// *******************************************************************************************

template< class DUMMY  >
template<  class T >
void tPlaybackBlockTemplate< DUMMY >::Read( T & data, int * dummyrequired )
{
    // get stream
    std::istream & stream = GetPlaybackStream();
    tASSERT( stream.good() );

    // read ( with conversion )
    typedef typename tTypeToStream< T >::TOSTREAM TOSTREAM;
    TOSTREAM dummy ;
    stream >> dummy;
    data = static_cast< T >( dummy );
}

*/

// ******************************************************************************************
// *
// *	Archive
// *
// ******************************************************************************************
//!
//!		@param	section	    section to archive to
//!		@param	level  	    debug level of the data (lower levels get archived sooner)
//!		@param	data	    data to archive
//!
// ******************************************************************************************

template< class DATA >
void tRecorderSync< DATA >::Archive( char const * section, int level, DATA & data )
{
    // see if it is really a DEBUG only section
    tASSERT( section && *section == '_' );

    if ( level <= GetDebugLevelPlayback() )
    {
        DATA copy = data;

        // read data from archive
        if ( tRecorder::PlaybackStrict( section, copy ) )
        {
#ifdef DEBUG_DIFFERENCE
            // determine difference
            REAL diff = GetDifference( data, copy );

            static REAL alarmDiff = EPS;
            if ( diff > alarmDiff )
            {
                alarmDiff = diff * 2;
                REAL st_GetDifference( REAL a, REAL b);
                REAL st_GetDifference( int a, int b);
                REAL st_GetDifference( tString const & a, tString const & b );
                std::cout << "Syncing difference found: " << data << "!=" << copy << " by " << diff << "\n";
                st_Breakpoint();
            }
#endif

            // restore data, hoping that the playback can take the little bump
            if ( level <= GetDebugLevelRecording() )
            {
                data = copy;
            }
        }
        else if ( tRecorder::IsPlayingBack() )
        {
            std::cout << "Syncing difference found: expected " << section << ".\n";

            st_Breakpoint();
        }
    }

    // archive data
    if ( level <= GetDebugLevelRecording() )
        tRecorder::Record( section, data );
}

REAL st_GetDifference( REAL a, REAL b);
REAL st_GetDifference( int a, int b);
REAL st_GetDifference( unsigned int a, unsigned int b);
REAL st_GetDifference( long unsigned int a, long unsigned int b);
REAL st_GetDifference( tString const & a, tString const & b );

// ******************************************************************************************
// *
// *	GetDifference
// *
// ******************************************************************************************
//!
//!		@param	a	        object a
//!		@param	b	        object b
//!		@return		        |a-b|, interpreted as appropriate
//!
// ******************************************************************************************

template< class DATA >
float tRecorderSync< DATA >::GetDifference( DATA const & a, DATA const & b )
{
    return st_GetDifference( a, b );
}

// ******************************************************************************************
// *
// *	Archive
// *
// ******************************************************************************************
//!
//!     @param  strict  true if the success should be asserted
//!     @param  section the name of the section to record or play back
//!     @return         true on success
//!
// ******************************************************************************************

template< class BLOCK >
bool tRecorderTemplate1< BLOCK >::Archive( bool strict, char const * section )
{
    // create recording/playback block
    BLOCK block;

    // initialize
    if ( block.Initialize( section ) )
    {
        // return success
        return true;
    }

    // report failure
    tASSERT( !strict  || !BLOCK::GetArchive() );
    return false;
}

// ******************************************************************************************
// *
// *	Archive
// *
// ******************************************************************************************
//!
//!     @param  strict  true if the success should be asserted
//!     @param  section the name of the section to record or play back
//!     @param  data    bit of data to archive
//!     @return   true on success
//!
// ******************************************************************************************

template< class BLOCK, typename DATA >
bool tRecorderTemplate2< BLOCK, DATA >::Archive( bool strict, char const * section, DATA data )
{
    // create recording/playback block
    BLOCK block;

    // initialize
    if ( block.Initialize( section ) )
    {
        // successfully initialized: archive data
        block.Archive( data );

        // return success
        return true;
    }

    // report failure
    tASSERT( !strict || !BLOCK::GetArchive() );
    return false;
}

// ******************************************************************************************
// *
// *	Archive
// *
// ******************************************************************************************
//!
//!     @param  strict  true if the success should be asserted
//!     @param  section the name of the section to record or play back
//!     @param  data1    first bit of data to archive
//!     @param  data2    second bit of data to archive
//!     @return   true on success
//!
// ******************************************************************************************

template< class BLOCK, typename DATA1, typename DATA2 >
bool tRecorderTemplate3< BLOCK, DATA1, DATA2 >::Archive( bool strict, char const * section, DATA1 data1, DATA2 data2 )
{
    // create recording/playback block
    BLOCK block;

    // initialize
    if ( block.Initialize( section ) )
    {
        // successfully initialized: archive data
        block.Archive( data1 ).Archive( data2 );

        // return success
        return true;
    }

    // report failure
    tASSERT( !strict  || !BLOCK::GetArchive() );
    return false;
}

// ******************************************************************************************
// *
// *	Write
// *
// ******************************************************************************************
//!
//!		@param	stream	        the stream to write to
//!     @param  data            the data to archive
//!     @param  nodummyrequired dummy parameter indicating by type that no conversion is required
//!
// ******************************************************************************************

template< class DATA >
void tRecorderBlockHelper< DATA >::Write( std::ostream & stream, DATA const & data, int nodummyrequired )
{
    // write
    stream << data;
}

// ******************************************************************************************
// *
// *	Write
// *
// ******************************************************************************************
//!
//!		@param	stream	        the stream to write to
//!     @param  data            the data to archive
//!     @param  dummyrequired   dummy parameter indicating by type that conversion is required
//!
// ******************************************************************************************

template< class DATA >
void tRecorderBlockHelper< DATA >::Write( std::ostream & stream, DATA const & data, int * dummyrequired )
{
    // write ( converted )
    typedef typename tTypeToStream< DATA >::TOSTREAM TOSTREAM;
    TOSTREAM dummy = static_cast< TOSTREAM >( data );
    stream << dummy;
}

// ******************************************************************************************
// *
// *	Read
// *
// ******************************************************************************************
//!
//!		@param	stream	        the stream to read from
//!     @param  data            the data to archive
//!     @param  nodummyrequired dummy parameter indicating by type that no conversion is required
//!
// ******************************************************************************************

template< class DATA >
void tRecorderBlockHelper< DATA >::Read( std::istream & stream, DATA & data, int nodummyrequired )
{
    tASSERT( stream.good() );

    // read
    stream >> data;
}

// ******************************************************************************************
// *
// *	Read
// *
// ******************************************************************************************
//!
//!		@param	stream	        the stream to read from
//!     @param  data            the data to archive
//!     @param  dummyrequired   dummy parameter indicating by type that conversion is required
//!
// ******************************************************************************************

template< class DATA >
void tRecorderBlockHelper< DATA >::Read( std::istream & stream, DATA & data, int * dummyrequired )
{
    tASSERT( stream.good() );

    // read ( with conversion )
    typedef typename tTypeToStream< DATA >::TOSTREAM TOSTREAM;
    TOSTREAM dummy ;
    stream >> dummy;
    data = static_cast< DATA >( dummy );
}

#endif // TRECORDING_H_INCLUDED
