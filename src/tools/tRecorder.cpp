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

// declaration
#ifndef		TRECORDER_H_INCLUDED
#include	"tRecorder.h"
#endif

#include    "tConfiguration.h"
#include    "tDirectories.h"
#include    "tRecorderInternal.h"
#include    "tSysTime.h"
#include    <vector>

#undef 	INLINE_DEF
#define INLINE_DEF

// *****************************************************************************************
// *
// *   IsRecording
// *
// *****************************************************************************************
//!
//!        @return     true if a recording is running
//!
// *****************************************************************************************

bool tRecorderBase::IsRecording( void )
{
    return tRecordingBlock::GetArchive();
}

// *****************************************************************************************
// *
// *   IsPlayingBack
// *
// *****************************************************************************************
//!
//!        @return     true if a playback is running
//!
// *****************************************************************************************

bool tRecorderBase::IsPlayingBack( void )
{
    return tPlaybackBlock::GetArchive();
}

// *****************************************************************************************
// *
// *   IsRunning
// *
// *****************************************************************************************
//!
//!        @return     true if recording or playback are running
//!
// *****************************************************************************************

bool tRecorderBase::IsRunning( void )
{
    return IsRecording() || IsPlayingBack();
}

// *****************************************************************************************
// *
// *   Stop
// *
// *****************************************************************************************
//!
//!
// *****************************************************************************************

void tRecorderBase::StopRecording( void )
{
    return tRecording::Stop();
}
static void st_StopRecording(std::istream &)
{
    tRecorderBase::StopRecording();
}

static tConfItemFunc snm("STOP_RECORDING",&st_StopRecording);


// *******************************************************************************************
// *
// *    Record
// *
// *******************************************************************************************
//!
//!     @param  section the name of the section to record or play back
//!     @return   true on success
//!
// *******************************************************************************************

bool tRecorder::Record( char const * section )
{
    // delegate
    return tRecorderTemplate1< tRecordingBlock >::Archive( false, section );
}

// *******************************************************************************************
// *
// *    Playback
// *
// *******************************************************************************************
//!
//!     @param  section the name of the section to record or play back
//!     @return   true on success
//!
// *******************************************************************************************

bool tRecorder::Playback( char const * section )
{
    // delegate
    return tRecorderTemplate1< tPlaybackBlock >::Archive( false, section );
}

// *******************************************************************************************
// *
// *    PlaybackStrict
// *
// *******************************************************************************************
//!
//!     @param  section the name of the section to record or play back
//!     @return   true on success
//!
// *******************************************************************************************

bool tRecorder::PlaybackStrict( char const * section )
{
    // delegate
    return tRecorderTemplate1< tPlaybackBlock >::Archive( true, section );
}

// *****************************************************************************************
// *****************************************************************************************
// *****************************************************************************************
// *****************************************************************************************

// *****************************************************************************************
// *
// *	tLineString
// *
// *****************************************************************************************
//!
//!		@param	other	the string to copy
//!
// *****************************************************************************************

tLineString::tLineString( tString const & other )
        :tString( other )
{
}

// *****************************************************************************************
// *
// *	tLineString
// *
// *****************************************************************************************
//!
//!
// *****************************************************************************************

tLineString::tLineString( void )
{
}

// *****************************************************************************************
// *
// *	~tLineString
// *
// *****************************************************************************************
//!
//!
// *****************************************************************************************

tLineString::~tLineString( void )
{
}

//! persistent string writing operator
std::ostream & operator << ( std::ostream & s, tLineString const & line )
{
    // write magic character
    s << 'L';

    // print string, encode newlines
    for( int i=0; i<line.Len(); ++i)
    {
        char c = line[i];
        if ( c == '\n' )
            s << "\\n";
        if ( c == '\\' )
            s << "\\\\";
        else if ( c != '\0' )
            s << c;
    }

    return s << '\n';
}

