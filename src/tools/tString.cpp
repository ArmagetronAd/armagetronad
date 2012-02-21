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

#include "tMemManager.h"
#include "tString.h"
#include "tColor.h"
#include "tLocale.h"
#include "tConfiguration.h"
#include "tException.h"
#include <ctype.h>
#include <time.h>
#include <string>
#include <iostream>
#include "utf8.h"

// *******************************************************************************
// *
// *	Helper functions
// *
// *******************************************************************************

// reads an escape sequence from a stream
// c: the character just read from the stream
// c2: eventually set to the second character read
// s: the stream to read from
static bool st_ReadEscapeSequence( char & c, char & c2, std::istream & s )
{
    c2 = '\0';

    // detect escaping
    if ( c == '\\' && !s.eof() && s.good() )
    {
        c2 = s.get();

        // nothing useful read?
        if ( s.eof() )
        {
            c2 = 0;
            return false;
        }

        // interpret special escape sequences
        switch (c2)
        {
        case 'n':
            // turn \n into newline
            c = '\n';
            c2 = 0;
            return true;
        case '"':
        case ' ':
        case '\'':
        case '\\':
        case '\n':
            // include quoting character as literal
            return true;
        default:
            // take the whole \x sequence as it appeared.
            return false;
        }
    }

    return false;
}

// reads an escape sequence from a stream, putting back unread char
// c: the character just read from the stream
// s: the stream to read from
static bool st_ReadEscapeSequence( char & c, std::istream & s )
{
    char c2 = '\0';
    bool ret = st_ReadEscapeSequence( c, c2, s );
    if ( c2 && s.good() )
    {
        s.putback( c2 );
    }
    return ret;
}

// *******************************************************************************
// *
// *	stream reading operator
// *
// *******************************************************************************
//!
//!     @todo   write symmetric stream writing operator
//!		@param	s stream to read from
//!		@param	x string to read to
//!		@return	reference to stream for chaining
//!
// *******************************************************************************

std::istream & operator>> (std::istream &s,tString &x)
{
    x.Clear();

    std::ws(s);

    char c=s.get();

    // check if the string is quoted
    bool quoted = false;
    char quoteChar = c;   // if it applies, this is the quoting character
    if ( c == '"' || c == '\'' )
    {
        // yes, it is
        quoted = true;
        c = s.get();
    }

    bool lastEscape = false;
    while((quoted || !( isblank(c) || c == '\n' || c == '\r' ) ) && s.good() && !s.eof()){
        // read and interpret escape sequences
        bool thisEscape = false;
        if ( !lastEscape && !( thisEscape = st_ReadEscapeSequence( c, s ) ) && quoted && c == quoteChar )
        {
            // no escape, this string is quoted and the current character is an end quote. We're finished.
            c = s.get();
            break;
        }
        else
        {
            // append escaped or regular character
            x += c;
            c=s.get();
        }

        // lastEscape = thisEscape;
    }

    if(s.good())
    {
        s.putback(c);
    }
    return s;
}


//removed in favor of searching whole string...
/*void tString::RemoveStartColor(){
	tString oldname = *this;
//	unsigned short int colorcodelength = 0;
	tString newname = "";
	if (oldname.ge2("0x") == false){
		return;
	}
	newname.SetLen(14);
//	unsigned short int numcounter = 0;
	unsigned short int i = 0;
	for (i=2; i<oldname.Len(); i++){
		if (oldname(i) > 47 && oldname(i) < 58) {
//			std::cout << oldname(i) << std::endl;
			colorcodelength++;
		}
		else {
			break;
		}
	}
	unsigned short int c = 0;
	for (i=colorcodelength+2; i<oldname.Len(); i++) {
//		std::cout << oldname(i);
		newname(c) = oldname(i);
		c++;
	}
	for (i=8; i<oldname.Len(); i++) {
//		std::cout << oldname(i);
		newname(c) = oldname(i);
		c++;
	}
//	std::cout << std::endl << newname << std::endl;
	*this = newname;
}*/

/*
bool tString::operator==(const tString &other) const
{
    if (other.Len() != Len())
        return false;
    for (int i= Len()-1; i>=0; i--)
        if (other(i) != (*this)(i))
            return false;

    return true;
}
*/

// Original ge2, didn't compile under linux, some stupid overload or something, so we now have ge2
/*bool tString::operator>=(const tString &other) const
{
	if (other.Len() > Len()) {
//		std::cout << "lenissue\n";
		return false;
	}
	for (int i=0; i<other.Len()-1; i++) {
		if (other(i) != (*this)(i)) {
//			std::cout << "matchissue: '" << other(i) << "' '" << (*this)(i) << "'\n";
			return false;
		}
//		else {
//			std::cout << other(i) << " " << (*this)(i) << std::endl;
//		}
	}

	return true;
} */

// char st_stringOutputBuffer[tMAX_STRING_OUTPUT];

// *******************************************************************************
// *
// *	tString
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

tString::tString( void )
{
}

// *******************************************************************************
// *
// *	tString
// *
// *******************************************************************************
//!
//!		@param	other source of copy operation
//!
// *******************************************************************************

tString::tString( BASE const & other )
        : string( other )
{
}

// *******************************************************************************
// *
// *	tString
// *
// *******************************************************************************
//!
//!		@param	other source of copy operation
//!
// *******************************************************************************

tString::tString( tString const & other )
        : string( other )
{
}

// *******************************************************************************
// *
// *	tString
// *
// *******************************************************************************
//!
//!		@param	other source of copy operation
//!
// *******************************************************************************

tString::tString( CHAR const * other )
        : string( other ? other : "" )
{
    // tASSERT( other );
}

// *******************************************************************************
// *
// *	tString
// *
// *******************************************************************************
//!
//!		@param	other source of copy operation
//!
// *******************************************************************************

//tString::tString( tOutput const & other )
//{
//    operator=( other );
//}

// *******************************************************************************
// *
// *	operator =
// *
// *******************************************************************************
//!
//!		@param	other source of the copy operation
//!		@return	reference to this for chaining
//!
// *******************************************************************************

tString & tString::operator =( tOutput const & other )
{
    Clear();
    *this << other;
    return *this;
}

// *******************************************************************************
// *
// *   Size
// *
// *******************************************************************************
//!
//!        @return the size of the string in characters NOT including the trailing 0.
//!
// *******************************************************************************

tString::size_type tString::Size( void ) const
{
    return size();
}

