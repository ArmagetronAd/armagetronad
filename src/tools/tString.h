
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

#ifndef ArmageTron_STRING_H
#define ArmageTron_STRING_H

#include "config.h"

#include "defs.h"

#include <string>
#include <sstream>
#include <iostream>
#include <iosfwd>

// macros that help wrapping operators defined for a base class, they probably
// should be refactored into another header. They assume THISCLASS is a typedef to the
// class just being defined and BASE the base class.

//! wraps a mutating operator (+=, -=, *=). op is the operator and T the argument type.
#define WRAP_MUTATING_OPERATOR(op, T) inline THISCLASS & operator op( T t ){ static_cast< BASE & >( *this ) op t; return *this; }
//! wraps a mutating operator (+=, -=, *=) for all argument types. op is the operator.
#define WRAP_MUTATING_OPERATOR_ALLTYPES(op) template< class T > WRAP_MUTATING_OPERATOR(op, T const &)

//! wraps a non-mutating binary operator (+, -, *). op is the operator and T the second argument.
#define WRAP_OPERATOR(op, T) inline THISCLASS operator op( T t ) const { return THISCLASS( static_cast< const BASE & >( *this ) op  t ); }
//! wraps a non-mutating binary operator (+, -, *) for second arguments. op is the operator.
#define WRAP_OPERATOR_ALLTYPES(op) template< class T > WRAP_OPERATOR(op, T const &)

//typedef tArray<char> string;
class tOutput;

using std::string;

//! string class: standard string with some extras
class tString:public string{
private:
public:
    typedef tString          THISCLASS; //!< typedef for this class
    typedef std::string      BASE;      //!< typedef for base clase
    typedef BASE::value_type CHAR;      //!< the character class

    tString();                         //!< default constructor
    tString(const BASE &);             //!< pseudo copy constructor
    tString(const tString &);          //!< copy constructor
    explicit tString(const CHAR *);    //!< conversion from C string
    template<typename T>
    tString(T begin, T end) : string(begin,end) {} //!< initialisation by iterators
    // explicit tString(const char *);    //!< conversion from C string
    // explicit tString(const tOutput &); //!< conversion from output

    // wrap base operators so they have the correct type
    // if you end up here in the debugger, don't bother to look up
    WRAP_OPERATOR_ALLTYPES(+)              // concatenation
    WRAP_MUTATING_OPERATOR_ALLTYPES(+=)    // appending
    WRAP_MUTATING_OPERATOR(=,CHAR const *) // assignment of C string
    WRAP_MUTATING_OPERATOR(=,BASE const &) // assignment from C++ string

    tString & operator =( tOutput const & other ); //!< assignment from output collector

    size_type Size() const;                        //!< Returns the size of the string in characters.

    //! Converts the string to an arbitrary type, starting at the given position.
#ifdef _MSC_VER
    bool Convert( int & target, size_type startPos = 0 ) const;
    bool Convert( REAL & target, size_type startPos = 0 ) const;
#else
    template < class T > bool Convert( T & target, size_type startPos = 0 ) const;
#endif

    void ReadLine(std::istream &s, bool enableEscapeSequences=false); //!< read a whole line from a stream into this string

    void Clear();                       //!< clears the string
    void SetPos( int len, bool cut );   //!< makes this string exactly of length len.

    bool StartsWith( const tString & other ) const; //!< determines whether this string starts with the argument string
    bool StartsWith( const CHAR * other ) const;    //!< determines whether this string starts with the argument string

    bool EndsWith( const tString & other) const;    //!< determines whether this string ends with the argument string
    bool EndsWith( const CHAR* other) const;        //!< determines whether this string ends with the argument string

    tString LTrim() const;                          //!< returns a copy with leading whitespace removed
    tString RTrim() const;                          //!< returns a copy with trailing whitespace removed
    tString Trim() const;                           //!< returns a copy with leading and trailing whitespace removed

    int StrPos( const tString &tofind ) const;      //!< Get the position of a substring within this string.
    int StrPos( const CHAR * tofind ) const;        //!< Get the position of a substring within this string.

    tString SubStr( int start, int len) const;      //!< Get an arbitrary substring within this string.
    tString SubStr( int start ) const;              //!< Get a substring within this string from the end.

    int ToInt( size_type pos = 0 ) const;           //!< Returns the string converted to an integer.
    REAL ToFloat( size_type pos = 0 ) const;        //!< Returns the string converted to a float.

    //! Compares two strings alphanumerically
    static int CompareAlphaNumerical( const tString& a, const tString &b);

    //! Strips all whitespace from a string
    tString StripWhitespace( void ) const;

    //! Converts the string to lowercase
    tString ToLower(void) const;
    //! Converts the string to uppercase
    tString ToUpper(void) const;

