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

#include "tLuaScript.h"
#include "tConfiguration.h"
#include "tResourceManager.h"
#include "tDirectories.h"

int LuaLoadLine(lua_State* L) {
    if (lua_gettop(L) != 1 || !lua_isstring(L, -1)) {
        luaL_where(L, 1);
        lua_pushstring(L, "RUNTIME ERROR: This function requires exactly 1 string parameter!");
        return lua_error(L);
    }
    std::stringstream ss;
    ss << lua_tostring(L,-1);
    tConfItemBase::LoadLine(ss);
    return 0;
}



void LuaStateHandler::PrintError() const {
    const char * msg = lua_tostring(m_L, -1);
    if (msg) std::cerr << "LUA ERROR: <<" << msg << ">>" << std::endl;
    else std::cerr << "LUA ERROR!" << std::endl;
}

void LuaStateHandler::Run(const std::string & p_script) const {
    StackCleaner sc(m_L);
    if (luaL_dostring(m_L, p_script.c_str())) {
        PrintError();
    }
}

void LuaStateHandler::Load(const std::string & p_filename) const {
    StackCleaner sc(m_L);
    std::string fn = p_filename;
    if (luaL_dofile(m_L, fn.c_str())) {
        PrintError();
    }
}

void LuaStateHandler::LoadRemote(const std::string & p_filename) const {
    StackCleaner sc(m_L);
    tString localFile = tResourceManager::locateResource(NULL, p_filename.c_str());
    if ( localFile ) {
        // remote lua script MUST be sandboxed. We can't use default load
        // Load(std::string(localFile));
        if (luaL_loadfile(m_L, localFile)) {
            PrintError();
        } else {
            LuaState::Instance.SandboxedExec(0,0);
/*
            lua_rawgeti(m_L, LUA_REGISTRYINDEX, LuaState::Instance.m_sandbox_env_ref);
            lua_setfenv(m_L, -2);
            if (lua_pcall(m_L, 0, 0, 0)) {
                PrintError();
            }
*/
        }
    } else {
        con << tOutput( "$lua_remote_file_not_found", p_filename.c_str());
    }
}

LuaState LuaState::Instance;

const LuaStateHandler LuaState::New() {
    const LuaStateHandler LS(luaL_newstate());
    if (!LS) {
        std::cerr << "LuaState creation failed!";
        throw;
    }
    luaL_openlibs(LS);

#ifdef __LUAJIT__
    luaJIT_setmode(m_state, 0, LUAJIT_MODE_ENGINE | LUAJIT_MODE_ON);
#endif

    // bind functions to lua
    LS.SetGlobal("loadline",&LuaLoadLine);
    LS.SetGlobal("add_ladderlog_event",&LuaAddLadderLogWriterBinding);

    return LS;
}

void LuaState::CreateSandbox() {
    StackCleaner sc(m_LS);
    // try to run create_sandbox script to build sandbox environnement table.
    std::ifstream s;
    static const tString file("create_sandbox.lua");
    if (tDirectories::Config().Open(s, file)) {
        if (luaL_loadstring(m_LS, std::string((std::istreambuf_iterator<char>(s)),
                                          std::istreambuf_iterator<char>()).c_str())) {
            m_LS.PrintError();
        } else {
            if (lua_pcall( m_LS, 0, 1, 0 )) {
                m_LS.PrintError();
            } else if (!lua_istable(m_LS, -1)) {
                lua_pop(m_LS, 1);
                lua_newtable(m_LS);
            }
        }
    } else { // otherwise, create the most resticted sandbox ever: empty table
        lua_newtable(m_LS);
    }
    // store this table into a new reference and keep it safe
    m_sandbox_env_ref = luaL_ref(m_LS, LUA_REGISTRYINDEX);
}

static tString st_luaGameScript("");
static tSettingItem<tString> st_luaGameScriptConf( "LUA_GAME_SCRIPT", st_luaGameScript );

void LuaState::Restart() {
    LuaState::Instance->ForceGC();
    lua_close(LuaState::Instance);
    LuaState::Instance.m_LS=New();
    LuaState::Instance.CreateSandbox();

    // try to load everytime.lua
    std::ifstream s;
    static const tString everytime("everytime.lua");
    if ( tDirectories::Config().Open(s, everytime)
         || tDirectories::Var().Open(s, everytime) ) {
    LuaState::Instance->Run(std::string((std::istreambuf_iterator<char>(s)),
                                        std::istreambuf_iterator<char>()));
    }
    s.close();
    // try to load lua_game_script file
    if (st_luaGameScript=="") return;
    if ( tDirectories::Config().Open(s, st_luaGameScript)
         || tDirectories::Var().Open(s, st_luaGameScript) ) {
    LuaState::Instance->Run(std::string((std::istreambuf_iterator<char>(s)),
                                        std::istreambuf_iterator<char>()));
    }
    s.close();
}

void LuaState::SandboxedExec(int nargs, int nresults) {
    lua_rawgeti(Instance, LUA_REGISTRYINDEX, Instance.m_sandbox_env_ref);
    lua_setfenv(Instance, -2-nargs);
    if (lua_pcall(Instance, nargs, nresults, 0)) {
        Instance->PrintError();
    }
}
