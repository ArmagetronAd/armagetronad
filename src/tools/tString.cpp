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

#include "tMemManager.h"
#include "tString.h"
#include "tColor.h"
#include "tLocale.h"
#include "tConfiguration.h"
#include <ctype.h>
#include <time.h>
#include <string>
#include <iostream>

tString::tString(){
    operator[](0)='\0';
}

tString::tString(const char *x){
    operator[](0)='\0';
    operator+=(x);
}

tString::tString(const tString &x)
        :tArray< char >()
{
    operator[](0)='\0';
    operator=(x);

    tASSERT( (*this) == x );
}

tString::tString(const tOutput &x){
    operator[](0)='\0';
    operator=(x);
}

// reads an escape sequence from a stream
// c: the character just read from the stream
// c2: eventually set to the second character read
// s: the stream to read from
static bool st_ReadEscapeSequence( char & c, char & c2, std::istream & s )
{
    c2 = '\0';

    // detect escaping
    if ( c == '\\' )
    {
        c = s.get();

        // nothing useful read?
        if ( s.eof() )
        {
            c = '\\';
            return false;
        }

        // interpret special escape sequences
        switch (c)
        {
        case 'n':
            // turn \n into newline
            c = '\n';
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
            c2 = c;
            c = '\\';
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

void tString::ReadLine(std::istream &s, bool enableEscapeSequences ){
    char c=' ';
    int i=0;
    SetLen(0);

    // note: this is not the same as std::ws(s), we're stopping at newlines.
    while(c!='\n' && c!='\r' && isblank(c) &&  s.good() && !s.eof()){
        c=s.get();
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
                operator[](i++)=c;
                c = 'x';
                continue;
            }
            else if ( c2 )
            {
                operator[](i++)=c;
                operator[](i++)=c2;
                c = 'x';
                continue;
            }
        }
        operator[](i++)=c;
    }

    operator[](i)='\0';
}

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

tString & tString::operator=(const tOutput& o){
    Clear();
    return operator <<(o);
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

std::istream & operator>> (std::istream &s,tString &x){
    int i=0;
    x.SetLen(0);

    // eat whitespace
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
    while((quoted || !( isblank(c) || c == '\n' || c == '\r') ) && s.good() && !s.eof()){
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
            x[i++]=c;
            c=s.get();
        }

        // lastEscape = thisEscape;
    }

    if(s.good())
    {
        s.putback(c);
    }
    x[i]='\0';
    return s;
}

#ifdef DEBUG
class tQuoteTester
{
    // checks that a specific input produces a specific output when read via <<
    static void Test( char const * in, char const * out )
    {
        std::stringstream s;
        s << in << " NEXTWORD";
        tString o;
        s >> o;
        tASSERT( o == out );
    }

public:
    tQuoteTester()
    {
        // basic stuff
        Test( "abc", "abc" );
        
        // quoted
        Test( "'abc'", "abc" );

        // escaped
        Test( "\\'a\\\"b", "'a\"b" );
        
        // chained backslashes
        Test( "\\\\", "\\" );
        Test( "\\\\\\\\", "\\\\" );
        Test( "'\\\\\\\\'", "\\\\" );
    }
};

static tQuoteTester tester;
#endif

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
                SetLen( l - 1 );
                operator+=(' ');
            }
            else
            {
                SetLen( 0 );
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



//added by me (Tank Program)
//sees if a string starts with another string
//created for remote admin...

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
        if (other(i) != (*this)(i)) {
            return false;
        }
    }

    return true;
}

int tString::StrPos( const tString &tofind ) const
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

int tString::StrPos( const char * tofind ) const {
    return StrPos( tString ( tofind ) );
}

tString tString::SubStr( const int start, int len) const
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

tString tString::SubStr( const int start ) const
{
    return SubStr (start, Len()-start-1 );
}

//based on GetInt
int tString::toInt( int pos ) const
{
    int ret = 0;
    int digit = 0;
    while ( pos < Len() && digit >= 0 && digit <= 9 )
    {
        ret = ret*10 + digit;
        digit = (*this)(pos) - '0';
        pos++;
    }

    // check whether the last character read was not whitespace or the end of the string
    if ( pos < Len() && !isblank((*this)(pos-1)) )
    {
        return 0;
    }

    return ret;
}