// *******************************************************************************
// *
// *	ReadLine
// *
// *******************************************************************************
//!
//!		@param	s	stream to read from
//!		@param	enableEscapeSequences set to true if escape sequences (\n) shall be respected
//!     @param  indent maximum amount of indentation to remove
//!     @param  eatenWhitespace will be set to how much spacing was stripped
//!
// *******************************************************************************

void tString::ReadLine( std::istream & s, bool enableEscapeSequences, int indent, int * eatenWhitespace )
{
    char c=' ';
    Clear();
    int whitespace = -1;
    while(c!='\n' && c!='\r' && isblank(c) && (whitespace < indent || indent < 0) &&  s.good() && !s.eof()){
        c=s.get();
        whitespace++;
    }

    if (eatenWhitespace != 0)
    {
        *eatenWhitespace = whitespace;
    }

    if(s.good())
    {
        s.putback(c);
    }
    c='x';

    while( true )
    {
        c=s.get();

        // notice end of line or file
        if ( c=='\n' || c=='\r' || !s.good() || s.eof())
            break;

        if ( enableEscapeSequences )
        {
            char c2 = '\0';
            if ( st_ReadEscapeSequence( c, c2, s ) )
            {
                *this += c;
                c = 'x';
                continue;
            }
            else if ( c2 )
            {
                *this += c;
                *this += c2;
                c = 'x';
                continue;
            }
        }

        *this += c;
    }
}

// *******************************************************************************
// *
// *	Clear
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

void tString::Clear( void )
{
#ifdef _MSC_VER
    *this = "";
#else
    string::clear();
#endif
}

// *******************************************************************************************
// *
// *   SetPos
// *
// *******************************************************************************************
//!
//!        @param  len the target length
//!        @param  cut if set, the string may be cut back if its current length is bigger than length
//!
// *******************************************************************************************

void tString::SetPos(int l, bool cut){
    int i;
    if ( l < Len() )
    {
        if ( cut )
        {
            if ( l > 0 )
            {
                SetSize( l-1 );
                operator+=(' ');
            }
            else
            {
                Clear();
            }
        }
        else
        {
            operator+=(' ');
        }
    }
    if( l == Len() && !cut)
    {
        operator+=(' ');
    }
    for(i=Len();i<l;i++)
        operator+=(' ');
}

//added by me (Tank Program)
//sees if a string starts with another string
//created for remote admin...
//but used all over the place because it's a cool method --Lucifer

// *******************************************************************************************
// *
// *   StartsWith
// *
// *******************************************************************************************
//!
//!        @param  other  the string to compare the start with
//!        @return        true if this starts with other
//!
// *******************************************************************************************

bool tString::StartsWith( const tString & other ) const
{
    // const tString & rmhxt = *this;
    // rmhxt.RemoveHex();
    // *this = rmhxt;
    // other.RemoveHex();
    if (other.Len() > Len()) {
        return false;
    }
    for (int i=0; i<other.Len()-1; i++) {
        if (other[i] != (*this)[i]) {
            return false;
        }
    }

    return true;
}

// *******************************************************************************************
// *
// *   StartsWith
// *
// *******************************************************************************************
//!
//!        @param  other  the string to compare the start with
//!        @return        true if this starts with other
//!
// *******************************************************************************************

bool tString::StartsWith( const char * other ) const
{
    return StartsWith( tString( other ) );
}

// Added by me (Lucifer)
// Just reverses the two strings and returns StartsWith
// I thought that was clever, but it probably isn't

// *******************************************************************************************
// *
// *   EndsWith
// *
// *******************************************************************************************
//!
//!        @param  other  the string to compare the end with
//!        @return        true if this ends with other
//!
// *******************************************************************************************

bool tString::EndsWith(const tString & other ) const
{
    if (other.Len() > Len()) {
        return false;
    }
    tString thisString = Reverse();
    tString otherString(other);
    otherString = otherString.Reverse();

    // Haha, just use StartsWith to do the comparison :)
    return thisString.StartsWith(otherString);
    //return true;
}

// *******************************************************************************************
//!
//!        @param  other  the string to compare the end with
//!        @return        true if this ends with other
//!
// *******************************************************************************************

bool tString::EndsWith( const char* other) const {
    return EndsWith( tString(other) );
}

// *******************************************************************************
// *
// *	LTrim
// *
// *******************************************************************************
//!
//!		@return		the trimmed string
//!
// *******************************************************************************

tString tString::LTrim( void ) const
{
    tString toReturn;
    bool trim = true;

    for( size_t i = 0; i<size(); i++ )
    {
        if( !isblank((*this)[i]) )
            trim = false;
        if( !trim)
            toReturn << (*this)[i];
    }
    return toReturn;
}

// *******************************************************************************
// *
// *	RTrim
// *
// *******************************************************************************
//!
//!		@return		the trimmed string
//!
// *******************************************************************************

tString tString::RTrim( void ) const
{
    tString toReturn;
    toReturn = Reverse();

    toReturn = toReturn.LTrim();
    return toReturn.Reverse();
}

// *******************************************************************************
// *
// *	Trim
// *
// *******************************************************************************
//!
//!		@return		the trimmed string
//!
// *******************************************************************************

tString tString::Trim( void ) const
{
    return LTrim().RTrim();
}

// *******************************************************************************
// *
// *	StrPos
// *
// *******************************************************************************
//!
//!		@param	tofind	the string to find
//!		@return		    position of the start of the string or negative if not found
//!
// *******************************************************************************

int tString::StrPos( const tString & tofind ) const
{
    if (tofind.Len() > Len()) {
        return -1;
    }
    for (int i=0; i<Len()-1; i++) {
        if ((*this)(i) == tofind(0)) {
            bool found = true;
            for (int j=0; j<tofind.Len()-1 && i+j < Len()-1; j++) {
                if ((*this)(i+j) != tofind(j))
                    found = false;
            }
            if (found == true)
                return i;
        }
    }

    return -1;
}

int tString::StrPos( int start, const tString & tofind ) const
{
    if (tofind.Len() > Len()) {
        return -1;
    }
    for (int i = start; i<Len()-1; i++) {
        if ((*this)(i) == tofind(0)) {
            bool found = true;
            for (int j=0; j<tofind.Len()-1; j++) {
                if ((*this)(i+j) != tofind(j))
                    found = false;
            }
            if (found == true)
                return i;
        }
    }

    return -1;
}

