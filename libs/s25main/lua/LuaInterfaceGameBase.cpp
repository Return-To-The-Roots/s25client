// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "LuaInterfaceGameBase.h"
#include "WindowManager.h"
#include "ingameWindows/iwMsgbox.h"
#include "mygettext/mygettext.h"
#include "resources/ResourceId.h"
#include "s25util/Log.h"

unsigned LuaInterfaceGameBase::GetVersion()
{
    return 1;
}

unsigned LuaInterfaceGameBase::GetFeatureLevel()
{
    return 3;
}

LuaInterfaceGameBase::LuaInterfaceGameBase(const ILocalGameState& localGameState) : localGameState(localGameState)
{
    Register(lua);
}

void LuaInterfaceGameBase::Register(kaguya::State& state)
{
    state["RTTRGameBase"].setClass(
      kaguya::UserdataMetatable<LuaInterfaceGameBase, LuaInterfaceBase>()
        .addStaticFunction("GetFeatureLevel", &LuaInterfaceGameBase::GetFeatureLevel)
        .addFunction("IsHost", &LuaInterfaceGameBase::IsHost)
        .addFunction("GetLocalPlayerIdx", &LuaInterfaceGameBase::GetLocalPlayerIdx)
        .addOverloadedFunctions("MsgBox", &LuaInterfaceGameBase::MsgBox, &LuaInterfaceGameBase::MsgBox2)
        .addOverloadedFunctions("MsgBoxEx", &LuaInterfaceGameBase::MsgBoxEx, &LuaInterfaceGameBase::MsgBoxEx2));
}

bool LuaInterfaceGameBase::CheckScriptVersion()
{
    kaguya::LuaRef func = lua["getRequiredLuaVersion"];
    if(func.type() == LUA_TFUNCTION)
    {
        const auto scriptVersion = func.call<unsigned>();
        if(scriptVersion == GetVersion())
            return true;
        else
        {
            LOG.write(_("Wrong lua script version: %1%. Current version: %2%.%3%.\n")) % scriptVersion % GetVersion()
              % GetFeatureLevel();
            return false;
        }
    } else
    {
        LOG.write(_("Lua script did not provide the function getRequiredLuaVersion()! It is probably outdated.\n"));
        return false;
    }
}

bool LuaInterfaceGameBase::IsHost() const
{
    return localGameState.IsHost();
}

unsigned LuaInterfaceGameBase::GetLocalPlayerIdx() const
{
    return localGameState.GetPlayerId();
}

void LuaInterfaceGameBase::MsgBox(const std::string& title, const std::string& msg, bool isError)
{
    WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(_(title), _(msg), nullptr, MsgboxButton::Ok,
                                                  isError ? MsgboxIcon::ExclamationRed : MsgboxIcon::ExclamationGreen));
}

void LuaInterfaceGameBase::MsgBoxEx(const std::string& title, const std::string& msg, const std::string& iconFile,
                                    unsigned iconIdx)
{
    WINDOWMANAGER.Show(
      std::make_unique<iwMsgbox>(_(title), _(msg), nullptr, MsgboxButton::Ok, ResourceId::make(iconFile), iconIdx));
}

void LuaInterfaceGameBase::MsgBoxEx2(const std::string& title, const std::string& msg, const std::string& iconFile,
                                     unsigned iconIdx, int iconX, int iconY)
{
    auto msgBox =
      std::make_unique<iwMsgbox>(_(title), _(msg), nullptr, MsgboxButton::Ok, ResourceId::make(iconFile), iconIdx);
    msgBox->MoveIcon(DrawPoint(iconX, iconY));
    WINDOWMANAGER.Show(std::move(msgBox));
}
