#ifndef _TSCRIPTING_H_
#define _TSCRIPTING_H_

#include "aa_config.h"

#ifdef ENABLE_SCRIPTING

#ifdef WITH_RUBY
#include <ruby.h>
#include <intern.h>

#elif defined(WITH_LUA)
extern "C" {
#include "lua.h"
}

#elif defined(WITH_PHP)

#elif defined(WITH_PYTHON)
extern "C" {
#include <Python.h>
}
#endif

#include <stdexcept>
#include <string>
#include "tDirectories.h"
#include "tConfiguration.h"

/*#############################################################
  # GENERIC SCRIPTING INTERFACE
  ###########################################################*/

// a class representing lists of arguments to be passed to script procedures
class args;

// main scripting interface (singleton)
template <typename T, typename DataType, typename ProcType>
class tScriptingBase {
public:
    // type representing data of any type
    typedef DataType data_type;
    // type representing a proc
    typedef ProcType proc_type;

    virtual ~tScriptingBase() { };
    // Init scripting, load required modules, etc.
    virtual void InitializeInterpreter() = 0;
    // Stop (and clean) scripting
    virtual void CleanupInterpreter() = 0;
    // load (and exec) a file
    virtual void Load(const tPath & path, const char * filename, bool w=false) = 0;
    // Eval (and exec) a string
    virtual data_type Eval(std::string code) = 0;
    // get a internal reference to an already defined proc (for callbacks)
    // if proc is empty, check for an unnamed function or a block
    virtual proc_type GetProcRef(std::string proc) = 0;
    // call an already defined proc (for callbacks)
    virtual void Exec(proc_type proc, args *data) = 0;

    static T &GetInstance() {
	static T s_instance;
        return s_instance;
    };
protected:
  tScriptingBase() { };
private:
  tScriptingBase(tScriptingBase const&);
  tScriptingBase& operator=(tScriptingBase const&);
};


/*#############################################################
  # RUBY SPECIFIC
  ###########################################################*/

#ifdef WITH_RUBY
class tScripting: public tScriptingBase<tScripting, VALUE, VALUE> {
    void CheckStatus(int status);
    std::string GetExceptionInfo();
    static tScripting::data_type ExecProtect(tScripting::data_type value);
    static tScripting::data_type LoadProtect(tScripting::data_type value);
    static tScripting::data_type EvalProtect(tScripting::data_type value);
public:
    tScripting();
    virtual void InitializeInterpreter();
    virtual void CleanupInterpreter();
    virtual void Load(const tPath & path, const char * filename, bool w=false);
    virtual data_type Eval(std::string code);
    virtual proc_type GetProcRef(std::string proc);
    virtual void Exec(proc_type proc, args *data);
};

/*#############################################################
  # LUA SPECIFIC
  ###########################################################*/

#elif defined(WITH_LUA)
class tScripting: public tScriptingBase<tScripting, lua_State*> {
public:
    tScripting();
    virtual void InitializeInterpreter();
    virtual void CleanupInterpreter();
    virtual void Load(const tPath & path, const char * filename, bool w=false);
    virtual data_type Eval(std::string code);
    virtual proc_type GetProcRef(std::string proc);
    virtual void Exec(proc_type proc, args *data);
};

/*#############################################################
  # PHP SPECIFIC
  ###########################################################*/

#elif defined(WITH_PHP)
class tScripting: public tScriptingBase<tScripting> {
public:
    tScripting();
    virtual void InitializeInterpreter();
    virtual void CleanupInterpreter();
    virtual void Load(const tPath & path, const char * filename, bool w=false);
    virtual data_type Eval(std::string code);
    virtual proc_type GetProcRef(std::string proc);
    virtual void Exec(proc_type proc, args *data);
};

/*#############################################################
  # PYTHON SPECIFIC
  ###########################################################*/

#elif defined(WITH_PYTHON)
class tScripting: public tScriptingBase<tScripting, PyObject*, PyObject*> {
    PyObject *main_module, *main_dict;
public:
    tScripting();
    virtual void InitializeInterpreter();
    virtual void CleanupInterpreter();
    virtual void Load(const tPath & path, const char * filename, bool w=false);
    virtual data_type Eval(std::string code);
    virtual proc_type GetProcRef(std::string proc);
    virtual void Exec(proc_type proc, args *data);
};

#endif

/*#############################################################
  # LANGUAGE INDEPENDANT STUFF
  ###########################################################*/

// A class representing lists of arguments to be passed to script procedures
class args {
    tScripting::data_type data;
public:
    args();
    ~args();
    void clear();
    tScripting::data_type get() { return data; }
    args &operator<< (args);
    args &operator<< (tScripting::data_type);
    args &operator<< (int);
    args &operator<< (double);
    args &operator<< (tOutput const &x);
    args &operator<< (tString &x);
};

// scripting configure item
class tConfItemScript:public tConfItemBase{
    tScripting::proc_type callback;
public:
    tConfItemScript(const char *title, tScripting::proc_type proc):tConfItemBase(title),callback(proc){}
    virtual ~tConfItemScript() {}

    virtual void ReadVal(std::istream &s)
    {   tString t;
        t.ReadLine(s);
        args tmp;
        tmp << t;
        tScripting::GetInstance().Exec(callback, &tmp);
    }
    virtual void WriteVal(std::ostream &s) {}

    virtual bool Save() {return false;}
};

class tCallbackScripting : public tListItem<tCallbackScripting> {
    tScripting::proc_type block;
//protected:
//    static tScripting::value_type ExecProtect(tScripting::value_type);
public:
    tCallbackScripting(tCallbackScripting *& anchor);
    static void Exec(tCallbackScripting *anchor);
};


#endif // ENABLE_SCRIPTING

#endif // ! _TSCRIPTING_H_
