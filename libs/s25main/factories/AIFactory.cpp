// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "AIFactory.h"
#include "ai/DummyAI.h"
#include "ai/aijh/AIPlayerJH.h"
#include "gameTypes/AIInfo.h"

std::unique_ptr<AIPlayer> AIFactory::Create(const AI::Info& aiInfo, unsigned playerId, const GameWorldBase& world)
{
    switch(aiInfo.type)
    {
        case AI::DUMMY: return std::make_unique<DummyAI>(playerId, world, aiInfo.level); break;
        case AI::DEFAULT:
        default: return std::make_unique<AIJH::AIPlayerJH>(playerId, world, aiInfo.level); break;
    }
}
