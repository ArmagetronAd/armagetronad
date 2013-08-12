/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2005 by the AA DevTeam (see the file AUTHORS(.txt)
in the main source directory)

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

#include "eLadderLog.h"
#include "nNetwork.h"
#include "rConsole.h"
#include "tConfiguration.h"
#include "tCrypto.h"
#include "tDirectories.h"
#include "tRecorder.h"
#include "md5.h"
#include <iostream>
#include <fstream>
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

// TODO make theses events non-configurable.
static eLadderLogWriter se_specWriter( "SPEC", true, "name spec+" );
static eLadderLogWriter se_encodingWriter( "ENCODING", true, "charset" );

static void se_WriteSpecLadderLogData( const eLadderLogWriter & writer, bool outputFile=true, bool outputScript=true )
{
    // TODO don't write the SPEC event when it was already enabled.
    se_specWriter.SetForFile( writer.IsEnabledForFile() && outputFile );
    se_specWriter.SetForScript( writer.IsEnabledForScript() && outputScript );
    se_specWriter << writer.Name() << writer.Specification();
    se_specWriter.write();
}

//! Handles configuring ladderlog settings
class eLadderLogManager
{
    enum SetMode
    {
        SetMode_File   = 1 << 0,
        SetMode_Script = 1 << 1
    };
    typedef std::map< tString, eLadderLogWriter * > WriterMap;
    WriterMap writers_;
public:
    eLadderLogManager()
        :writers_()
    {
    }

    void Add( eLadderLogWriter *writer )
    {
        writers_[ writer->Name() ] = writer;
    }

    void Remove( eLadderLogWriter *writer )
    {
        writers_.erase( writer->Name() );
    }

    void WriteAll( std::istream & s )
    {
        SetAll( "$ladderlog_write_all_help", s, SetMode_File | SetMode_Script );
    }

    void ScriptWriteAll( std::istream & s )
    {
        SetAll( "$ladderlog_script_write_all_help", s, SetMode_Script );
    }

    void ScriptWrite( std::istream & s )
    {
        bool enable;
        s >> enable;
        if ( s.fail() )
        {
            if ( tConfItemBase::printErrors )
                con << tOutput( "$ladderlog_script_write_help" ) << '\n';
            return;
        }

        eListParse::StringSet names;
        eListParse::se_ParseList( names, s );
        int numberValid = 0;

        for ( eListParse::StringSet::const_iterator it = names.begin(); it != names.end(); ++it )
        {
            WriterMap::iterator conf = writers_.find( *it );
            if ( conf != writers_.end() )
            {
                numberValid++;
                conf->second->SetForScript( enable );
                se_WriteSpecLadderLogData( *conf->second, false, true );
            }
        }

        if ( tConfItemBase::printChange )
        {
            if ( enable )
                con << tOutput( "$ladderlog_script_write_enabled", numberValid ) << '\n';
            else
                con << tOutput( "$ladderlog_script_write_disabled", numberValid ) << '\n';
        }
    }

    void WriteInitData( bool outputFile, bool outputScript )
    {
        for ( WriterMap::iterator it = writers_.begin(); it != writers_.end(); ++it )
        {
            se_WriteSpecLadderLogData( *it->second, outputFile, outputScript );
        }
        se_encodingWriter.SetForFile( outputFile );
        se_encodingWriter.SetForScript( outputScript );
        se_encodingWriter << st_internalEncoding;
        se_encodingWriter.write();
    }

