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

#ifndef ELADDERLOG_H_SS82563
#define ELADDERLOG_H_SS82563

#include "aa_config.h"
#include "tConfiguration.h"
#include "tString.h"
#include <set>

//! create a global instance of this to write stuff to ladderlog.txt
class eLadderLogWriter
{
    tString name_;
    tString specification_;
    bool isEnabledForFile_;
    bool isEnabledForScript_;
    tColoredString cache_;
#ifdef DEBUG
    int appendCount_;
#endif
    tSettingItem<bool> *conf_;
public:
    //! Create a new ladderlog event
    //!
    //! @param specification A string of tokens that each specify how to parse an individual
    //!     part of a ladderlog line
    //!
    //!     * The tokens should each be separated by one space.
    //!     * A token should be in the form  of "name" or "name:type-specifier".
    //!       A type-specifier gives hints to the user's ladderlog parser on how
    //!       to convert the token's string value to a different type.
    //!       The default type-specifiers are "int", "float" and "list".
    //!     * The last token in the specification can have a "+" on the end.
    //!       This indicates that the token is irregular and that it may contain
    //!       spaces.
    eLadderLogWriter( char const *name, bool enabledByDefault, char const *specification );
    ~eLadderLogWriter();

    const tString & Name() const { return name_; }
    const tString & Specification() const { return specification_; }
    bool IsEnabledForFile() const { return isEnabledForFile_; }
    bool IsEnabledForScript() const { return isEnabledForScript_; }

    void SetForFile( bool enable ) { isEnabledForFile_ = enable; }
    void SetForScript( bool enable ) { isEnabledForScript_ = enable; }

    //! append a field to the current message. Spaces are added automatically.
    template<typename T> eLadderLogWriter &operator<<( T const &s )
    {
        if( isEnabled() )
        {
            cache_ << ' ' << s;
#ifdef DEBUG
            appendCount_++;
#endif
        }
        return *this;
    }

    //!< send to ladderlog and clear message
    void write();

    //!< check this if you're going to make expensive calculations for ladderlog output
    bool isEnabled() const
    {
        return isEnabledForFile_ || isEnabledForScript_;
    }
private:
#ifdef DEBUG
    void EnsureConsistent() const;
#endif
};

// Sets up ladderlog.txt for usage, and prevents verbose output from events
// such as ENCODING and SPEC.
class eLadderLogInitializer
{
public:
    eLadderLogInitializer();
    virtual ~eLadderLogInitializer();
};

// Some helper utils
namespace eListParse
{
    typedef std::set< tString > StringSet;

    // Parses a list of tokens and adds them to "set".
    // Returns the number of entries added.
    int se_ParseList( StringSet & set, std::istream & s );
}

#endif /* end of include guard: ELADDERLOG_H_SS82563 */
