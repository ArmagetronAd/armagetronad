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
#include "tLocale.h"
#include "tConsole.h"
#include "tDirectories.h"
#include "tSafePTR.h"

#include <fstream>
#include <string>
#include <map>

class tLocaleSubItem; // identifies a single string in a single language

static tArray<tString> st_TemplateParameters;

static tString s_gameName("Armagetron"); // the official name of this game

class tLocaleItem: public tReferencable< tLocaleItem >    // idendifies a string in all languages
{
    friend class tLocaleSubItem;

    tString identifier;     // the cross-language identifier of this string
    tLocaleSubItem *items;  // the versions of various languages
    bool istemplate;        // does it contain \i-directives?

public:
    // static void Check();    // display warnings for all strings not defined in
    // the favorite language

    //  operator tString() const; // return the version of this string in the favorite language
    operator const char *() const;

    tLocaleItem(const char *identifier); // constructor taking the string identifier
    ~tLocaleItem();

    static void Load(const char *file, bool complete = true );  // load the language definitions from a file

    static void Clear();           // clear all locale data on program exit

    static tLocaleItem& Find(const char *identifier); // find identifier
};


static const tLanguage* st_firstLanguage  = NULL;
static const tLanguage* st_secondLanguage = NULL;

static tLanguage*      st_languageAnchor = NULL;

typedef std::map< std::string, tJUST_CONTROLLED_PTR< tLocaleItem > > tLocaleItemMap;

// static tLocaleItem*    st_localeAnchor   = NULL;

static tLanguage *currentLanguage = NULL;


class tLocaleSubItem: public tListItem<tLocaleSubItem>
{
public:
    const tLanguage *language;  // the language this string is in
    tString translation;        // the string itself

    tLocaleSubItem(tLocaleItem *item) // adds this SubItem to item
            : tListItem<tLocaleSubItem>(item->items){}
};



tLanguage::tLanguage(const tString& n)
        :tListItem<tLanguage>(st_languageAnchor),
        name(n)
{
}


tLanguage* tLanguage::FirstLanguage()
{
    return st_languageAnchor;
}

void tLanguage::SetFirstLanguage()  const
{
    st_firstLanguage = this;
    Load();
}

void tLanguage::SetSecondLanguage() const
{
    st_secondLanguage = this;
    Load();
}

void tLanguage::Load() const
{
    if (file.Len() > 1)
    {
        tLocaleItem::Load( file );
        file.Clear();
    }
}

void tLanguage::LoadLater( char const * file ) const
{
    // store name for later loading
    this->file = file;
}

tLanguage* tLanguage::Find(tString const & name )
{
    tLanguage *ret = FindSloppy( name );

    if (ret)
        return ret;

    return tNEW(tLanguage(name));
}

tLanguage* tLanguage::FindStrict(tString const & name )
{
    tLanguage *ret = FindSloppy( name );

    if (ret)
        return ret;

    tERR_ERROR_INT("Language " << name << " not found.");
    return NULL;
}

tLanguage* tLanguage::FindSloppy(tString const & name )
{
    tLanguage *ret = st_languageAnchor;
    while (ret)
    {
        if (ret->name == name)
            return ret;
        ret = ret->Next();
    }

    return NULL;
}


/*
void tLocaleItem::Check()
{
    tLocaleItem *run = st_localeAnchor;
    while (run)
    {
        tLocaleSubItem *r = run->items;
        bool ok = false;
        while (r)
        {
            if (r->language == currentLanguage)
                ok = true;
            r = r->Next();
        }

        if (!ok)
        {
            //			con << "Identifier " << run->identifier << " not translated.\n";
            con << run->identifier << "\n";
        }
        run = run->Next();
    }
}
*/