    void UpdateChecksum( md5_state_s *state )
    {
        for ( WriterMap::const_iterator it = writers_.begin(); it != writers_.end(); ++it )
        {
            const eLadderLogWriter & writer = *it->second;
            md5_append( state, (md5_byte_t const *)writer.Name().c_str(), writer.Name().size() );
            md5_append( state, (md5_byte_t const *)writer.Specification().c_str(), writer.Specification().size() );
        }
    }
private:
    void SetAll( const char *help, std::istream & s, int mode )
    {
        bool enable;
        s >> enable;

        if ( s.fail() )
        {
            if ( tConfItemBase::printErrors )
                con << tOutput( help ) << '\n';
            return;
        }

        for ( WriterMap::iterator it = writers_.begin(); it != writers_.end(); ++it )
        {
            if ( mode & SetMode_File )
                it->second->SetForFile( enable );
            if ( mode & SetMode_Script )
                it->second->SetForScript( enable );
            se_WriteSpecLadderLogData( *it->second, mode & SetMode_File, mode & SetMode_Script );
        }

        if ( tConfItemBase::printChange )
        {
            if ( enable )
                con << tOutput( "$ladderlog_write_all_enabled" ) << '\n';
            else
                con << tOutput( "$ladderlog_write_all_disabled" ) << '\n';
        }
    }
};

static eLadderLogManager & se_GlobalLadderLogManager()
{
    static eLadderLogManager manager;
    return manager;
}

//! A tSettingItem that operates on two boolean values.
class eLadderLogSetting : public tSettingItem< bool >
{
public:
    eLadderLogSetting( tString name, bool & fileTarget, bool & scriptTarget, const eLadderLogWriter & writer )
        : tConfItemBase( name, tOutput( "$ladderlog_write_template", writer.Name(), writer.Specification() ) )
        , tSettingItem< bool >( name, fileTarget )
        , scriptTarget_( &scriptTarget )
        , writer_( writer )
    {
    }

    virtual void WasChanged()
    {
        *scriptTarget_ = *target;
        se_WriteSpecLadderLogData( writer_ );
    }
private:
    bool *scriptTarget_;
    const eLadderLogWriter & writer_;
};

eLadderLogWriter::eLadderLogWriter( char const *name, bool enabledByDefault, char const *specification )
    : name_( name )
    , specification_( specification )
    , isEnabledForFile_( enabledByDefault )
    , isEnabledForScript_( enabledByDefault )
    , cache_( name )
#ifdef DEBUG
    , appendCount_( 0 )
#endif
{
    conf_ = new eLadderLogSetting( tString( "LADDERLOG_WRITE_" ) + name, isEnabledForFile_, isEnabledForScript_, *this );
    se_GlobalLadderLogManager().Add( this );
}

eLadderLogWriter::~eLadderLogWriter()
{
    se_GlobalLadderLogManager().Remove( this );
    if ( conf_ )
        delete conf_;
}

static bool se_ladderlogDecorateTS = false;
static tSettingItem< bool > se_ladderlogDecorateTSConf( "LADDERLOG_DECORATE_TIMESTAMP", se_ladderlogDecorateTS );

void eLadderLogWriter::write()
{
    if( isEnabled() && sn_GetNetState() != nCLIENT && !tRecorder::IsPlayingBack() )
    {
#ifdef DEBUG
        EnsureConsistent();
        appendCount_ = 0;
#endif
        std::string line;
        if ( se_ladderlogDecorateTS )
            line += st_GetCurrentTime("%Y/%m/%d-%H:%M:%S ");
        line += cache_;
        line += '\n';

        if ( isEnabledForFile_ )
        {
            std::ofstream o;
            if ( tDirectories::Var().Open( o, "ladderlog.txt", std::ios::app ) )
                o << line;
        }

        if ( isEnabledForScript_ )
        {
            sr_InputForScripts( line.c_str() );
        }
    }
    cache_ = name_;
}

#ifdef DEBUG
void eLadderLogWriter::EnsureConsistent() const
{
    int expected = std::count( specification_.begin(), specification_.end(), ' ' ) + 1;
    bool isConsistent = specification_.EndsWith( "+" ) ? appendCount_ >= expected : appendCount_ == expected;
    if ( !isConsistent )
        tERR_WARN(
            "Inconsistent spec for '" << name_ << "': expected " << expected << " append(s) but got " << appendCount_
        );
}
#endif

//********************************************************
// Commands
//********************************************************