// *******************************************************************************
// *
// *	StrPos
// *
// *******************************************************************************
//!
//!		@param	tofind	the string to find
//!		@return		    position of the start of the string or negative if not found
//!
// *******************************************************************************

int tString::StrPos( const CHAR * tofind ) const
{
    return StrPos( tString ( tofind ) );
}

int tString::StrPos( int start, const CHAR * tofind ) const
{
    return StrPos( start, tString ( tofind ) );
}

// *******************************************************************************
// *
// *	SubStr
// *
// *******************************************************************************
//!
//!		@param	start	start of string to extract
//!		@param	len     length of string to extract
//!		@return		    the substring
//!
// *******************************************************************************

tString tString::SubStr( const int start, int len ) const
{
    tASSERT( start >= 0 );

    if (start > Len())
        return tString("");

    //if len < 0 or too long, take the whole string
    if ( (len + start) >= Len() ||  len < 0)
        len = Len() - start - 1;

    tString toReturn("");

    for (int i=start; i<(len + start); i++) {
        toReturn << (*this)(i);
    }
    return  toReturn;
}

// *******************************************************************************
// *
// *	SubStr
// *
// *******************************************************************************
//!
//!		@param	start	start of string to extract
//!		@return		    the substring from start to the end of this string
//!
// *******************************************************************************

tString tString::SubStr( const int start ) const
{
    return SubStr (start, Len()-start-1 );
}

// helper for ToInt and ToFloat
template< class T > void ToT( tString const & source, T & target, tString::size_type pos, char const * name )
{
    if ( pos > source.Size() )
        pos = source.Size();

    if ( !source.Convert( target, pos ) )
    {
        std::ostringstream message;

        if ( pos > 0 )
        {
            message << "Expected an " << name << " at position " << pos << " of string \"" << source << "\", but got \"" << static_cast<tString::CHAR const *>(source) + pos << "\".";
        }
        else
        {
            message << "Expected an " << name << ", but got \"" << source << "\".";
        }
        throw tGenericException( message.str().c_str(), "Conversion Error" );
    }
}

// *******************************************************************************
// *
// *	ToInt
// *
// *******************************************************************************
//!
//!		@param	pos position to start looking at for a number
//!		@return	the number
//!
// *******************************************************************************

int tString::ToInt( size_type pos ) const
{
    int ret = 0;
    ToT( *this, ret, pos, "integer" );
    return ret;
}

// *******************************************************************************
// *
// *	ToFloat
// *
// *******************************************************************************
//!
//!		@param	pos position to start looking at for a number
//!		@return	the number
//!
// *******************************************************************************

REAL tString::ToFloat( size_type pos ) const
{
    REAL ret = 0;
    ToT( *this, ret, pos, "float" );
    return ret;
}

// exctact the integer at position pos plus 2^16 ( or the character ) [ implementation detail of CompareAlphanumerical ]
static int GetInt( const tString& s, int& pos )
{
    int ret = 0;
    int digit = 0;
    while ( pos < s.Len() && digit >= 0 && digit <= 9 )
    {
        ret = ret*10 + digit;
        digit = s[pos] - '0';
        pos++;
    }

    if ( ret > 0 )
    {
        return ret + 0x10000;
    }
    else
    {
        return digit + '0';
    }
}

// *******************************************************************************************
// *
// *   CompareAlphaNumerical
// *
// *******************************************************************************************
//!
//!        @param  a   first string to compare
//!        @param  b   second string to compare
//!        @return     -1 if b is bigger than a, +1 if a is bigger than b, 0 if they are equal
//!
// *******************************************************************************************

int tString::CompareAlphaNumerical( const tString& a, const tString &b)
{
    int apos = 0;
    int bpos = 0;

    while ( apos < a.Len() && bpos < b.Len() )
    {
        int adigit = GetInt( a,apos );
        int bdigit = GetInt( b,bpos );
        if ( adigit < bdigit )
            return 1;
        else if ( adigit > bdigit )
            return -1;
    }

    if ( a.Len() - apos < b.Len() - bpos )
        return 1;
    else if ( a.Len() - apos > b.Len() - bpos )
        return 1;
    else
        return 0;
}

// *******************************************************************************************
// *
// *	StripWhitespace
// *
// *******************************************************************************************
//!
//!    @return     a string with all whitespace removed. "hi everyone " -> "hieveryone"
//!
// *******************************************************************************************

tString tString::StripWhitespace( void ) const
{
    tString toReturn;

    for( int i = 0; i<=Len()-2; i++ )
    {
        if( !isblank((*this)(i)) )
            toReturn << (*this)(i);
    }
    return toReturn;
}

//! @return a string where all characters are converted to lowercase. "Hello World!" -> "hello world!"
tString tString::ToLower(void) const {
    tString ret(*this);
    for(iterator iter=ret.begin(); iter!=ret.end(); ++iter) {
        *iter = tolower(*iter);
    }
    return ret;
}

//! @return a string where all characters are converted to uppercase. "Hello World!" -> "HELLO WORLD!"
tString tString::ToUpper(void) const {
    tString ret(*this);
    for(iterator iter=ret.begin(); iter!=ret.end(); ++iter) {
        *iter = toupper(*iter);
    }
    return ret;
}

//! @param what the character that is searched for
//! @return the number of matching characters in the string, 0 if none
int tString::Count(CHAR what) const {
    int ret = 0;
    for(const_iterator iter=begin(); iter!=end(); ++iter) {
        if (*iter == what)
            ret++;
    }
    return ret;
}

//static const char delimiters[] = "`~!@#$%^&*()-=_+[]\\{}|;':\",./<>? ";
static tString delimiters("!?.:;_()-, ");
static tConfItemLine st_wordDelimiters( "WORD_DELIMITERS", delimiters );

// *******************************************************************************************
// *
// *	PosWordRight
// *
// *******************************************************************************************
//!
//!    @param      start       The location of right to search
//!    @return                 Position relative from start to the delimiter to the right.
//!
// *******************************************************************************************

int tString::PosWordRight( int start ) const {
    int nextDelimiter = strcspn( SubStr( start, Len() ), delimiters );
    int toMove = nextDelimiter;

    // A delimter in our immediate path
    if ( toMove == 0 ) {
        // Move over delimiters
        while ( nextDelimiter == 0 && start + 1 < Len() ) {
            toMove++;
            start++;
            nextDelimiter = strcspn( SubStr( start, Len() ), delimiters );
        }
        // Skip over the word if not multiple delimiters
        if ( toMove == 1 ) {
            toMove += nextDelimiter;
        }
    }

    return toMove;
}

