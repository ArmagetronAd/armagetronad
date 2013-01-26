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

#ifndef ArmageTron_LUASCRIPT_H
#define ArmageTron_LUASCRIPT_H

#ifdef __LUAJIT__
    #include <luajit-2.0/lua.hpp>
#elif defined(__ALT_LUA_PATH__)
extern "C"
{
    #include <lua5.1/lua.h>
    #include <lua5.1/lauxlib.h>
    #include <lua5.1/lualib.h>
}
#else
extern "C"
{
    #include <lua.h>
    #include <lauxlib.h>
    #include <lualib.h>
}
#endif

#include <iostream>
#include <string>
#include <stdexcept>
#include <boost/type_traits/is_integral.hpp>
#include <boost/utility/enable_if.hpp>

#include "tConfiguration.h"
#include "tLocale.h"
#include "tString.h"

// Some convinient macros to handle exceptions
#define LUA_EXCEPTIONS_TRY try

#define LUA_EXCEPTIONS_CATCH_BAD_ALLOC_TO_LUA \
catch (const std::bad_alloc &) { PushData(L, (const char*)"Allocation error!"); }

#define LUA_EXCEPTIONS_CATCH_COMMON_TO_LUA \
catch (std::string & Msg) { PushData(L, Msg); } \
catch (std::runtime_error& Err) { PushData(L, Err.what()); } \
catch (...) { PushData(L,(const char *)"Unhandled error!"); } \
return lua_error(L);

#define LUA_EXCEPTIONS_CATCH_COMMON_TO_CPP \
catch (std::string& Msg) { std::cerr << Msg << std::endl; throw; } \
catch (std::runtime_error& Err) { std::cerr << Err.what() << std::endl; throw; } \
catch (...) { std::cerr << "Unhandled error!" << std::endl; throw; }


// Extract from lua source code (file lauxlib.c), a convinient macro
// to convert a stack index to positive.
#define abs_index(L, i) ((i) > 0 || (i) <= LUA_REGISTRYINDEX ? (i) : lua_gettop(L) + (i) + 1)


//  A simple class to keep stack clean
class StackCleaner {
    lua_State * const m_L;
    int m_top; // index to future top value
public:
    // constructors and destructor should be enough to ensure the stack will be
    // restored to a clean state when leaving scope.
    StackCleaner(lua_State *p_L): m_L(p_L), m_top(lua_gettop(p_L)) {}
    ~StackCleaner() { lua_settop(m_L, m_top); }
};


//  Templates Data:
//     A set of templates to get/push data from/to lua stack
//     They can be specialized by users in order to handle user types

//  Default template definition with Get/Push prototype only.
template<typename S, typename Enable = void> struct Data {
    inline static S Get(lua_State* L, int index);
    inline static void Push(lua_State* L, S val);
};

//  Specialization for bool
template<> struct Data<bool> {
    inline static bool Get(lua_State* L, int index) {
#ifdef CHECK_TYPES_FROM_LUA
        CheckType(L,index,lua_isboolean(L, index),"Boolean");
#endif
        return lua_toboolean(L, index);
    }
    inline static void Push(lua_State* L, bool val) {lua_pushboolean(L, val);}
};

//  Specialization for integer
template<> struct Data<lua_Integer> {
    inline static lua_Integer Get(lua_State* L, int index) {
#ifdef CHECK_TYPES_FROM_LUA
        CheckType(L,index,lua_isnumber(L, index),"Integer");
#endif
        return lua_tointeger(L, index);
    }
    inline static void Push(lua_State* L, lua_Integer val) {lua_pushinteger(L, val);}
};

//  Specialization for numbers (floating point)
template<> struct Data<lua_Number> {
    inline static lua_Number Get(lua_State* L, int index) {
#ifdef CHECK_TYPES_FROM_LUA
        CheckType(L,index,lua_isnumber(L, index),"Number");
#endif
        return lua_tonumber(L, index);
    }
    inline static void Push(lua_State* L, lua_Number val) {lua_pushnumber(L, val);}
};