//! persistent string reading operator
std::istream & operator >> ( std::istream & s, tLineString & line )
{
    // read magic character
    char c;
    s >> c;
    tASSERT( 'L' == c );

    tString read;

    // read line
    read.ReadLine(s);

    // std::cout << "Read: " << read << "\n";

    line.Clear();

    // copy line, replacing "\n" with real newline
    for(size_t i=0; i<read.Size(); ++i)
    {
        char c = read[i];
        if ( c != '\\' || i+1 == read.Size() || ( read[i+1] != 'n' && read[i+1] != '\\' ) )
        {
            line << c;
        }
        else if ( read[i+1] == '\\' )
        {
            line << "\\";
            i++;
        }
        else // if ( read[i+1] != 'n' )
        {
            line << "\n";
            i++;
        }
    }

    return s;
}

// *****************************************************************************************
// *****************************************************************************************
// *****************************************************************************************
// *****************************************************************************************

// *****************************************************************************************
// *
// *	Initialize
// *
// *****************************************************************************************
//!
//!		@param	section	   name of the section to start
//!		@param	recording  recording to read block from
//!		@return		       true on success
//!
// *****************************************************************************************

bool tRecordingBlockBase::Initialize( char const * section, tRecording * recording )
{
    // initialize recording pointer
    recording_ = recording;
    if (!recording_)
        return false;

    // start section
    recording_->BeginSection( section );

    // return success
    return true;
}

// *****************************************************************************************
// *
// *	Initialize
// *
// *****************************************************************************************
//!
//!		@param	section	   name of the section to start
//!		@return		       true on success
//!
// *****************************************************************************************

bool tRecordingBlockBase::Initialize( char const * section )
{
    // delegate
    return Initialize( section, tRecording::currentRecording_ );
}

// *****************************************************************************************
// *
// *	Separator
// *
// *****************************************************************************************
//!
//!
// *****************************************************************************************

void tRecordingBlockBase::Separator( void )
{
    GetRecordingStream() << "\n";
    separate_ = false;
}

// ******************************************************************************************
// *
// *	GetArchive
// *
// ******************************************************************************************
//!
//!		@return		the currently running recording
//!
// ******************************************************************************************

tRecording * tRecordingBlockBase::GetArchive( void )
{
    return tRecording::currentRecording_;
}

// *****************************************************************************************
// *
// *	tRecordingBlockBase
// *
// *****************************************************************************************
//!
//!
// *****************************************************************************************

tRecordingBlockBase::tRecordingBlockBase( void )
        : separate_( true ), recording_( NULL )
{
}

// *****************************************************************************************
// *
// *	~tRecordingBlockBase
// *
// *****************************************************************************************
//!
//!
// *****************************************************************************************

tRecordingBlockBase::~tRecordingBlockBase( void )
{
    // make sure everything is logged, even if program crashes
    if (recording_)
        GetRecordingStream().flush();

    recording_ = 0;
}

// *****************************************************************************************
// *
// *	GetRecordingStream
// *
// *****************************************************************************************
//!
//!		@return		the stream of the recording
//!
// *****************************************************************************************

std::ostream & tRecordingBlockBase::GetRecordingStream() const
{
    tASSERT( recording_ );

    return recording_->DoGetStream();
}

// *******************************************************************************************
// *
// *    tRecordingBlock
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

tRecordingBlock::tRecordingBlock( void )
{
}

// *******************************************************************************************
// *
// *    ~tRecordingBlock
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

tRecordingBlock::~tRecordingBlock( void )
{
}

// *****************************************************************************************
// *****************************************************************************************
// *****************************************************************************************
// *****************************************************************************************

// *****************************************************************************************
// *
// *	Initialize
// *
// *****************************************************************************************
//!
//!		@param	section	   name of section to read
//!		@param	playback   playback to read from
//!		@return		       true on success
//!
// *****************************************************************************************

bool tPlaybackBlockBase::Initialize( char const * section, tPlayback * playback )
{
    // initialize playback pointer
    playback_ = playback;
    if (!playback_)
        return false;

    // std::cout << playback_->GetNextSection() << "," << section << "\n";

    // read section
    if( playback_->GetNextSection() != section )
    {
        playback_ = NULL;
        return false;
    }

    // return success
    return true;
}

