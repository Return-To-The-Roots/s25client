// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/Nation.h"
#include "gameTypes/TeamTypes.h"
#include <string>

namespace kaguya {
class State;
}
struct BasePlayerInfo;

class LuaPlayerBase
{
protected:
    LuaPlayerBase() = default;
    virtual ~LuaPlayerBase() = default;

    virtual const BasePlayerInfo& GetPlayer() const = 0;

public:
    static void Register(kaguya::State& state);

    std::string GetName() const;
    Nation GetNation() const;
    Team GetTeam() const;
    unsigned GetColor() const;
    bool IsHuman() const;
    bool IsAI() const;
    bool IsClosed() const;
    bool IsFree() const;
    int GetAILevel() const;
};