    int PosWordRight(int start) const;          //!< Computes the position of the next delimiter relative to start
    int PosWordLeft(int start) const;           //!< Computes the position of the previous delimiter relative to start
    int RemoveWordRight(int start);             //!< Remove word right according to the delimiters
    int RemoveWordLeft(int start);              //!< Remove word left according to the delimiters
    void RemoveSubStr(int start, int length);   //!< Remove a substring, in-place
    tString GetFileMimeExtension() const;       //!< Gets the lowercased file extension, as in a MIME type
    tString Reverse() const;                    //!< Reverses strings

    tString Truncate( int truncateAt ) const;   //!< Returns a truncated string (copy of this with excess chars replaced by "...")


    // TO BE REPLACED
    // auto-expanding subscription operators, will become non-expanding
    CHAR operator[]( size_t i ) const;
    CHAR & operator[]( size_t i );
    CHAR operator[]( int i ) const;
    CHAR & operator[]( int i );

    void SetSize( unsigned int size ); //!< sets the string length by cutting or appending spaces

#ifdef NOLEGACY
private:
#endif
    // non-expanding subscription operators, will disappear
    CHAR operator()( size_t i ) const;
    CHAR & operator()( size_t i );
    CHAR operator()( int i ) const;
    CHAR & operator()( int i );

    // comparison operators
    int Compare( const CHAR* other ) const;		             //!< Compares two strings lexicographically
    int Compare( const CHAR* other, bool ignoreCase ) const; //!< Compares two strings lexicographically, ignoring case differences if second argument is true

    // automatic conversion will go away, it's dangerous
    operator const CHAR*() const;

    int Count(CHAR what) const; //!< Counts the number of a certain character within the string
    int LongestLine() const; //!< the length of the longest line

    // LEGACY FUNCTIONS, DON'T USE IN NEW CODE
    int Len() const; //!< returns the lenghth PLUS ONE (the allocated length)

    void SetLen( int len ); //!< sets the allocated length
};

//! proxy class for inserting color markings
struct tColoredStringProxy
{
    REAL r_, g_, b_;

    tColoredStringProxy( REAL r, REAL g, REAL b )
            : r_(r), g_(g), b_(b)
    {}
};

//! strings that know about color markings
class tColoredString: public tString
{
public:
    typedef tColoredString   THISCLASS; //!< typedef for this class
    typedef tString          BASE;      //!< typedef for base clase

    ~tColoredString();                                      //!< Destructor
    tColoredString();                                       //!< Default constructor
    tColoredString( const tColoredString& other );          //!< Copy constructor
    explicit tColoredString( const tString& other );        //!< Base copy constructor
    explicit tColoredString( const CHAR * other );          //!< Constructor from raw C string
    explicit tColoredString( const tOutput & other );       //!< Constructor from output gatherer

    //! Assignment operators
    WRAP_MUTATING_OPERATOR(=,CHAR const *)    // assignment of C string
    WRAP_MUTATING_OPERATOR(=,BASE const &)    // assignment from C++ string
    WRAP_MUTATING_OPERATOR(=,tOutput const &) // assignment from output collector

    static tString RemoveColors( const CHAR *c );           //!< Removes the color codes from a string
    void SetPos( int len, bool cut=false );                 //!< Makes sure string has length len when color codes are removed

    void RemoveTrailingColor();                             //!< Removes trailing, unfinished color code
    void NetFilter();                                       //!< filters strings from the net for strange things like newlinesa

    // void RemoveHex();                                       //!< ?

    int LongestLine() const;

    //! Creates a color string inserter
    inline static tColoredStringProxy ColorString( REAL r,REAL g,REAL b )
    {
        return tColoredStringProxy( r, g, b );
    }
};

//! string building streaming operator
template<class T> tString & operator <<(tString &s,const T &c)
{
    std::ostringstream S;

    S << c;

    return s += S.str();
}

//! string building streaming operator
tColoredString & operator <<(tColoredString &s, const tColoredStringProxy &colorCode );

//! string building streaming operator
template<class T> tColoredString & operator <<(tColoredString &s,const T &c)
{
    // delegate to basic string function...
    static_cast< tString& >( s ) << c;

    // but don't allow overhanging color codes
    s.RemoveTrailingColor();

    return s;
}

//! stream reading operator honoring escape sequences and quoting
std::istream & operator>> (std::istream &s,tString &x);

#ifndef _MSC_VER
// *******************************************************************************
// *
// *	Convert
// *
// *******************************************************************************
//!
//!		@param	target	  the variable to write the conversion result to
//!		@param	startPos  the position to start the conversion at
//!		@return		      true on success
//!
// *******************************************************************************

template< class T >
bool tString::Convert( T & target, size_type startPos ) const
{
    // generate string stream and advance it to the start position
    std::istringstream s(*this);
    s.seekg(startPos);

    // read it
    s >> target;

    // return failure condition
    return !s.fail() && s.eof() || isspace(s.get());
}
#endif

#endif


