// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

class GameWorldBase;
class SerializedGameData;
class Game;
class ILocalGameState;

class MapSerializer
{
public:
    static void Serialize(const GameWorldBase& world, SerializedGameData& sgd);
    static void Deserialize(GameWorldBase& world, SerializedGameData& sgd, Game& game, ILocalGameState& localgameState);
};