//  Specialization for string (null terminated)
template<> struct Data<const char *> {
    inline static const char * Get(lua_State* L, int index) {
#ifdef CHECK_TYPES_FROM_LUA
        CheckType(L,index,lua_isstring(L, index),"String");
#endif
        return lua_tostring(L, index);
    }
    inline static void Push(lua_State* L, const char * val) {lua_pushstring(L, val);}
};

//  Specialization for string
template<> struct Data<std::string> {
    inline static std::string Get(lua_State* L, int index) {
#ifdef CHECK_TYPES_FROM_LUA
        CheckType(L,index,lua_isstring(L, index),"String");
#endif
        return std::string(lua_tostring(L,index),lua_strlen(L,index));
    }
    inline static void Push(lua_State* L, std::string val) {lua_pushlstring(L,val.data(),val.size());}
};

//  Extra specialization for char
template<> struct Data<char> {
    inline static const char Get(lua_State* L, int index) {
#ifdef CHECK_TYPES_FROM_LUA
        CheckType(L,index,lua_isstring(L, index),"String");
#endif
        return lua_tostring(L, index)[0];
    }
    inline static void Push(lua_State* L, const char val) {lua_pushlstring(L, &val, 1);}
};

//  Specialization for lua functions
template<> struct Data<lua_CFunction> {
    inline static lua_CFunction Get(lua_State* L, int index) {
#ifdef CHECK_TYPES_FROM_LUA
        CheckType(L,index,lua_iscfunction(L, index) ,"lua_CFunction");
#endif
        return lua_tocfunction(L, index);
    }
    inline static void Push(lua_State* L, lua_CFunction val) {lua_pushcfunction(L, val);}
};

//  Template LuaTypeMap, to bind a C++ type to a lua type (ie a Data Template).

// An helper class to define typemaps for types convertible via simple static cast
template<typename Cpp_t, typename Sel_t = Cpp_t> struct TypeMapHelper {
    typedef Cpp_t cpp_type;
    typedef Sel_t select_type;
};

//  LuaTypeMap is intended to be specialized by users to fit their needs.
//  It musts provide appropriate conversion routines between C++ and lua types

template<typename T, typename Enable = void> struct TypeMap: public TypeMapHelper<T>{};

// TypeMap specialization for any integer variant types
template<typename T> struct is_integer: boost::is_integral<T> {};
template<> struct is_integer<bool>: boost::false_type {};

template<typename T> struct TypeMap<T, typename boost::enable_if<is_integer<T> > >:
public TypeMapHelper<T, lua_Integer> {};

// Specializations for other basic C++ types
template<> struct TypeMap<bool>:          public TypeMapHelper<bool>{};
template<> struct TypeMap<double>:        public TypeMapHelper<double, lua_Number>{};
template<> struct TypeMap<float>:         public TypeMapHelper<float, lua_Number>{};
template<> struct TypeMap<const char *>:  public TypeMapHelper<const char *>{};
template<> struct TypeMap<std::string>:   public TypeMapHelper<std::string>{};
template<> struct TypeMap<lua_CFunction>: public TypeMapHelper<lua_CFunction>{};

template<> struct TypeMap<tString>:       public TypeMapHelper<const char*>{};
template<> struct TypeMap<tOutput>:       public TypeMapHelper<const char*>{};
template<> struct TypeMap<tAccessLevel>:  public TypeMapHelper<lua_Number>{};
template<> struct TypeMap<unsigned short>:public TypeMapHelper<lua_Number>{};

//  Main templates to get/push data

template<typename T>
inline static T GetData(lua_State* L, int index) {
    return Data<typename TypeMap<T>::select_type>::Get(L, index);
}

template<typename T>
inline static void PushData(lua_State* L, T const& val) {
    Data<typename TypeMap<T>::select_type>::Push(L, val);
//    Data<typename TypeMap<typename boost::remove_const<typename boost::remove_reference<T>::type>::type>::select_type>::Push(L, val);
}