// *******************************************************************************************
// *
// *	PosWordLeft
// *
// *******************************************************************************************
//!
//!    @param      start       The location of left to search
//!    @return                 Position relative from start to the delimiter to the left.
//!
// *******************************************************************************************

int tString::PosWordLeft( int start ) const {
    return -1 * Reverse().PosWordRight( Len() - start - 1 );
}

// *******************************************************************************************
// *
// *	RemoveWordRight
// *
// *******************************************************************************************
//!
//!    @param      start       The location to start removing from
//!    @return                 Number of characters removed and to which direction
//!                                x > 0 right
//!                                x < 0 left
//!
// *******************************************************************************************

int tString::RemoveWordRight( int start ) {
    int removed = PosWordRight( start );
    RemoveSubStr( start, removed );
    return removed;
}

// *******************************************************************************************
// *
// *	RemoveWordLeft
// *
// *******************************************************************************************
//!
//!    @param      start       The location to start removing from
//!    @return                 Number of characters removed and to which direction
//!                                x > 0 right
//!                                x < 0 left
//!
// *******************************************************************************************

int tString::RemoveWordLeft( int start ) {
    int removed = PosWordLeft( start );
    RemoveSubStr( start, removed );
    return removed;
}

// *******************************************************************************************
// *
// *	RemoveSubStr
// *
// *******************************************************************************************
//!
//!    @param      start       The position in the string
//!    @param      length      How many characters to delete and the direction.
//!                                x > 0 deletes right
//!                                x < 0 deletes left
//!
// *******************************************************************************************

void tString::RemoveSubStr( int start, int length ) {
    int strLen = size();
    if ( length < 0 ) {
        start += length;
        length = abs( length );
    }

    if ( start + length > strLen || start < 0 || length == 0 ) {
        return;
    }

    if ( start == 0 ) {
        if ( strLen - length == 0 ) {
            *this = ("");
        }
        else {
            *this = SubStr( start + length, strLen );
        }
    }
    else {
        *this = SubStr( 0, start ) + SubStr( start + length, strLen );
    }

    // SetSze(strLen+1-length);
}

// *******************************************************************************************
// *
// *	RemoveSubStrUtf8
// *
// *******************************************************************************************
//!
//!    @param      start       The position in the string
//!    @param      length      How many characters to delete and the direction.
//!                                x > 0 deletes right
//!                                x < 0 deletes left
//!	   @returns the number or characters that were really deleted.
//!
// *******************************************************************************************

int tString::RemoveSubStrUtf8( int start, int length ) {
    int strLen = size();
    if ( length < 0 ) {
        start += length;
        length = -length;
    }

    if ( start + length > strLen || start < 0 || length == 0 ) {
        return 0;
    }

    while(((*this)[start] & 0xc0) == 0x80) {
        --start;
        ++length;
    }
    while(start+length < strLen && ((*this)[start+length] & 0xc0) == 0x80) {
        ++length;
    }

    erase(start,length);

    return length;

    // SetSze(strLen+1-length);
}


// *******************************************************************************************
// *
// *    GetFileMimeExtension
// *
// *******************************************************************************************
//!
//!     @param  none
//!     @return         The extension of the filename represented by this string
//!
// *******************************************************************************************

tString tString::GetFileMimeExtension() const {
    tString extension;
    int currentPosition = Len();

    bool goonie = true;

    if( currentPosition < 2 ) return tString("");

    while(goonie) {
        extension += SubStr(currentPosition,1);
        if( SubStr(currentPosition,1) == "." ) goonie = false;
        currentPosition--;
    }

    extension = extension.Reverse();
    if( extension == (*this) ) return tString("");

    for(size_t i=0; i<extension.size(); i++)
        extension[i] = tolower(extension[i]);
    return extension;
}

// *******************************************************************************************
// *
// *	Reverse
// *
// *******************************************************************************************
//!
//!    @return                 A copy of the string reversed
//!
// *******************************************************************************************

tString tString::Reverse() const {
    tString reversed;
    for ( int index = Len() - 2; index >= 0; index-- ) {
        reversed << ( *this ) ( index );
    }

    return reversed;
}

// *******************************************************************************************
// *
// *	Truncate
// *
// *******************************************************************************************
//!
//!    @param      truncateAt       The postion to truncate at
//!    @return     A new string with the truncated text and "..." concatenated
//!
//!    Example: tString("word").Truncate( 2 ) -> "wo..."
//!
// *******************************************************************************************

tString tString::Truncate( int truncateAt ) const
{
    // The string does not need to be truncated
    if ( truncateAt >= Len() )
        return *this;

    return SubStr( 0, truncateAt ) + "...";
}

// *******************************************************************************************
// *
// *	FromUnknown
// *
// *******************************************************************************************
//!
//!    @param      source       Source string of unknown format
//!    @return     A new string that is valid utf8
//!
// *******************************************************************************************
tString tString::FromUnknown( char const * source )
{
    std::string ret( source );

    // if the string already is utf8, just return it
    if( utf8::is_valid( ret.begin(), ret.end() ) )
    {
        return ret;
    }
    else
    {
        // convert it
        return st_Latin1ToUTF8( ret );
    }
}

// *******************************************************************************
// *
// *	operator []
// *
// *******************************************************************************
//!
//!		@param	i	position to look up
//!		@return		character at given position
//!
// *******************************************************************************

tString::CHAR tString::operator []( size_t i ) const
{
    tASSERT( i <= size() );
    return BASE::operator[]( i );
}

// *******************************************************************************
// *
// *	operator []
// *
// *******************************************************************************
//!
//!		@param	i	position to look up
//!		@return		character at given position
//!
// *******************************************************************************

tString::CHAR & tString::operator []( size_t i )
{
    tASSERT( i <= size() );
    return BASE::operator[]( i );
}

// *******************************************************************************
// *
// *	operator []
// *
// *******************************************************************************
//!
//!		@param	i	position to look up
//!		@return		character at given position
//!
// *******************************************************************************

tString::CHAR tString::operator []( int i ) const
{
    return operator[]( size_t(i) );
}

// *******************************************************************************
// *
// *	operator []
// *
// *******************************************************************************
//!
//!		@param	i	position to look up
//!		@return		character at given position
//!
// *******************************************************************************

tString::CHAR & tString::operator []( int i )
{
    return operator[]( size_t(i) );
}