tLocaleItem::operator const char *() const// return the version of this string in the favorite language
{
    static tLanguage * english = tLanguage::FindStrict( tString("British English") );

    tString *first = NULL, *second = NULL, *third = NULL, *fourth = NULL;
    const tString *ret = NULL;

    tLocaleSubItem *run = items;
    while (run)
    {
        if (st_firstLanguage && run->language == st_firstLanguage)
            first = &run->translation;

        if (st_secondLanguage && run->language == st_secondLanguage)
            second = &run->translation;

        if (run->language == english)
            third = &run->translation;

        fourth = &run->translation;
        run = run->Next();
    }

    if (first)
        ret = first;
    else if (second)
        ret = second;
    else
    {
        if (third)
            ret = third;
        else
        {
            // load english and try again
            static bool loadEnglish = true;
            if ( loadEnglish )
            {
                loadEnglish = false;
                english->Load();
                return *this;
            }
        }

        if (!ret && fourth)
            ret = fourth;
        if (!ret)
            ret = &identifier;
    }

    if (!istemplate)
        return *ret;   // no template replacements need to be made
    else
    {
        const tString& temp = *ret;
        static tString        replaced;
        replaced.Clear();
        for(int i = 0; i < temp.Len(); i++)
        {
            char c = temp(i);
            if (c != '\\')
                replaced += c;
            else if (i < temp.Len() - 1)
            {
                c = temp(i+1);
                if (c == 'g')
                    replaced << s_gameName;
                else
                {
                    int index = c-'0';
                    if (index > 0 && index < 10)
                        replaced << st_TemplateParameters[index];
                    else if (temp(i+1) == '\\')
                        replaced << '\n';
                }
                i++;
            }
        }

        return replaced;
    }
}


tLocaleItem::tLocaleItem(const char *id) // constructor taking the string identifier
//:tListItem<tLocaleItem>(st_localeAnchor),
        :identifier(id), items(NULL), istemplate(false)
{
}

tLocaleItem::~tLocaleItem()
{
    while (items)
        delete (items);
}


void tLocaleItem::Clear()
{
    //while (st_localeAnchor)
    //    delete (st_localeAnchor);

    while (st_languageAnchor)
        delete (st_languageAnchor);
}


tLocaleItem& tLocaleItem::Find(const char *nn)
{
    /*
    tLocaleItem *ret = st_localeAnchor;
     tString n(nn);

    while (ret)
    {
        if (ret->identifier == n)
            return *ret;
        ret = ret->Next();
    }


    return *tNEW(tLocaleItem(n));
    */
    static tLocaleItemMap    st_localeMap;

    tJUST_CONTROLLED_PTR< tLocaleItem > & ret = st_localeMap[ nn ];

    if ( !ret )
        ret = tNEW(tLocaleItem(nn));

    return *ret;
}

static const tString LANGUAGE("language");
static const tString INCLUDE("include");
// static const tString CHECK("check");

void tLocaleItem::Load(const char *file, bool complete)  // load the language definitions from a file
{
    // bool check = false;
    {
        tString f;

        f <<  "language/" << file;

        std::ifstream s;
        if ( tDirectories::Data().Open( s, f ) )
        {
            while (!s.eof() && s.good())
            {
                tString id;
                s >> id;

                if (id.Len() <= 1 )
                {
                    continue;
                }

                if(id[0] == '#')
                {
                    tString dummy;
                    dummy.ReadLine(s);
                    continue;
                }

                if (LANGUAGE == id)
                {
                    tString lang;
                    lang.ReadLine(s);
                    currentLanguage = tLanguage::Find(lang);

                    // delayed loading
                    if (!complete)
                    {
                        currentLanguage->LoadLater(file);
                        return;
                    }
                    continue;
                }

                if (INCLUDE == id)
                {
                    tString inc;
                    inc.ReadLine(s);
                    Load(inc, complete);
                    continue;
                }

                /*
                if (CHECK == id)
                {
                    check = true;
                    tString dummy;
                    dummy.ReadLine(s);
                    continue;
                }
                */

                // id is a true string identifier.
                tLocaleItem &li = Find(id);
                tLocaleSubItem *r = li.items;
                bool done = false;
                while (r && !done)
                {
                    if (r->language == currentLanguage)
                    {
                        // r->translation.ReadLine(s);
                        done = true;
                        continue;
                    }
                    r = r->Next();
                }

                if (!done)
                    r = tNEW(tLocaleSubItem)(&li);
                else
                {
                    // st_Breakpoint();
                    // con << "Locale item " << id << " defined twice in language "
                    // << currentLanguage->Name() << ".\n";
                }

                tString pre;
                pre.ReadLine(s);
                r->translation.Clear();

                for (int i=0; i< pre.Len(); i++)
                {
                    char c = pre(i);
                    if (c != '\\')
                        r->translation += c;
                    else if (i < pre.Len()-1)
                        switch (pre(i+1))
                        {
                        case 'n':
                            r->translation += '\n';
                            i++;
                            break;

                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9':
                        case 'g':
                            r->translation += '\\';
                            li.istemplate = true;
                            break;

                            /*
                            		 case '\\':
                            		 r->translation << "\\\\";
                            		 i++;
                            */

                        default:
                            r->translation += '\\';
                            break;
                        }
                }

                r->language = currentLanguage;
                //	r->translation.ReadLine(s);


            }
        }
    }

    /*
    if (check)
        Check();
    */
}