// *****************************************************************************************
// *
// *	Initialize
// *
// *****************************************************************************************
//!
//!		@param	section	   name of section to read
//!		@return		       true on success
//!
// *****************************************************************************************

bool tPlaybackBlockBase::Initialize( char const * section )
{
    return Initialize( section, tPlayback::currentPlayback_ );
}

// *****************************************************************************************
// *
// *	Separator
// *
// *****************************************************************************************
//!
//!
// *****************************************************************************************

void tPlaybackBlockBase::Separator( void ) const
{
}
// ******************************************************************************************
// *
// *	GetArchive
// *
// ******************************************************************************************
//!
//!		@return		the currently running playback
//!
// ******************************************************************************************

tPlayback * tPlaybackBlockBase::GetArchive( void )
{
    return tPlayback::currentPlayback_;
}

// *****************************************************************************************
// *
// *	tPlaybackBlockBase
// *
// *****************************************************************************************
//!
//!
// *****************************************************************************************

tPlaybackBlockBase::tPlaybackBlockBase( void )
        : playback_( 0 )
{
}

// *****************************************************************************************
// *
// *	~tPlaybackBlockBase
// *
// *****************************************************************************************
//!
//!
// *****************************************************************************************

tPlaybackBlockBase::~tPlaybackBlockBase( void )
{
    // end current block and read next
    if ( playback_ )
        playback_->AdvanceSection();

    playback_ = 0;
}

// *****************************************************************************************
// *
// *	GetPlaybackStream
// *
// *****************************************************************************************
//!
//!		@return		the stream of the playback
//!
// *****************************************************************************************

std::istream & tPlaybackBlockBase::GetPlaybackStream() const
{
    tASSERT( playback_ );

    return playback_->DoGetStream();
}

// *******************************************************************************************
// *
// *    tPlaybackBlock
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

tPlaybackBlock::tPlaybackBlock( void )
{
}

// *******************************************************************************************
// *
// *    ~tPlaybackBlock
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

tPlaybackBlock::~tPlaybackBlock( void )
{
}

// *****************************************************************************************
// *****************************************************************************************
// *****************************************************************************************
// *****************************************************************************************

static int st_debugLevelRecording=0;
static tSettingItem<int>rdb( "RECORDING_DEBUGLEVEL",
                             st_debugLevelRecording );

// returns the playback debug level, archiving the result
static int st_GetDebugLevelPlayback()
{
    // sync level with recording
    int level = st_debugLevelRecording;
    tRecorder::Playback( "DEBUGLEVEL", level );
    tRecorder::Record( "DEBUGLEVEL", level );

    return level;
}

// *******************************************************************************************
// *
// *	GetDebugLevelPlayback
// *
// *******************************************************************************************
//!
//!		@return
//!
// *******************************************************************************************

int tRecorderSyncBase::GetDebugLevelPlayback( void )
{
    // get the playback level only once
    static int level = st_GetDebugLevelPlayback();

    return level;
}

// *******************************************************************************************
// *
// *	GetDebugLevelRecording
// *
// *******************************************************************************************
//!
//!		@return
//!
// *******************************************************************************************

int tRecorderSyncBase::GetDebugLevelRecording( void )
{
    // delegate so this doesn't change when the config changes
    return GetDebugLevelPlayback();
    // return st_debugLevelRecording;
}

REAL st_GetDifference( REAL a, REAL b)
{
    return fabs( a - b );
}

REAL st_GetDifference( int a, int b)
{
    return fabs( REAL( a - b ) );
}

REAL st_GetDifference( unsigned int a, unsigned int b)
{
    return fabs( REAL( a - b ) );
}

REAL st_GetDifference( unsigned long int a, unsigned long int b)
{
    return fabs( REAL( a - b ) );
}

REAL st_GetDifference( tString const & a, tString const & b )
{
    return ( a == b ) ? 0 : 1;
}

static char const * st_fileOpen = "FILE_OPEN";
static char const * st_fileRead = "FILE_READ";

