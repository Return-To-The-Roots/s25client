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
#include <fstream>

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

LuaInterfaceBase::LuaInterfaceBase(): lua(kaguya::NoLoadLib())
{
    luaopen_base(lua.state());
    luaopen_package(lua.state());
    luaopen_string(lua.state());
    luaopen_table(lua.state());
    luaopen_math(lua.state());

    Register(lua);
}

LuaInterfaceBase::~LuaInterfaceBase()
{}

void LuaInterfaceBase::Register(kaguya::State& state)
{
    state["RTTRBase"].setClass(kaguya::ClassMetatable<LuaInterfaceBase>()
        .addMemberFunction("Log", &LuaInterfaceBase::Log)
        .addMemberFunction("IsHost", &LuaInterfaceBase::IsHost)
        .addMemberFunction("GetLocalPlayerIdx", &LuaInterfaceBase::GetLocalPlayerIdx)
        .addMemberFunction("MsgBox", &LuaInterfaceBase::MsgBox)
        .addMemberFunction("MsgBox", &LuaInterfaceBase::MsgBox2)
        .addMemberFunction("MsgBoxEx", &LuaInterfaceBase::MsgBoxEx)
        .addMemberFunction("MsgBoxEx", &LuaInterfaceBase::MsgBoxEx2)
        );
    state.setErrorHandler(ErrorHandler);
}

void LuaInterfaceBase::ErrorHandler(int status, const char* message)
{
    LOG.lprintf("Lua error: %s\n", message);
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
        if(GLOBALVARS.isTest)
            throw std::runtime_error("Could not load lua script");
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
        if(GLOBALVARS.isTest)
            throw std::runtime_error("Could not load lua script");
        return false;
    } else
    {
        script_ = script;
        return true;
    }
}

bool LuaInterfaceBase::IsHost() const
{
    return GAMECLIENT.IsHost();
}

unsigned LuaInterfaceBase::GetLocalPlayerIdx() const
{
    return GAMECLIENT.GetPlayerID();
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
    msgBox->MoveIcon(iconX, iconY);
    WINDOWMANAGER.Show(msgBox);
}

void LuaInterfaceBase::Log(const std::string& msg)
{
    LOG.lprintf("%s\n", msg.c_str());
}
