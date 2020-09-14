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

#pragma once

#include "GameCommand.h"
#include "GameMessage.h"
#include "PlayerGameCommands.h"
#include <vector>

class Serializer;

class GameMessage_GameCommand : public GameMessageWithPlayer
{
public:
    PlayerGameCommands cmds;

    GameMessage_GameCommand(); //-V730
    GameMessage_GameCommand(uint8_t player, const AsyncChecksum& checksum, const std::vector<gc::GameCommandPtr>& gcs);

    void Serialize(Serializer& ser) const override;
    void Deserialize(Serializer& ser) override;
    bool Run(GameMessageInterface* callback) const override;
};
