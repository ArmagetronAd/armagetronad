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

#ifndef _SSCRIPTING_H_
#define _SSCRIPTING_H_

#include "aa_config.h"

#ifdef ENABLE_SCRIPTING

#include <stdexcept>
#include <string>
#include <boost/shared_ptr.hpp>
#include "tString.h"
#include "tLocale.h"
#include "tDirectories.h"
#include "tConfiguration.h"

/*#############################################################
  # GENERIC SCRIPTING INTERFACE
  ###########################################################*/

class sCallable;

// An abstract class representing data from scripting
class sData {
public:
    typedef boost::shared_ptr<sData> ptr;

    virtual ~sData() {}

// clear current data content
    virtual void                Clear() = 0;

// getters
    virtual int                 GetInt() const = 0;
    virtual bool                GetBool() const = 0;
    virtual REAL                GetReal() const = 0;
    virtual const char *        GetString() const = 0;

// setters
    virtual void Set(const int &i) = 0;
    virtual void Set(const bool &b) = 0;
    virtual void Set(const REAL &r) = 0;
    virtual void Set(const char* s) = 0;
    void Set(const tString &s) { this->Set(s.c_str()); }
    void Set(const tOutput &s) { this->Set((const char *)s); }

// checkers
    virtual const bool IsInt() = 0;
    virtual const bool IsBool() = 0;
    virtual const bool IsReal() = 0;
    virtual const bool IsString() = 0;

// copy and assignment
    virtual void Copy(const sData & source) = 0;
    sData & operator=(const sData & source) { Copy(source); return *this; } 

// comparisons (required to build args list)
    virtual bool Equal(sData const &a) const = 0;
    bool operator==(sData const & other) const { return this->Equal(other); }
};


// An abstract class representing a list of data (to be passed as parameters to a callable)
class sArgs {
public:
    typedef boost::shared_ptr<sArgs> ptr;

    virtual ~sArgs() {}

    virtual void Clear() = 0;      //<! Clear the argument list

    // add parameters for a future call
    virtual sArgs & operator<< (int const & i) = 0;
    virtual sArgs & operator<< (bool const & b) = 0;
    virtual sArgs & operator<< (REAL const & r) = 0;
    virtual sArgs & operator<< (tOutput const & x) = 0;
    virtual sArgs & operator<< (tString const & x) = 0;
    virtual sArgs & operator<< (sData const & d) = 0;
};

// An abstract class representing an executable object
class sCallable {
public:
    typedef boost::shared_ptr<sCallable> ptr;

    virtual ~sCallable() {}

    // add parameters for a future call
    virtual sCallable & operator<< (int const & i) = 0;
    virtual sCallable & operator<< (bool const & b) = 0;
    virtual sCallable & operator<< (REAL const & r) = 0;
    virtual sCallable & operator<< (tOutput const & x) = 0;
    virtual sCallable & operator<< (tString const & x) = 0;
    virtual sCallable & operator<< (sData const & d) = 0;

    virtual sCallable & operator<< (sArgs const & d) = 0;

    virtual void Set (const char * funcname) = 0; //<! Set this callable to script function called
    virtual void Set (const sData & object) = 0;  //<! Set this callable to this script object
    virtual void Call() = 0;                      //<! Execute
    virtual void Clear() = 0;                     //<! Clear the argument list

    template<typename T, typename... Args>
    void Call(const T& value, const Args&... args) {
        *this << value;
        if (sizeof...(args)) Call(args...);
        else Call();
    } 

};

// main scripting interface
class sScripting {
public:
    typedef boost::shared_ptr<sScripting> ptr;
    // this function must be defined in the language specific file 
    // in order to create the proper sScripting manager
    // and avoid the global initialization fiasco
    static sScripting * GetInstance();

    virtual ~sScripting() {}

    // Init scripting
    virtual void InitializeInterpreter() = 0;
    // Stop (and clean) scripting
    virtual void CleanupInterpreter() = 0;

    // load (and exec) a file
    virtual void Load(const tPath & p_path, const char * p_filename, bool p_w=false) = 0;
    // Eval (and exec) a string
    virtual sData::ptr Eval(const std::string & p_code) const = 0;

    // get a internal reference to an already defined proc (for callbacks)
    // if proc is empty, check for an unnamed function or a block
    virtual sCallable::ptr GetProcRef(const std::string & p_proc) const = 0;

    // virtual constructor idiom
    virtual sData::ptr CreateData() const = 0;
    virtual sArgs::ptr CreateArgs() const = 0;
};

/*#############################################################
  # GENERIC SCRIPTING TOOLS
  ###########################################################*/

// scripting configure item
class tConfItemScript:public tConfItemBase{
    sCallable::ptr callback;
public:
    tConfItemScript(const char *title, sCallable::ptr proc):tConfItemBase(title),callback(proc){}
    virtual ~tConfItemScript() {}

    virtual void ReadVal(std::istream &s)
    {   tString t;
        t.ReadLine(s);
        callback->Clear();
        (*callback) << t;
        callback->Call();
    }
    virtual void WriteVal(std::ostream &s) {}

    virtual bool Save() {return false;}
};

class tCallbackScripting : public tListItem<tCallbackScripting> {
    sCallable::ptr block;
//protected:
//    static sData ExecProtect(sData);
public:
    tCallbackScripting(tCallbackScripting *& anchor);
    static void Exec(tCallbackScripting *anchor);
};

#endif // ENABLE_SCRIPTING

#endif // ! _SSCRIPTING_H_