class tOutputItemLocale: public tOutputItemBase
{
    const tLocaleItem *element;
public:
    tOutputItemLocale(tOutput& o, const tLocaleItem& e);
    virtual void Print(tString& target) const;
    virtual void Clone(tOutput& o)      const;
};

class tOutputItemSpace: public tOutputItemBase
{
public:
    tOutputItemSpace(tOutput& o);
    virtual void Print(tString& target) const;
    virtual void Clone(tOutput& o)      const;
};


class tOutputItemTemplate: public tOutputItemBase
{
    int       num;
    tString   parameter;
public:
    tOutputItemTemplate(tOutput& o, int n, const char *p);
    virtual void Print(tString& target) const;
    virtual void Clone(tOutput& o)      const;
};


tOutput::tOutput(): anchor(0){}
tOutput::~tOutput()
{
    while (anchor)
        delete anchor;
}

void tOutput::Clear()
{
    while (anchor)
        delete anchor;
}


static void getstring(tString &target, tOutputItemBase *item)
{
    if (!item)
        return;
    getstring(target, item->Next());
    item->Print(target);
}

/*
tOutput::operator tString() const
{
#ifdef DEBUG
#ifndef WIN32
	static bool recursion = false;
	if (recursion)
    {
		tERR_ERROR_INT("Locale Recursion!");
    }
	recursion = true;
#endif
#endif

	static tString x;
	x.Clear();
	getstring(x, anchor);

#ifdef DEBUG
#ifndef WIN32
	recursion = false;
#endif
#endif

	return x;
}
*/

tOutput::operator const char *() const
{
#ifdef DEBUG
#ifndef WIN32
    static bool recursion = false;
    if (recursion)
    {
        tERR_ERROR_INT("Locale Recursion!");
    }
    recursion = true;
#endif
#endif

    // get a relatively safe buffer to write to
    static const int maxstrings = 5;
    static int current = 0;
    static tString buffers[maxstrings];
    tString & x = buffers[current];
    current = ( current + 1 ) % maxstrings;

    x.Clear();
    getstring(x, anchor);

#ifdef DEBUG
#ifndef WIN32
    recursion = false;
#endif
#endif

    return x;
}


void tOutput::AddLiteral(const char *x)
{
    tNEW(tOutputItem<tString>)(*this, tString(x));
}

void tOutput::AddSpace()
{
    tNEW(tOutputItemSpace)(*this);
}

void tOutput::AddLocale(const char *x)
{
    tNEW(tOutputItemLocale)(*this, tLocale::Find(x));
}


tOutput & tOutput::SetTemplateParameter(int num, const char *parameter)
{
    tNEW(tOutputItemTemplate)(*this, num, parameter);
    return *this;
}

tOutput &  tOutput::SetTemplateParameter(int num, int parameter)
{
    tString p;
    p << parameter;
    tNEW(tOutputItemTemplate)(*this, num, p);

    return *this;
}

tOutput & tOutput::SetTemplateParameter(int num, float parameter)
{
    tString p;
    p << parameter;
    tNEW(tOutputItemTemplate)(*this, num, p);

    return *this;
}


tOutput::tOutput(const tString& x)
        :anchor(NULL)
{
    tNEW(tOutputItem<tString>)(*this, x);
}