// *******************************************************************************
// *
// *	operator ( )
// *
// *******************************************************************************
//!
//!		@param	i	position to look up
//!		@return		character at given position
//!
// *******************************************************************************

tString::CHAR tString::operator ()( size_t i ) const
{
    tASSERT( i <= size() );
    return BASE::operator[]( i );
}

// *******************************************************************************
// *
// *	operator ( )
// *
// *******************************************************************************
//!
//!		@param	i	position to look up
//!		@return		character at given position
//!
// *******************************************************************************

tString::CHAR & tString::operator ()( size_t i )
{
    tASSERT( i < size() );
    return BASE::operator[]( i );
}

// *******************************************************************************
// *
// *	operator []
// *
// *******************************************************************************
//!
//!		@param	i	position to look up
//!		@return		character at given position
//!
// *******************************************************************************

tString::CHAR tString::operator ()( int i ) const
{
    return operator()( size_t(i) );
}

// *******************************************************************************
// *
// *	operator ()
// *
// *******************************************************************************
//!
//!		@param	i	position to look up
//!		@return		character at given position
//!
// *******************************************************************************

tString::CHAR & tString::operator ()( int i )
{
    return operator()( size_t(i) );
}

// *******************************************************************************************
// *
// *	Compare
// *
// *******************************************************************************************
//!
//!     @param	other       the string to compare with
//!     @param  ignoreCase  set to true to compare ignoring case
//!     @return             negative if *this is lexicograhically less that other, 0 if they are equal, positive otherwise
//!
// *******************************************************************************************

int tString::Compare( const char* other, bool ignoreCase ) const
{
    if ( ignoreCase ) {
        return strcasecmp( *this, other );
    }
    else {
        return Compare( other );
    }
}

// *******************************************************************************************
// *
// *	Compare
// *
// *******************************************************************************************
//!
//!     @param	other 	the string to compare with
//!     @return         negative if *this is lexicograhically less that other, 0 if they are equal, positive otherwise
//!
// *******************************************************************************************

int tString::Compare( const char* other ) const
{
    if ( !other )
        return 1;

    return strcmp( *this, other );
}

// *******************************************************************************
// *
// *	operator const CHAR*
// *
// *******************************************************************************
//!
//!    @return the C string version of this string
//!
// *******************************************************************************

tString::operator const tString::CHAR *( void ) const
{
    return c_str();
}

//! @param start The position in the string to begin measuring
//! @param len The length of the string to measure (in bytes)
//! @returns the real length in characters
int tString::LenUtf8( size_t start, size_t len ) const
{
    tASSERT(start+(len == npos ? 0 : len)<=size());
    size_type ret = 0;
    const_iterator i = begin() + start;
    const_iterator end_iter;
    if(len == npos) {
        end_iter = end();
    } else {
        end_iter = i + len;
    }
    for(; i != end_iter; ++i) {
        if((*i & 0xc0) != 0x80) {
            ++ret;
        }
    }
    return ret;
}

// *******************************************************************************
// *
// *	Len
// *
// *******************************************************************************
//!
//!		@return		the length including the trailing \\0
//!
// *******************************************************************************

int tString::Len( void ) const
{
    return size() + 1;
}

//! @return the length of the longest line
int tString::LongestLine( void ) const {
    unsigned longest = 0;
    unsigned pos = 0;
    int l;
    while ((l=find_first_of('\n', pos)) != -1) {
        if(l-pos > longest)
            longest = l-pos;
        pos = l+1;
    }
    if(size() - pos > longest)
        longest = size() - pos;
    return longest;
}

//! @return the length of the longest line, not including color codes
int tColoredString::LongestLine( void ) const {
    return tColoredString::RemoveColors(c_str()).LongestLine();
}

// *******************************************************************************
// *
// *	SetSize
// *
// *******************************************************************************
//!
//!		@param	len	the desired length
//!
// *******************************************************************************

void tString::SetSize( unsigned int len )
{
    tASSERT(len < 268435456);

    while ( len > size() )
        *this += ' ';
    if ( len < size() )
        *this = SubStr( 0, len );

    tASSERT( size() == len );
}

// *******************************************************************************
// *
// *	SetLen
// *
// *******************************************************************************
//!
//!     Legacy function, don't use.
//!		@param	len	the desired length (including the trailing \0)
//!
// *******************************************************************************

void tString::SetLen( int len )
{
    // legacy function, don't use
    st_Breakpoint();

    while ( len > Len() )
        *this += ' ';
    if ( len < Len() )
        *this = SubStr( 0, len );

    tASSERT( Len() == len );
}

// *******************************************************************************************
// *******************************************************************************************
// *******************************************************************************************

// *******************************************************************************************
// *
// *	~tColoredString
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

tColoredString::~tColoredString( void )
{
}

// *******************************************************************************************
// *
// *	tColoredString
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

tColoredString::tColoredString( void )
{
}

// *******************************************************************************************
// *
// *	tColoredString
// *
// *******************************************************************************************
//!
//!		@param	other string to copy from
//!
// *******************************************************************************************

tColoredString::tColoredString( const tColoredString & other )
        :tString( other )
{
}

// *******************************************************************************************
// *
// *	tColoredString
// *
// *******************************************************************************************
//!
//!     @param  other string to copy from
//!
// *******************************************************************************************

tColoredString::tColoredString( const tString & other )
        :tString( other )
{
}

// *******************************************************************************************
// *
// *	tColoredString
// *
// *******************************************************************************************
//!
//!     @param  other string to copy from
//!
// *******************************************************************************************

tColoredString::tColoredString( const std::string & other )
        :tString( other )
{
}

// *******************************************************************************************
// *
// *	tColoredString
// *
// *******************************************************************************************
//!
//!     @param  other C string to copy from
//!
// *******************************************************************************************

tColoredString::tColoredString( const char * other )
        :tString( other )
{
}

// *******************************************************************************************
// *
// *	tColoredString
// *
// *******************************************************************************************
//!
//!		@param	other	output to copy from
//!
// *******************************************************************************************

tColoredString::tColoredString( const tOutput & other )
        :tString( other )
{
}

// *******************************************************************************************
// *
// *	RemoveColors
// *
// *******************************************************************************************
//!
//!		@param	c	C style string or tString to clear of color codes
//!		@return   	cleared string
//!
// *******************************************************************************************