// This class wrap a lua state and provide some shortcuts to lua functions,
// but does not owned the state and therefore, does not close the lua state in destructor.
class LuaStateHandler {
    lua_State* m_L;
public:
    LuaStateHandler(lua_State * p_L):m_L(p_L) {}
    ~LuaStateHandler() {}
    
    // Access to internal pointer
    operator lua_State * () const { return m_L; }
    
    // Print error message from stack
    void PrintError() const;
    
    // --------------------------------------------------------------------------------
    //  Running/Loading script
    // --------------------------------------------------------------------------------
    void Run(std::string const& p_script) const;
    void Load(std::string const& p_filename) const;
    void LoadRemote(std::string const& p_filename) const;
    
    // --------------------------------------------------------------------------------
    //  Garbage collection
    // --------------------------------------------------------------------------------
    void StopGC()    const { lua_gc(m_L, LUA_GCSTOP, 0); }
    void RestartGC() const { lua_gc(m_L, LUA_GCRESTART, 0); }
    void ForceGC()   const { lua_gc(m_L, LUA_GCCOLLECT, 0); }
    
    // --------------------------------------------------------------------------------
    //  Get and Set globals variables
    // --------------------------------------------------------------------------------
    
    // A getter with return value.
    template<typename T>
    T GetGlobal(const char * name) const {
        LUA_EXCEPTIONS_TRY {
            StackCleaner sc(m_L);
            lua_getglobal(m_L, name);
            return GetData<T>(m_L, -1);
        } LUA_EXCEPTIONS_CATCH_COMMON_TO_CPP
    }
    // A getter with reference so template type can be deduced from ref type.
    template<typename T>
    void GetGlobal(const char * name, T& ref) const {
        LUA_EXCEPTIONS_TRY {
            StackCleaner sc(m_L);
            lua_getglobal(m_L, name);
            ref = GetData<T>(m_L, -1);
        } LUA_EXCEPTIONS_CATCH_COMMON_TO_CPP
    }
    
    // A setter for all supported type (cf. LuaTypemap.hpp).
    // 1st case: lua_CFunction.
    void SetGlobal(const char * name, lua_CFunction f) const {
        LUA_EXCEPTIONS_TRY {
            PushData(m_L, f);
            lua_setglobal(m_L, name);
        } LUA_EXCEPTIONS_CATCH_COMMON_TO_CPP
    }
    // 2nd case: any other cases (any integers, floating points, strings, etc.)
    template<typename T>
    void SetGlobal(const char * name, T const& f) const {
        LUA_EXCEPTIONS_TRY {
            PushData(m_L, f);
            lua_setglobal(m_L, name);
        } LUA_EXCEPTIONS_CATCH_COMMON_TO_CPP
    }
};


// this second class is a RAII class owning a lua state using LuaStateHandler
class LuaState
{
    LuaStateHandler m_LS;
    int m_sandbox_env_ref;
 
    // Create a new lua_state and wrap it into a LuaStateHandler.
    static LuaStateHandler const New();

    // Create sandbox environnement table
    void CreateSandbox();

    // forbid copy and assignment
    LuaState(LuaState const& other):m_LS(other.m_LS) {};
    LuaState& operator=(LuaState const& other) { return *this;};

    friend class LuaStateHandler;
public:
    LuaState():m_LS(New()), m_sandbox_env_ref(LUA_GLOBALSINDEX) {CreateSandbox();}
    ~LuaState()  { lua_close(m_LS); }

    // Various method/operator to access lua state or his handler
    operator lua_State * () const { return m_LS; }
    operator LuaStateHandler const& () const { return m_LS; }
    LuaStateHandler const& operator* () const { return m_LS; }
    LuaStateHandler const* operator-> () const { return &m_LS; }    

    static LuaState Instance;                           // a dirty dirty global var to access our main state.
    static void Restart();                              // Restart our main state.
    static void SandboxedExec(int nargs, int nresults); // Execute function on top of the stack (func+parameters)
};

// LuaCFunction prototypes to add/remove LadderLogWriter binding
int LuaAddLadderLogWriterBinding(lua_State* L);


#endif // ArmageTron_LUASCRIPT_H