tOutput::tOutput(const char * x)
        :anchor(NULL)
{
    *this << x;
}


tOutput::tOutput(const tLocaleItem &locale)
        :anchor(NULL)
{
    tNEW(tOutputItemLocale)(*this, locale);
}


static void copyrec(tOutput &targ, tOutputItemBase *it)
{
    if (!it)
        return;

    copyrec(targ, it->Next());
    it->Clone(targ);
}

tOutput::tOutput(const tOutput &o)
        :anchor(NULL)
{
    copyrec(*this, o.anchor);
}

tOutput& tOutput::operator=(const tOutput &o)
{
    Clear();

    copyrec(*this, o.anchor);

    return *this;
}

void tOutput::Append(const tOutput &o)
{
    copyrec(*this, o.anchor);
}


tOutputItemBase::tOutputItemBase(tOutput& o): tListItem<tOutputItemBase>(o.anchor){}

tOutputItemBase::~tOutputItemBase(){}



tOutputItemLocale::tOutputItemLocale(tOutput& o, const tLocaleItem& e)
        :tOutputItemBase(o),
        element(&e)
{
}

void tOutputItemLocale::Print(tString& target) const
{
    target << *element;
}

void tOutputItemLocale::Clone(tOutput& o)      const
{
    tNEW(tOutputItemLocale)(o, *element);
}


tOutputItemSpace::tOutputItemSpace(tOutput& o)
        :tOutputItemBase(o)
{}

void tOutputItemSpace::Print(tString& target) const
{
    target << ' ';
}

void tOutputItemSpace::Clone(tOutput& o)      const
{
    tNEW(tOutputItemSpace)(o);
}


tOutputItemTemplate::tOutputItemTemplate(tOutput& o, int n, const char *p)
        :tOutputItemBase(o), num(n), parameter(p)
{}

void tOutputItemTemplate::Print(tString& target) const
{
    st_TemplateParameters[num] = parameter;
}

void tOutputItemTemplate::Clone(tOutput& o)      const
{
    tNEW(tOutputItemTemplate)(o, num, parameter);
}


tOutput& operator << (tOutput &o, char *locale)
{
    return operator<<(o, static_cast<const char *>(locale));
}

// and a special implementation for the locales and strings:
tOutput& operator << (tOutput &o, const char *locale){
    return o.AddString(locale);
}

tOutput & tOutput::AddString(char const * locale)
{
    tOutput & o = *this;

    int len = strlen(locale);
    if (len == 0)
        return o;
    if (len == 1 && locale[0] == ' ')
        tNEW(tOutputItemSpace)(o);
    else if (locale[0] == '$')
        tNEW(tOutputItemLocale)(o, tLocale::Find(locale+1));
    else
        tNEW(tOutputItem<tString>)(o, tString(locale));

    return o;
}

// output operators
std::ostream& operator<< (std::ostream& s, const tOutput& o)
{
    return s << tString(o);
}

/*
std::stringstream& operator<< (std::stringstream& s, const tOutput& o)
{
	static_cast<std::ostream&>(s) << static_cast<const char *>(o);
	return s;
}
*/

tString& operator<< (tString& s, const tOutput& o)
{
    s << tString(o);
    return s;
}



// interface class for locale items
const tLocaleItem& tLocale::Find(const char* id)
{
    return tLocaleItem::Find(id);
}

void               tLocale::Load(const char* filename)
{
    tLocaleItem::Load(filename, false);

    // determine the name of the game
    s_gameName.Clear();
    s_gameName << tOutput("$game_name");
}

void               tLocale::Clear()
{
    tLocaleItem::Clear();
}

/*
std::stringstream& operator<<(std::stringstream& s, const tLocaleItem &t)
{
	static_cast<std::ostream&>(s) << static_cast<const char*>(t);
	return s;
}
*/

std::ostream& operator<<(std::ostream& s, const tLocaleItem &t)
{
    return s << static_cast<const char*>(t);
}

tString& operator<< (tString& s, const tLocaleItem& o)
{
    s << static_cast<const char *>(o);
    return s;
}