tString tColoredString::RemoveColors( const char * c, bool darkonly )
{
    // st_Breakpoint();
    tString ret;
    int len = strlen(c);
    bool removed = false;

    // walk through string
    while (*c!='\0'){
        // skip color codes
        if (*c=='0' && len >= 2 && c[1]=='x')
        {
            if( len >= 8 && darkonly )
            {
                tColor colorToFilter( c );
                if ( !colorToFilter.IsDark() || strncmp( c, "0xRESETT", 8 ) == 0 )
                    ret << c[0] << c[1] << c[2] << c[3] << c[4] << c[5] << c[6] << c[7];
                else
                    removed = true;

                c   += 8;
                len -= 8;
            }
            else if( len >= 8 )
            {
                c   += 8;
                len -= 8;
                removed = true;
            }
            else
            {
                // skip incomplete color codes, too
                return RemoveColors( ret, darkonly );
            }
        // st_Breakpoint();
        }
        else
        {
            ret << (*(c++));
            len--;
        }
    }

    // st_Breakpoint();

    return removed ? RemoveColors( ret, darkonly ) : ret;
}

// *******************************************************************************************
// *
// *	RemoveColors
// *
// *******************************************************************************************
//!
//!		@param	c	C style string to clear of color codes
//!		@return   	cleared string
//!
// *******************************************************************************************

tString tColoredString::RemoveColors( const char * c )
{
    return RemoveColors ( c, false );
}

// helper function: removes trailing color of string and returns number of chars
// used by color codes
static int RemoveTrailingColor( tString& s, int maxLen=-1 )
{
    // count bytes lost to color codes
    int posDisplacement = 0;
    int len = 0;

    // walk through string
    for ( size_t g=0; g+1 < s.Size(); g++)
    {
        if (s(g) == '0' && s(g+1) == 'x')
        {
            // test if the code is legal ( not so far at the end that it overlaps )
            if ( s.Size() >= g + 8 )
            {
                // everything is in order, record color code usage and advance
                posDisplacement+=8;
                g+=7;
            }
            else
            {
                // illegal code! Remove it.
                s.SetSize( g );
                // s[g]=0;
            }
        }
        else if ( maxLen > 0 )
        {
            if ( ++len >= maxLen )
            {
                // maximal end reached, cut it off
                s.SetSize( g-1 );
                // s[g]=0;
            }
        }
    }

    return posDisplacement;
}

// *******************************************************************************************
// *
// *	SetPos
// *
// *******************************************************************************************
//!
//!		@param	len     the desired length
//!		@param	cut	    only if set, the length of the string may be reduced
//!
// *******************************************************************************************


void tColoredString::SetPos( int len, bool cut )
{
    // determine desired raw length taking color codes into account and possibly cutting
    int wishLen = len + ::RemoveTrailingColor( *this, cut ? len : -1 );

    // delegate
    tString::SetPos( wishLen, cut );
}

// *******************************************************************************************
// *
// *	RemoveTrailingColor
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

void tColoredString::RemoveTrailingColor( void )
{
    // delegage
    ::RemoveTrailingColor( *this );
}

// *******************************************************************************************
// *
// *	NetFilter
// *
// *******************************************************************************************
//!
//!		@param	filterWhitespace whether whitespace should be filtered away
//!
// *******************************************************************************************

void tString::NetFilter( bool filterWhitespace )
{
    // output string
    tString out;

    // prepare reader and writer
    tString::iterator reader = begin();
    std::back_insert_iterator< tString > writer = back_inserter(out);

    try
    {
        // filter out illegal characters
        while( reader != end() )
        {
            // convert from utf8
            wchar_t c = utf8::next( reader, end() );

            // ignore backspace
            if ( c != 0x7f )
            {
                // filter
                if ( isblank(c) )
                {
                    // unify whitespace to regular space
                    c = ' ';
                }
                else if ( c < 32 )
                {
                    // nonprintable characters -> underscore
                    c = '_';
                }
                // everything else is legal.

                // convert back to utf8
                utf8::append( c, writer );
            }
        }
    }
    catch(...)
    {
        // just leave the conversion incomplete in case of encoding errors

        // but not in debug mode. This shoudln't happen.
#ifdef DEBUG
        tERR_ERROR( "Invalid utf8 strings in the system." );
#endif
    }

    *this = out;
}

bool st_colorStrings=true;

static tConfItem<bool> cs("COLOR_STRINGS",st_colorStrings);

static int RTC(REAL x){
    int ret=int(x*255);
    if (ret<0)
        ret=0;
    if (ret>255)
        ret=255;
    return ret;
}

static char hex_array[]="0123456789abcdef";

static char int_to_hex(int i){
    if (i<0 || i >15)
        return 'Q';
    else
        return hex_array[i];
}

// *******************************************************************************
// *
// *   color code streaming
// *
// *******************************************************************************
//!
//!     Appends a color code to a colored string
//!     @param s the string to append to
//!     @param colorCode the colorcode to append
//!
// *******************************************************************************

tColoredString & operator <<(tColoredString &s, const tColoredStringProxy &colorCode )
{
    if (st_colorStrings)
    {
        if ( colorCode.r_ == -1 || colorCode.g_ == -1 || colorCode.b_ == -1 )
        {
            s << "0xRESETT";
            return s;
        }

        char cs[9];
        cs[0]='0';
        cs[1]='x';

        int RGB[3];
        RGB[0]=RTC(colorCode.r_);
        RGB[1]=RTC(colorCode.g_);
        RGB[2]=RTC(colorCode.b_);

        for(int i=0;i<3;i++){
            int lp=RGB[i]%16;
            int hp=(RGB[i]-lp)/16;
            cs[2+2*i]=int_to_hex(hp);
            cs[2+2*i+1]=int_to_hex(lp);
        }
        cs[8]=0;

        s << cs;
    }

    return s;
}

#ifdef _MSC_VER
// non-template replacements for VisualC
bool tString::Convert( int & target, size_type startPos ) const
{
    // generate string stream and advance it to the start position
    std::istringstream s(*this);
    s.seekg(startPos);

    // read it
    s >> target;

    // return failure condition
    return !s.fail() && s.eof() || isblank(s.get());
}

bool tString::Convert( REAL & target, size_type startPos ) const
{
    // generate string stream and advance it to the start position
    std::istringstream s(*this);
    s.seekg(startPos);

    // read it
    s >> target;

    // return failure condition
    return !s.fail() && s.eof() || isblank(s.get());
}

#endif

// *******************************************************************************
// *
// *   Scrapyard
// *
// *******************************************************************************

