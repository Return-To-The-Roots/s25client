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
#ifndef CATAPULT_STONE_H_
#define CATAPULT_STONE_H_

#include "GameObject.h"
#include "gameTypes/MapTypes.h"

class GameWorldView;
class SerializedGameData;
class GameEvent;

/// Klasse für einen fliegenden Katapultstein
class CatapultStone : public GameObject
{
    private:

        /// (Map-)Koordinaten des Hauses, in dem er einschlagen soll
        const MapPoint dest_building;
        /// Aufschlagspunkt des Steines (Map-Koordinaten!)
        const MapPoint dest_map;
        /// Koordinaten der Startposition des Steins
        const int start_x, start_y;
        /// Koordinaten der Zielposition des Steins
        const int dest_x, dest_y;
        /// Explodiert der Stein schon? (false = fliegt)
        bool explode;
        /// Flieg-/Explodier-Event
        GameEvent* event;

    public:

        CatapultStone(const MapPoint dest_building, const MapPoint dest_map,
                      const int start_x, const int start_y, const int dest_x, const int dest_y, const unsigned fly_duration);

        CatapultStone(SerializedGameData& sgd, const unsigned obj_id);

        ~CatapultStone() override {}

        /// Zerstören
        void Destroy() override;

        /// Serialisierungsfunktionen
    protected:  void Serialize_CatapultStone(SerializedGameData& sgd) const;
    public:     void Serialize(SerializedGameData& sgd) const override { Serialize_CatapultStone(sgd); }

        // Zeichnet den fliegenden Stein
        void Draw(const GameWorldView& gwv, const int xoffset, const int yoffset);

        /// Event-Handler
        void HandleEvent(const unsigned int id) override;

        GO_Type GetGOT() const override { return GOT_CATAPULTSTONE; }
};

#endif // !CATAPULT_STONE_H_
