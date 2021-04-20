// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AIFactory.h"
#include "ai/DummyAI.h"
#include "ai/aijh/AIPlayerJH.h"
#include "gameTypes/AIInfo.h"

std::unique_ptr<AIPlayer> AIFactory::Create(const AI::Info& aiInfo, unsigned playerId, const GameWorldBase& world)
{
    switch(aiInfo.type)
    {
        case AI::Type::Dummy: return std::make_unique<DummyAI>(playerId, world, aiInfo.level); break;
        case AI::Type::Default:
        default: return std::make_unique<AIJH::AIPlayerJH>(playerId, world, aiInfo.level); break;
    }
}
