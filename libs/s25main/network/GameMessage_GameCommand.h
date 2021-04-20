// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