// *******************************************************************************
// *
// *	Open
// *
// *******************************************************************************
//!
//!		@param	searchPath	the path to search for the file
//!		@param	fileName	the name of the file to open
//!		@return	true if opening succeeded
//!
// *******************************************************************************

bool tTextFileRecorder::Open( tPath const & searchPath, char const * fileName )
{
    tASSERT( !stream_ );

    bool result = false;

    // try to read opening state from recording
    if ( !tRecorder::Playback( st_fileOpen, result, eof_ ) )
    {
        // open the stream (if not in recording mode)
        stream_ = tNEW( std::ifstream );
        result = searchPath.Open( *stream_, fileName );
        eof_ = !result || !stream_->good() || stream_->eof();
    }
    tRecorder::Record( st_fileOpen, result, eof_ );

    return result;
}

// *******************************************************************************
// *
// *	tTextFileRecorder
// *
// *******************************************************************************
//!
//!		@param	searchPath	the path to search for the file
//!		@param	fileName	the name of the file to open
//!
// *******************************************************************************

tTextFileRecorder::tTextFileRecorder( tPath const & searchPath, char const * fileName )
        : stream_(NULL), eof_(false)
{
    Open( searchPath, fileName );
}

// *******************************************************************************
// *
// *	tTextFileRecorder
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

tTextFileRecorder::tTextFileRecorder( void )
        : stream_(NULL), eof_(false)
{
    stream_ = NULL;
}

// *******************************************************************************
// *
// *	~tTextFileRecorder
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

tTextFileRecorder::~tTextFileRecorder( void )
{
    delete stream_;
    stream_ = NULL;
}

// *******************************************************************************
// *
// *	EndOfFile
// *
// *******************************************************************************
//!
//!		@return true if the end of file (or any other error that has the same effect) has been reached
//!
// *******************************************************************************

bool tTextFileRecorder::EndOfFile( void ) const
{
    return eof_;
}

// *******************************************************************************
// *
// *	GetLine
// *
// *******************************************************************************
//!
//!		@return the line read from the file or a recording thereof
//!
// *******************************************************************************

std::string tTextFileRecorder::GetLine( void )
{
    // try to read opening state from recording
    tLineString line;
    if ( !tRecorder::Playback( st_fileRead, line, eof_ ) )
    {
        // read a line
        tASSERT( stream_ );
        line.ReadLine( *stream_ );
        std::ws( *stream_ );
        eof_ = !stream_->good() || stream_->eof();
    }
    tRecorder::Record( st_fileRead, line, eof_ );

    // convert and return read line
    return std::string(line);
}

// *****************************************************************************
// * Multithread support, allows progress of background tasks to be synced
// *****************************************************************************

static std::vector< tBackgroundSyncEvent * > st_backgroundSyncEvents;
static boost::mutex st_backgroundSyncEventsMutex;
static const char * st_eventSection = "BACKGROUND_EVENT";
static const char * st_eventSectionEnd = "BACKGROUND_EVENT_END";

static boost::mutex st_backgroundSyncDesConstruction;
static int st_backgroundProcesses = 0;

