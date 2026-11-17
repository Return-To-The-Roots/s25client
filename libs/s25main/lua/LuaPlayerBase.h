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

    virtual const BasePlayerInfo& getPlayer() const = 0;

public:
    static void registerClasses(kaguya::State& state);

    std::string getName() const;
    Nation getNation() const;
    Team getTeam() const;
    unsigned getColor() const;
    bool isHuman() const;
    bool isAI() const;
    bool isClosed() const;
    bool isFree() const;
    int getAILevel() const;
};
