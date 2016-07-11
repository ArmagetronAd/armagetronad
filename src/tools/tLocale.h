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

**************************************************************************
*/

#ifndef ArmageTron_tLocale_H
#define ArmageTron_tLocale_H

#include "tMemManager.h"
#include "tString.h"
#include "tLinkedList.h"

extern const tString st_internalEncoding;

class tLocaleItem;
class tOutputItemBase;

class tLanguage: public tListItem<tLanguage>      // identifies a language
{
    tString name;           //!< the language's name
    mutable tString file;   //!< filename to load the language from (empty if the language was already loaded)
public:
    tLanguage(const tString& n);

    static tLanguage* FirstLanguage();

    void SetFirstLanguage()  const;  // set this language as the favorite choice
    void SetSecondLanguage() const;

    void Load() const; //!< loads this language fully
    void LoadLater( char const * file ) const; //!< loads this language later from this file

    static tLanguage* Find(tString const & name); //!< finds a language with the specified name. Never fails, creates new language on demand.
    static tLanguage* FindStrict(tString const & name); //!< finds a language with the specified name. Aborts if the language is not found.
    static tLanguage* FindSloppy(tString const & name); //!< finds a language with the specified name. Returns NULL if the language is not found.

    const tString& Name(){
        return name;
    }
};


// flexible output unit: the output string is generated on the fly from
// the components.

/* usage: like a normal string:
   int i=5;

   tOutput x;
   x << "menu_exit";  // lets x print the TEXT ITEM menu_exit from the language files
   x << i;            // concatenate i to it
   x << tString("menu_exit"); // lets x print the LITERAL string "menu_exit".
   x.Literal("menu_exit");   // the same.

   std::cout << x;
   // prints Menu Exit5menu_exitmenu_exit
*/

class tOutput{
    friend class tOutputItemBase;

    tOutputItemBase *anchor;

    // tOutput& operator << (const tOutput &o);
public:
    tOutput();
    ~tOutput();

    operator const char *() const;    // creates the output string
    //  operator tString() const;    // creates the output string
    void AddLiteral(const char *);       // adds a language independent string
    void AddLocale(const char *);        // adds a language dependant string
    void AddSpace();                     // adds a simple space
    void AddString(char const * pString); // checks the string, delegates to correct Add...()-Function

    // set a template parameter at this position of the output string
    tOutput & SetTemplateParameter(int num, const char *parameter);
    tOutput & SetTemplateParameter(int num, int         parameter);
    tOutput & SetTemplateParameter(int num, float       parameter);

    // delete all elements
    void Clear();

    tOutput(const tString& identifier);
    tOutput(const char * identifier);
    tOutput(const tLocaleItem &locale);

    // convenience template constructors: fill in template parameters with passed value
    template< class T1>
    tOutput( char const * identifier, T1 const & template1 )
            :anchor(NULL)
    {
        tASSERT( identifier && identifier[0] == '$' );

        SetTemplateParameter(1, template1);

        AddString(identifier);
    }

    template< class T1, class T2 >
    tOutput( char const * identifier, T1 const & template1, T2 const & template2 )
            :anchor(NULL)
    {
        tASSERT( identifier && identifier[0] == '$' );

        SetTemplateParameter(1, template1);
        SetTemplateParameter(2, template2);

        AddString(identifier);
    }

    template< class T1, class T2, class T3 >
    tOutput( char const * identifier, T1 const & template1, T2 const & template2, T3 const & template3 )
            :anchor(NULL)
    {
        tASSERT( identifier && identifier[0] == '$' );

        SetTemplateParameter(1, template1);
        SetTemplateParameter(2, template2);
        SetTemplateParameter(3, template3);

        AddString(identifier);
    }

    template< class T1, class T2, class T3, class T4 >
    tOutput( char const * identifier, T1 const & template1, T2 const & template2, T3 const & template3, T4 const & template4 )
            :anchor(NULL)
    {
        tASSERT( identifier && identifier[0] == '$' );

        SetTemplateParameter(1, template1);
        SetTemplateParameter(2, template2);
        SetTemplateParameter(3, template3);
        SetTemplateParameter(4, template4);

        AddString(identifier);
    }

    tOutput(const tOutput &o); // copy constructor
    tOutput& operator=(const tOutput &o); // copy operator

    void Append(const tOutput &o);

    bool IsEmpty()const {
        return !anchor;
    }
};

class tOutputItemBase: public tListItem<tOutputItemBase>
{
public:
    tOutputItemBase(tOutput& o);
    virtual ~tOutputItemBase();
    virtual void Print(tString& target) const = 0;
    virtual void Clone(tOutput& o)      const = 0;
};


template <class T>class tOutputItem: public tOutputItemBase
{
    T element;
    public:
tOutputItem(tOutput& o, const T& e): tOutputItemBase(o), element(e){}
virtual void Print(tString& target) const {
    target << element;
}
virtual void Clone(tOutput& o)      const {
    tNEW(tOutputItem<T>)(o, element);
}
};



// now the print template:
template<class T> tOutput&  operator<< (tOutput& o, const T& e)
{
    tNEW(tOutputItem<T>)(o, e);
    return o;
}

// and a special implementation for the locales and strings:
tOutput& operator << (tOutput &o, const char *locale);
tOutput& operator << (tOutput &o, char *locale);

// output operators
std::ostream& operator<< (std::ostream& s, const tOutput& o);
//std::stringstream& operator<< (std::stringstream& s, const tOutput& o);
tString& operator<< (tString& s, const tOutput& o);



// interface class for locale items
class tLocale
{
public:
    static const tLocaleItem& Find(const char* id);
    static void               Load(const char* filename);
    static void               Clear();           // clear all locale data on program exit
};

std::ostream& operator<< (std::ostream& s, const tLocaleItem& o);
//std::stringstream& operator<<(std::stringstream& s, const tLocaleItem &t);
tString& operator<< (tString& s, const tLocaleItem& o);


#endif
