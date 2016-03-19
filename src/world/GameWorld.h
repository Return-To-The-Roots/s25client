// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#include "gameTypes/MapTypes.h"
#include <string>

class SerializedGameData;

class GameWorld : public GameWorldViewer, public GameWorldGame
{
    public:

        /// Lädt eine Karte
        bool LoadMap(const std::string& mapFilePath, const std::string& luaFilePath);

        /// Serialisiert den gesamten GameWorld
        void Serialize(SerializedGameData& sgd) const;
        void Deserialize(SerializedGameData& sgd);

        /// Sagt der GW Bescheid, dass ein Objekt von Bedeutung an x,y vernichtet wurde, damit dieser
        /// dass ggf. an den WindowManager weiterleiten kann, damit auch ein Fenster wieder geschlossen wird
        void ImportantObjectDestroyed(const MapPoint pt) override;
        /// Sagt, dass ein Militärgebäude eingenommen wurde und ggf. ein entsprechender "Fanfarensound" abgespielt werden sollte
        void MilitaryBuildingCaptured(const MapPoint pt, const unsigned char player) override;

    private:
 
};

#endif // GameWorld_h__
