// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>

class GameWorldBase;
class AIPlayer;
namespace AI {
struct Info;
}

class AIFactory
{
public:
    AIFactory() = delete;

    static std::unique_ptr<AIPlayer> Create(const AI::Info& aiInfo, unsigned playerId, const GameWorldBase& world);
};