int tString::toInt() const {
    return toInt(0);
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


/*
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
*/


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


tString & tString::operator+=(const tString &s)
{
    if (Len() > 0 && operator()(Len()-1) == 0)
        SetLen(Len()-1);

    for (int i = 0; i< s.Len(); i++)
        operator[](Len()) = s(i);

    if (Len() > 0 && operator()(Len()-1) != 0)
        operator[](Len()) = 0;

    return *this;
}

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

*/

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
// *	Compare
// *
// *******************************************************************************************
//!
//!     @param	other       the string to compare with
//!     @param  ignoreCase
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
// *	operator =
// *
// *******************************************************************************************
//!
//!		@param	c	C string to copy from
//!     @return     reference to this to chain assignments
//!
// *******************************************************************************************

tString & tColoredString::operator =( const char * c )
{
    tString::operator = ( c );
    return *this;
}

// *******************************************************************************************
// *
// *	operator =
// *
// *******************************************************************************************
//!
//!		@param	s	string to copy from
//!     @return     reference to this to chain assignments
//!
// *******************************************************************************************

tString & tColoredString::operator =( const tString & s )
{
    tString::operator = ( s );
    return *this;
}

// *******************************************************************************************
// *
// *	operator =
// *
// *******************************************************************************************
//!
//!		@param	s   output to copy from
//!		@return		reference to this to chain assignments
//!
// *******************************************************************************************

tString & tColoredString::operator =( const tOutput & s )
{
    tString::operator = ( s );
    return *this;
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
    for ( int g=0; g < s.Len()-1; g++)
    {
        if (s(g) == '0' && s(g+1) == 'x')
        {
            // test if the code is legal ( not so far at the end that it overlaps )
            if ( s.Len() >= g + 9 )
            {
                // everything is in order, record color code usage and advance
                posDisplacement+=8;
                g+=7;
            }
            else
            {
                // illegal code! Remove it.
                s.SetLen( g );
                s[g]=0;
            }
        }
        else if ( maxLen > 0 )
        {
            if ( ++len >= maxLen )
            {
                // maximal end reached, cut it off
                s.SetLen( g );
                s[g]=0;
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

// filters illegal characters
class tCharacterFilter
{
public:
    tCharacterFilter()
    {
        int i;
        filter[0]=0;

        // map all unknown characters to underscores
        for (i=255; i>=0; --i)
        {
            filter[i] = '_';
        }

        // leave ASCII characters as they are
        // for (i=127; i>=32; --i)
        // no, leave all visible ISO Latin 1 characters as they are
        for (i=255; i>=32; --i)
        {
            filter[i] = i;
        }
        filter[0xA0] = ' '; // non-breaking space

        // map return and tab to space
        SetMap('\n',' ');
        SetMap('\t',' ');

        //! map umlauts and stuff to their base characters
        /*
        SetMap(0xc0,0xc5,'A');
        SetMap(0xd1,0xd6,'O');
        SetMap(0xd9,0xdD,'U');
        SetMap(0xdf,'s');
        SetMap(0xe0,0xe5,'a');
        SetMap(0xe8,0xeb,'e');
        SetMap(0xec,0xef,'i');
        SetMap(0xf0,0xf6,'o');
        SetMap(0xf9,0xfc,'u');
        */

        //!todo: map weird chars
        // make this data driven.
    }

    char Filter( unsigned char in )
    {
        return filter[ static_cast< unsigned int >( in )];
    }
private:
    void SetMap( int in1, int in2, unsigned char out)
    {
        tASSERT( in2 <= 0xff );
        tASSERT( 0 <= in1 );
        tASSERT( in1 < in2 );
        for( int i = in2; i >= in1; --i )
            filter[ i ] = out;
    }

    void SetMap( unsigned char in, unsigned char out)
    {
        filter[ static_cast< unsigned int >( in ) ] = out;
    }

    char filter[256];
};


// *******************************************************************************************
// *
// *	NetFilter
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

void tString::NetFilter( void )
{
    static tCharacterFilter filter;

    // run through string
    for( int i = Len()-2; i>=0; --i )
    {
        // character to filter
        char & my = (*this)(i);

        my = filter.Filter(my);
    }
}

// *******************************************************************************************
// *
// *	RemoveHex
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

void tColoredString::RemoveHex( void )
{
    /*tString oldS = *this;
    tString newS = "";

    for (int i=0; i<oldS.Len()-1; i++){
        if (oldS[i] == '0'){
            if (((oldS[i]+1) != oldS.Len()) && ((oldS[i+1] == 'x') || (oldS[i+1] == 'X')))
     {
     int r;
     int g;
    int b;
    char s[2];
    s[0] = oldS[i+2];
    s[1] = oldS[i+3];
    sscanf(s, "%x", &r);
    s[0] = oldS[i+4];
    s[1] = oldS[i+5];
    sscanf(s, "%x", &g);
    s[0] = oldS[i+6];
    s[1] = oldS[i+7];
    sscanf(s, "%x", &b);
    newS << ColorString(r/255, g/255, b/255);
    i = i + 8;
                if (i>=oldS.Len()){
                    break;
                }
            }
        }
        newS << oldS[i];
    }
    //        for (i=0; i<newS.Len()-1; i++){
    //        if (newS[i] == '0'){
    //            if (((newS[i]+1) != newS.Len()) && ((newS[i+1] == 'x') || (newS[i+1] == 'X'))){
    //    newS.RemoveHex();
    //    }
    //    }
    //    } 
    //this = newS; */
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
    int strLen = Len()-1;
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
        *this = SubStr( 0, start ) << SubStr( start + length, strLen );
    }

    SetLen(strLen+1-length);
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

    return SubStr( 0, truncateAt ) << "...";
}

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

// replacement for tString::EndsWith from the trunk
bool st_StringEndsWith( tString const & test, tString const & end )
{
    int start = test.Len() - end.Len();
    return start >= 0 && test.SubStr( start ) == end;
}

bool st_StringEndsWith( tString const & test, char const * end )
{
    return st_StringEndsWith( test, tString( end ) );
}
