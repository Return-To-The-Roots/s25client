// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "LuaInterfaceGameBase.h"
#include "WindowManager.h"
#include "ingameWindows/iwMsgbox.h"
#include "mygettext/mygettext.h"
#include "resources/ResourceId.h"
#include "s25util/Log.h"

unsigned LuaInterfaceGameBase::getVersion()
{
    return 1;
}

unsigned LuaInterfaceGameBase::getFeatureLevel()
{
    return 8;
}

LuaInterfaceGameBase::LuaInterfaceGameBase(const ILocalGameState& localGameState) : localGameState(localGameState)
{
    Register(lua);
}

void LuaInterfaceGameBase::Register(kaguya::State& state)
{
    state["RTTRGameBase"].setClass(
      kaguya::UserdataMetatable<LuaInterfaceGameBase, LuaInterfaceBase>()
        .addStaticFunction("GetFeatureLevel", &LuaInterfaceGameBase::getFeatureLevel)
        .addFunction("IsHost", &LuaInterfaceGameBase::isHost)
        .addFunction("GetLocalPlayerIdx", &LuaInterfaceGameBase::getLocalPlayerIdx)
        .addOverloadedFunctions("MsgBox", &LuaInterfaceGameBase::MsgBox, &LuaInterfaceGameBase::MsgBox2)
        .addOverloadedFunctions("MsgBoxEx", &LuaInterfaceGameBase::MsgBoxEx, &LuaInterfaceGameBase::MsgBoxEx2));
}

bool LuaInterfaceGameBase::CheckScriptVersion()
{
    kaguya::LuaRef func = lua["getRequiredLuaVersion"];
    if(func.type() == LUA_TFUNCTION)
    {
        const auto scriptVersion = func.call<unsigned>();
        if(scriptVersion == getVersion())
            return true;
        else
        {
            LOG.write(_("Wrong lua script version: %1%. Current version: %2%.%3%.\n")) % scriptVersion % getVersion()
              % getFeatureLevel();
            return false;
        }
    } else
    {
        LOG.write(_("Lua script did not provide the function getRequiredLuaVersion()! It is probably outdated.\n"));
        return false;
    }
}

bool LuaInterfaceGameBase::isHost() const
{
    return localGameState.isHost();
}

unsigned LuaInterfaceGameBase::getLocalPlayerIdx() const
{
    return localGameState.getPlayerId();
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