/*
tString & tString::operator<<(const char *c){
    return operator+=(c);
}

tString & tString::operator+=(const char *c){
    if (c){
        if ( Len() == 0 )
            operator[](0)='\0';

        int i=Len()-1;
        if (i<0) i=0;
        while (*c!='\0'){
            operator[](i)=*c;
            i++;
            c++;
        }
        operator[](i)='\0';
    }
    return *this;
}

tString & tString::operator=(const char *c){
    Clear();
    return operator+=(c);
}

tString tString::operator+(const char *c) const{
    tString s(*this);
    return s+=c;
    //return s;
}

tString::operator const char *() const{
    if (Len())
        return &operator()(0);
    else
        return "";
}

tString & tString::operator<<(char c){
    return operator+=(c);
}

tString & tString::operator+=(char c){
    if ( Len() == 0 )
        operator[](0)='\0';

    int i=Len();
    if (i<=0) i=1;
    operator[](i-1)=c;
    operator[](i)='\0';
    return *this;
}

tString tString::operator+(char c) const{
    tString s(*this);
    return s+=c;
    //return s;
}


std::ostream & operator<< (std::ostream &s,const tString &x){
    if(x.Len())
        return s << &(x(0));
    else
        return s;
}

  void operator <<(tString &s,const char * c){
  std::stringstream S(st_stringOutputBuffer,tMAX_STRING_OUTPUT-1);
  S << c << '\0';
  s+=st_stringOutputBuffer;
  }

  void operator <<(tString &s,const unsigned char * c){
  std::stringstream S(st_stringOutputBuffer,tMAX_STRING_OUTPUT-1);
  S << c << '\0';
  s+=st_stringOutputBuffer;
  }

  void operator <<(tString &s,int c){
  std::stringstream S(st_stringOutputBuffer,tMAX_STRING_OUTPUT-1);
  S << c << '\0';
  s+=st_stringOutputBuffer;
  }

  void operator <<(tString &s,float c){
  std::stringstream S(st_stringOutputBuffer,tMAX_STRING_OUTPUT-1);
  S << c << '\0';
  s+=st_stringOutputBuffer;
  }
*/


std::stringstream& operator<<(std::stringstream& s,const tString &t)
{
    static_cast<std::ostream&>(s) << static_cast<const char *>(t);
    return s;
}

/*
std::stringstream& operator<<(std::stringstream& s, const int &t)
{
	static_cast<std::ostream&>(s) << static_cast<int >(t);
	return s;
}

std::stringstream& operator<<(std::stringstream& s, const float &t)
{
	static_cast<std::ostream&>(s) << static_cast<float>(t);
	return s;
}

std::stringstream& operator<<(std::stringstream& s, const short unsigned int &t)
{
	static_cast<std::ostream&>(s) << static_cast<int>(t);
	return s;
}

std::stringstream& operator<<(std::stringstream& s, const short int &t)
{
	static_cast<std::ostream&>(s) << static_cast<int>(t);
	return s;
}

std::stringstream& operator<<(std::stringstream& s, const unsigned int &t)
{
	static_cast<std::ostream&>(s) << static_cast<int>(t);
	return s;
}

std::stringstream& operator<<(std::stringstream& s, const unsigned long &t)
{
	static_cast<std::ostream&>(s) << static_cast<int>(t);
	return s;
}

std::stringstream& operator<<(std::stringstream& s, char t)
{
	static_cast<std::ostream&>(s) << t;
	return s;
}

std::stringstream& operator<<(std::stringstream& s, bool t)
{
	static_cast<std::ostream&>(s) << static_cast<int>(t);
	return s;
}


std::stringstream& operator<<(std::stringstream& s, const char * const &t)
{
	static_cast<std::ostream&>(s) << static_cast<const char *>(t);
	return s;
}


tString & tString::operator=(const tString &s)
{
    // self copying is unsafe
    if ( &s == this )
        return *this;

    Clear();
    for (int i = s.Len()-1; i>=0; i--)
        operator[](i) = s(i);

    tASSERT( (*this) == s );

    return *this;
}

*/

/*

//static char* st_TempString = NULL;
#ifdef DEBUG
//static int   st_TempStringLength = 10;
#else
//static int   st_TempStringLength = 1000;
#endif

// static int

class tTempStringCleanup
{
public:
	~tTempStringCleanup()
		{
			if (st_TempString)
				free( st_TempString );

			st_TempString = NULL;
		}
};

static tTempStringCleanup cleanup;

char * tString::ReserveTempString()
{
	if (!st_TempString)
		st_TempString = reinterpret_cast<char*>( malloc(st_TempStringLength) );

	st_TempString[st_TempStringLength-1] = 0;
	st_TempString[st_TempStringLength-2] = 0;

	return st_TempString;
}


int    tString::TempStringLength()
{
	return st_TempStringLength;
}

void   tString::MakeTempStringLonger()
{
	free(st_TempString);
	st_TempString = NULL;
	st_TempStringLength *= 2;
}

// *******************************************************************************************
// *
// *	operator ==
// *
// *******************************************************************************************
//!
//!		@param	other	the string to compare with
//!		@return			true only if the two strings are equal
//!
// *******************************************************************************************

bool tString::operator ==( const char* other ) const
{
    return Compare( other ) == 0;
}

// *******************************************************************************************
// *
// *	operator !=
// *
// *******************************************************************************************
//!
//!		@param	other	the string to compare with
//!		@return			true only if the two strings are not equal
//!
// *******************************************************************************************

bool tString::operator !=( const char* other ) const
{
    return Compare( other ) != 0;
}

// *******************************************************************************************
// *
// *	operator <
// *
// *******************************************************************************************
//!
//!		@param	other	the string to compare with
//!		@return			true only if *this is lexicographically before other
//!
// *******************************************************************************************

bool tString::operator <( const char* other ) const
{
    return Compare( other ) < 0;
}

// *******************************************************************************************
// *
// *	operator>
// *
// *******************************************************************************************
//!
//!		@param	other	the string to compare with
//!		@return			true only if *this is lexicographically after other
//!
// *******************************************************************************************

bool tString::operator>( const char* other ) const
{
    return Compare( other ) > 0;
}

// *******************************************************************************************
// *
// *	operator <=
// *
// *******************************************************************************************
//!
//!		@param	other	the string to compare with
//!		@return			true only if *this is not lexicographically after other
//!
// *******************************************************************************************

bool tString::operator <=( const char* other ) const
{
    return Compare( other ) <= 0;
}

// *******************************************************************************************
// *
// *	operator >=
// *
// *******************************************************************************************
//!
//!		@param	other	the string to compare with
//!		@return			true only if *this is not lexicographically before other
//!
// *******************************************************************************************

bool tString::operator >=( const char* other ) const
{
    return Compare( other ) >= 0;
}

bool operator==( const char* first, const tString& second )
{
    return second == first;
}

bool operator!=( const char* first, const tString& second )
{
    return second != first;
}

bool operator<( const char* first, const tString& second )
{
    return second > first;
}

bool operator>( const char* first, const tString& second )
{
    return second < first;
}

bool operator<=( const char* first, const tString& second )
{
    return second >= first;
}

bool operator>=( const char* first, const tString& second )
{
    return second <= first;
}

bool operator==( const tString& first, const tString& second )
{
    return first.operator==( second );
}

bool operator!=( const tString& first, const tString& second )
{
    return first.operator!=( second );
}

bool operator<( const tString& first, const tString& second )
{
    return first.operator<( second );
}

bool operator>( const tString& first, const tString& second )
{
    return first.operator>( second );
}

bool operator<=( const tString& first, const tString& second )
{
    return first.operator<=( second );
}

bool operator>=( const tString& first, const tString& second )
{
    return first.operator>=( second );
}

void testconversion()
{
    float x;
    tString t("1");
    tVERIFY( t.Convert( x ) );
    t = "1.2 b s";
    tVERIFY( t.Convert( x ) );
    t = "bla";
    tVERIFY( !t.Convert( x ) );
    t = "1b";
    tVERIFY( !t.Convert( x ) );
}
*/