static void se_LadderLogWriteAll( std::istream & s )
{
    se_GlobalLadderLogManager().WriteAll( s );
}
static tConfItemFunc se_ladderLogWriteAllConf( "LADDERLOG_WRITE_ALL", se_LadderLogWriteAll );

#if HAVE_UNISTD_H
static void se_LadderLogScriptWriteAll( std::istream & s )
{
    se_GlobalLadderLogManager().ScriptWriteAll( s );
}
static tConfItemFunc se_ladderLogScriptWriteAllConf( "LADDERLOG_SCRIPT_WRITE_ALL", se_LadderLogScriptWriteAll );

static void se_LadderLogScriptWrite( std::istream & s )
{
    se_GlobalLadderLogManager().ScriptWrite( s );
}
static tConfItemFunc se_ladderLogScriptWriteConf( "LADDERLOG_SCRIPT_WRITE", se_LadderLogScriptWrite );
#endif /* HAVE_UNISTD_H */


//********************************************************
// Internal implementation
//********************************************************

struct eLadderLogInternal
{
    eLadderLogInternal()
        : fileSize_( 0 )
        , checksum_()

        // Increment this if there there was a bug fix that wouldn't normally trigger eLadderLogInitializer
        // to write its data.
        , version_( 1 )
    {
    }

    void LoadFromFile()
    {
#define CHECK() if ( f.eof() || !f.good() ) { fileSize_ = 0; checksum_.Clear(); version_ = 0; return; }
        std::ifstream f;
        if ( tDirectories::Var().Open( f, "internal_ladderlog_data.txt" ) )
        {
            CHECK();
            f >> fileSize_;

            CHECK();
            f >> checksum_;

            CHECK();
            f >> version_;
        }
#undef CHECK
    }

    void LoadCurrent()
    {
#ifdef HAVE_SYS_STAT_H
        {
            tString path = tDirectories::Var().GetReadPath( "ladderlog.txt" );
            if ( !path.empty() )
            {
                struct stat info;
                stat( path.c_str(), &info );
                fileSize_ = info.st_size;
            }
        }
#endif
        {
            md5_state_s state;
            md5_init( &state );
            se_GlobalLadderLogManager().UpdateChecksum( &state );
            md5_finish( &state, checksum_.content );
        }
    }

    void SaveToFile() const
    {
        std::ofstream o;
        if ( tDirectories::Var().Open( o, "internal_ladderlog_data.txt", std::ios::trunc ) )
        {
            o << fileSize_ << '\n';
            o << checksum_ << '\n';
            o << version_ << '\n';
        }
    }

    bool HasChanged( const eLadderLogInternal & other ) const
    {
        if ( fileSize_ != other.fileSize_ || checksum_ != other.checksum_ || version_ != other.version_ )
            return true;
        return false;
    }

    off_t fileSize_;
    tChecksum checksum_;
    int version_;
};

#ifdef DEDICATED
void se_ScriptSpawnedCallback()
{
    se_GlobalLadderLogManager().WriteInitData( false, true );
}

static rScriptSpawnedCallback se_scriptCallback( se_ScriptSpawnedCallback );
#endif

eLadderLogInitializer::eLadderLogInitializer()
{
    eLadderLogInternal oldData, newData;
    oldData.LoadFromFile();
    newData.LoadCurrent();
    if ( newData.HasChanged( oldData ) )
        se_GlobalLadderLogManager().WriteInitData( true, false );
}

eLadderLogInitializer::~eLadderLogInitializer()
{
    eLadderLogInternal data;
    data.LoadCurrent();
    data.SaveToFile();
}


//********************************************************
// Misc
//********************************************************

namespace eListParse
{
    int se_ParseList( StringSet & set, std::istream & s )
    {
        int entries_count = 0;
        while ( s.good() )
        {
            tString name;
            s >> name;
            if ( name.Len() > 1 )
            {
                std::pair< StringSet::iterator, bool > ret = set.insert( name );
                if ( ret.second ) entries_count++;
            }
        }
        return entries_count;
    }
}
