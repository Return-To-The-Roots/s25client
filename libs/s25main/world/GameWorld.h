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

#ifndef GameWorld_h__
#define GameWorld_h__

#include "world/GameWorldGame.h"
#include "world/GameWorldViewer.h"
#include "gameTypes/MapCoordinates.h"
#include <memory>
#include <string>

class SerializedGameData;
class Game;

class GameWorld : public GameWorldGame
{
public:
    GameWorld(const std::vector<PlayerInfo>& playerInfos, const GlobalGameSettings& gameSettings, EventManager& em);

    /// Lädt eine Karte
    bool LoadMap(std::shared_ptr<Game> game, const std::string& mapFilePath, const std::string& luaFilePath);

    /// Serialisiert den gesamten GameWorld
    void Serialize(SerializedGameData& sgd) const;
    void Deserialize(std::shared_ptr<Game> game, SerializedGameData& sgd);
};

#endif // GameWorld_h__
