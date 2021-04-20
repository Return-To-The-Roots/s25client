// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ILocalGameState.h"
#include "lua/LuaInterfaceBase.h"

class LuaInterfaceGameBase : public LuaInterfaceBase
{
public:
    static void Register(kaguya::State& state);

    bool CheckScriptVersion();

    /// Return version of the interface. Changes here reflect breaking changes
    static unsigned GetVersion();
    /// Get the feature level of this version. Reset on Version increase, increase for added features
    static unsigned GetFeatureLevel();

protected:
    explicit LuaInterfaceGameBase(const ILocalGameState& localGameState);

    /// Return true, if local player is the host
    bool IsHost() const;
    /// Return the index of the local player
    unsigned GetLocalPlayerIdx() const;
    /// Show a message box with given title and text.
    /// If isError is true, a red exclamation mark is shown, otherwise a green one is shown
    void MsgBox(const std::string& title, const std::string& msg, bool isError);
    void MsgBox2(const std::string& title, const std::string& msg) { MsgBox(title, msg, false); }
    /// Shows a message with a custom icon. Image with iconIdx must exist in iconFile and iconFile must be loaded!
    void MsgBoxEx(const std::string& title, const std::string& msg, const std::string& iconFile, unsigned iconIdx);
    void MsgBoxEx2(const std::string& title, const std::string& msg, const std::string& iconFile, unsigned iconIdx,
                   int iconX, int iconY);

private:
    const ILocalGameState& localGameState;
};