// *******************************************************************************************
// *
// *	tIsInList
// *
// *******************************************************************************************
//!
//!    @param      list       The string representation of the list
//!    @param      item       The item to look for
//!    @return     true if the list contains the item
//!
//!    Example: tIsInList( "ab, cd", "ab" ) -> true
//!
// *******************************************************************************************
//! check whether item is in a comma or whitespace separated list
bool tIsInList( tString const & list_, tString const & item )
{
    tString list = list_;

    while( list != "" )
    {
        // find the item
        int pos = list.StrPos( item );

        // no traditional match? shoot.
        if ( pos < 0 )
        {
            return false;
        }

        // check whether the match is a true list match
        if (
            ( pos == 0 || list[pos-1] == ',' || isblank(list[pos-1]) )
            &&
            ( pos + item.Len() >= list.Len() || list[pos+item.Len()-1] == ',' || isblank(list[pos+item.Len()-1]) )
            )
        {
            return true;
        }
        else
        {
            // no? truncate the list and go on.
            list = list.SubStr( pos + 1 );
        }
    }

    return false;
}

// **********************************************************************
// *
// *	tToLower
// *
// **********************************************************************
//!
//!    @param      toTransform   The string to transform
//!
// **********************************************************************
void tToLower( tString & toTransform )
{
    for( int i = toTransform.Len()-2; i >= 0; --i )
    {
        toTransform[i] = tolower( toTransform[i] );
    }
}

// **********************************************************************
// *
// *	tToUpper
// *
// **********************************************************************
//!
//!    @param      toTransform   The string to transform
//!
// **********************************************************************
void tToUpper( tString & toTransform )
{
    for( int i = toTransform.Len()-2; i >= 0; --i )
    {
        toTransform[i] = toupper( toTransform[i] );
    }
}

// **********************************************************************
// *
// *	Filter
// *
// **********************************************************************
//!
//!    @param      in            Character to process filters on
//!
// **********************************************************************

wchar_t tCharacterFilter::Filter( wchar_t in )
{
    if ( in < filter.Len() )
    {
        return filter[ in ];
    }
    else
    {
        // unknown characters get mapped to underscores.
        return '_';
    }
}

// **********************************************************************
// *
// *	FilterByteString
// *
// **********************************************************************
//!
//!    @param      in            Character to process filters on
//!
// **********************************************************************

tString tCharacterFilter::FilterByteString( tString const & s )
{
    int len = s.Len() -1;
    tString out;
    unsigned char c;
    for ( int i = 0; i < len; i++ )
    {
        c = Filter( s[i] );
        if ( c != 0x7f ) // If it's a del character, that means we want to strip it
        {
            out << c;
        }
    }

    return out;
}

// **********************************************************************
// *
// *	FilterString
// *
// **********************************************************************
//!
//!    @param      in            Character to process filters on
//!
// **********************************************************************

tString tCharacterFilter::FilterString( tString const & s )
{
    // output string
    tString out;

    // prepare reader and writer
    tString::const_iterator reader = s.begin();
    std::back_insert_iterator< tString > writer = back_inserter(out);

    try
    {
        // filter out illegal characters
        while( reader != s.end() )
        {
            // convert from utf8
            wchar_t c = utf8::next( reader, s.end() );
            // Check if we don't want to strip it anyway
            wchar_t f = Filter( c );
            if ( f != 0x7f )
            {
                // filter and convert back to utf8
                utf8::append( f, writer );
            }
        }
    }
    catch(...)
    {
        // just leave the conversion incomplete in case of encoding errors

        // but not in debug mode. This shoudln't happen.
#ifdef DEBUG
        tERR_ERROR( "Invalid utf8 strings in the system." );
#endif
    }

    return out;
}

// **********************************************************************
// *
// *	SetMap
// *
// **********************************************************************
//!
//!    @param      in1           Begin of the character range to process filters on
//!    @param      in2           End of the character range
//!    @param      out           Filtered character
//!
// **********************************************************************

void tCharacterFilter::SetMap( wchar_t in1, wchar_t in2, wchar_t out)
{
    // fill gaps with underscores
    for( wchar_t i = filter.Len(); i < in1; ++i )
    {
        filter[ i ] = '_';
    }

    tASSERT( in1 <= in2 );
    for( wchar_t i = in2; i >= in1; --i )
        filter[ i ] = out;
}

// **********************************************************************
// *
// *	SetMap
// *
// **********************************************************************
//!
//!    @param      in            Character to process filters on
//!    @param      out           Filtered character
//!
// **********************************************************************

void tCharacterFilter::SetMap( wchar_t in, wchar_t out )
{
    SetMap( in, in, out );
}

tString st_GetCurrentTime( char const * szFormat )
{
    char szTemp[128];
    time_t     now;
    struct tm *pTime;
    now = time(NULL);
    pTime = localtime(&now);
    strftime(szTemp,sizeof(szTemp),szFormat,pTime);
    return tString(szTemp);
}