void st_SyncBackgroundThreads()
{
    if( tRecorder::IsPlayingBack() )
    {
        int id;
        bool one = false;
        while( tRecorder::Playback( st_eventSection, id ) )
        {
            one = true;
            tBackgroundSyncEvent * event = NULL;
            while( true )
            {
                // find ID in background events
                {
                    boost::lock_guard< boost::mutex > lock( st_backgroundSyncEventsMutex );

                    for( std::vector< tBackgroundSyncEvent * >::iterator i = st_backgroundSyncEvents.begin(); i != st_backgroundSyncEvents.end(); ++i )
                    {
                        if( (*i)->ID() == id )
                        {
                            event = *i;

                            // erase event from unordered list
                            if( st_backgroundSyncEvents.size() > 1 )
                            {
                                *i = st_backgroundSyncEvents[st_backgroundSyncEvents.size()-1];
                            }
                            st_backgroundSyncEvents.pop_back();

                            // sync event
                            event->Sync();

                            break;
                        }
                    }
                }

                if( event )
                {
                    // found, good! Get out.
                    break;
                }
                else
                {
                    // not found yet. Wait a bit.
                    tDelay( 1000 );
                }
            }
        }
        if( one )
        {
            tRecorder::PlaybackStrict( st_eventSectionEnd );
        }
    }
    else
    {
        bool oneEvent = false;

        boost::lock_guard< boost::mutex > lock( st_backgroundSyncEventsMutex );

        while ( st_backgroundSyncEvents.size() )
        {
            oneEvent = true;
            tBackgroundSyncEvent * event = st_backgroundSyncEvents.back();
            st_backgroundSyncEvents.pop_back();

            // record the event ID
            tRecorder::Record( st_eventSection, event->ID() );

            // and let the background thread roll on
            event->Sync();
        }

        if( oneEvent )
        {
            tRecorder::Record( st_eventSectionEnd );
        }
    }
}

tBackgroundSyncEvent::tBackgroundSyncEvent( tBackgroundSync & sync, bool delayEntry )
  : sync_( sync ), entered_( false )
{
    if( !delayEntry )
    {
        Enter();
    }
}

//! enters critical section
void tBackgroundSyncEvent::Enter()
{
    tASSERT( !entered_ );

    entered_ = true;

    // adds to sync list
    {
        boost::lock_guard< boost::mutex > lock( st_backgroundSyncEventsMutex );
        sync_.exit_.lock();
    
        st_backgroundSyncEvents.push_back( this );
    }

    // locks the entry mutex manually, this blocks until the foreground thread releases its lock
    sync_.entry_.lock();
}

tBackgroundSyncEvent::~tBackgroundSyncEvent()
{
    if( entered_ )
    {
        Leave();
    }
}

//! leaves critical section
void tBackgroundSyncEvent::Leave()
{
    tASSERT( entered_ );

    entered_ = false;

    // unlocks entry mutex
    sync_.entry_.unlock();
    sync_.exit_.unlock();

    // check that we're not on the list
#ifdef DEBUG
    boost::lock_guard< boost::mutex > lock( st_backgroundSyncEventsMutex );
    for( std::vector< tBackgroundSyncEvent * >::iterator i = st_backgroundSyncEvents.begin(); i != st_backgroundSyncEvents.end(); ++i )
    {
        tASSERT( (*i) != this );
    }
#endif
}
    
//! called from the main thread to sync up
void tBackgroundSyncEvent::Sync()
{
    boost::lock_guard< boost::mutex > lock( st_backgroundSyncDesConstruction );

    // call static function; *this is going to be destroyed in the background
    // while this function runs, can't be good.
    Sync( sync_ );
}

//! called from the main thread to sync up
void tBackgroundSyncEvent::Sync( tBackgroundSync & s )
{
    // unlocks the entry mutex so background thread can continue
    s.entryLockForeground_.unlock();

    // temporarily lock exit mutex; this waits for the background task to finish
    boost::lock_guard< boost::mutex > exitLockForeground_( s.exit_ );

    // locks entry again
    s.entryLockForeground_.lock();
}

//! returns the ID of the background thread
int tBackgroundSyncEvent::ID() const
{
    return sync_.id_;
}

class tBackgroundWaiter
{
public:
    ~tBackgroundWaiter()
    {
        // allow background processes to finish
        while( st_backgroundProcesses > 0 )
        {
            tDelay( 1000 );
            st_SyncBackgroundThreads();
        }
    }
};

tBackgroundSync::tBackgroundSync()
: entryLockForeground_( entry_ )
{
    boost::lock_guard< boost::mutex > lock( st_backgroundSyncDesConstruction );
    static int currentID = 0;
    id_ = currentID++;
    st_backgroundProcesses++;

    // clean up later
    static tBackgroundWaiter waiter;
}

tBackgroundSync::~tBackgroundSync()
{
    boost::lock_guard< boost::mutex > lock( st_backgroundSyncDesConstruction );
    st_backgroundProcesses--;
}

