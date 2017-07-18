// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

#include "defines.h" // IWYU pragma: keep
#include "LuaInterfaceBase.h"
#include "GlobalVars.h"
#include "GameClient.h"
#include "WindowManager.h"
#include "ingameWindows/iwMsgbox.h"
#include "libutil/src/Log.h"
#include "mygettext/src/mygettext.h"
#include <utility>
#include <fstream>

namespace kaguya{
    template<typename T1, typename T2>
    struct lua_type_traits< std::pair<T1, T2> >
    {
        static int push(lua_State* l, const std::pair<T1, T2>& v)
        {
            int count = 0;
            count += lua_type_traits<T1>::push(l, v.first);
            count += lua_type_traits<T2>::push(l, v.second);
            return count;
        }
    };
}

unsigned LuaInterfaceBase::GetVersion()
{
    return 1;
}

unsigned LuaInterfaceBase::GetFeatureLevel()
{
    return 1;
}

LuaInterfaceBase::LuaInterfaceBase(): lua(kaguya::NoLoadLib())
{
    lua.openlib("base", luaopen_base);
    lua.openlib("package", luaopen_package);
    lua.openlib("string", luaopen_string);
    lua.openlib("table", luaopen_table);
    lua.openlib("math", luaopen_math);

    Register(lua);
}

LuaInterfaceBase::~LuaInterfaceBase()
{}

void LuaInterfaceBase::Register(kaguya::State& state)
{
    state["RTTRBase"].setClass(kaguya::UserdataMetatable<LuaInterfaceBase>()
        .addStaticFunction("GetFeatureLevel", &LuaInterfaceBase::GetFeatureLevel)
        .addFunction("Log", &LuaInterfaceBase::Log)
        .addFunction("IsHost", &LuaInterfaceBase::IsHost)
        .addFunction("GetLocalPlayerIdx", &LuaInterfaceBase::GetLocalPlayerIdx)
        .addOverloadedFunctions("MsgBox", &LuaInterfaceBase::MsgBox, &LuaInterfaceBase::MsgBox2)
        .addOverloadedFunctions("MsgBoxEx", &LuaInterfaceBase::MsgBoxEx, &LuaInterfaceBase::MsgBoxEx2)
        );
    state.setErrorHandler(ErrorHandler);
}

void LuaInterfaceBase::ErrorHandler(int status, const char* message)
{
    LOG.write(_("Lua error: %s\n")) % message;
    if(GLOBALVARS.isTest)
    {
        GLOBALVARS.errorOccured = true;
        throw std::runtime_error(message);
    }
}

void LuaInterfaceBase::ErrorHandlerThrow(int status, const char* message)
{
    throw std::runtime_error(message);
}

bool LuaInterfaceBase::LoadScript(const std::string& scriptPath)
{
    if(!lua.dofile(scriptPath))
    {
        script_.clear();
        return false;
    } else
    {
        std::ifstream scriptFile(scriptPath.c_str());
        script_.assign(std::istreambuf_iterator<char>(scriptFile), std::istreambuf_iterator<char>());
        return true;
    }
}

bool LuaInterfaceBase::LoadScriptString(const std::string& script)
{
    if(!lua.dostring(script))
    {
        script_.clear();
        return false;
    } else
    {
        script_ = script;
        return true;
    }
}

bool LuaInterfaceBase::CheckScriptVersion()
{
    kaguya::LuaRef func = lua["getRequiredLuaVersion"];
    if(func.type() == LUA_TFUNCTION)
    {
        const unsigned scriptVersion = func.call<unsigned>();
        if(scriptVersion == GetVersion())
            return true;
        else
        {
            LOG.write(_("Wrong lua script version: %1%. Current version: %2%.%3%.")) % scriptVersion % GetVersion() % GetFeatureLevel();
            return false;
        }
    } else
    {
        LOG.write(_("Lua script did not provide the function getRequiredLuaVersion()! It is probably outdated."));
        return false;
    }
}

bool LuaInterfaceBase::IsHost() const
{
    return GAMECLIENT.IsHost();
}

unsigned LuaInterfaceBase::GetLocalPlayerIdx() const
{
    return GAMECLIENT.GetPlayerId();
}

void LuaInterfaceBase::MsgBox(const std::string& title, const std::string& msg, bool isError)
{
    WINDOWMANAGER.Show(new iwMsgbox(_(title), _(msg), NULL, MSB_OK, isError ? MSB_EXCLAMATIONRED : MSB_EXCLAMATIONGREEN));
}

void LuaInterfaceBase::MsgBoxEx(const std::string& title, const std::string& msg, const std::string& iconFile, unsigned iconIdx)
{
    WINDOWMANAGER.Show(new iwMsgbox(_(title), _(msg), NULL, MSB_OK, iconFile, iconIdx));
}

void LuaInterfaceBase::MsgBoxEx2(const std::string& title, const std::string& msg, const std::string& iconFile, unsigned iconIdx, int iconX, int iconY)
{
    iwMsgbox* msgBox = new iwMsgbox(_(title), _(msg), NULL, MSB_OK, iconFile, iconIdx);
    msgBox->MoveIcon(DrawPoint(iconX, iconY));
    WINDOWMANAGER.Show(msgBox);
}

void LuaInterfaceBase::Log(const std::string& msg)
{
    LOG.write("%s\n") % msg;
}
