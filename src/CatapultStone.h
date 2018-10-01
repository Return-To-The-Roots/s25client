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
#ifndef CATAPULT_STONE_H_
#define CATAPULT_STONE_H_

#include "DrawPoint.h"
#include "GameObject.h"
#include "gameTypes/MapCoordinates.h"

class SerializedGameData;
class GameEvent;

/// Klasse für einen fliegenden Katapultstein
class CatapultStone : public GameObject
{
public:
    /// Target which was aimed for
    const MapPoint dest_building;
    /// Actual point this is going to hit (if equal dest_building -> Hit!)
    const MapPoint dest_map;

private:
    /// Koordinaten der Startposition des Steins
    const Position startPos;
    /// Koordinaten der Zielposition des Steins
    const Position destPos;
    /// Explodiert der Stein schon? (false = fliegt)
    bool explode;
    /// Flieg-/Explodier-Event
    const GameEvent* event;

public:
    CatapultStone(const MapPoint dest_building, const MapPoint dest_map, const DrawPoint start, const DrawPoint dest,
                  const unsigned fly_duration);

    CatapultStone(SerializedGameData& sgd, const unsigned obj_id);

    /// Zerstören
    void Destroy() override;

    /// Serialisierungsfunktionen
protected:
    void Serialize_CatapultStone(SerializedGameData& sgd) const;

public:
    void Serialize(SerializedGameData& sgd) const override { Serialize_CatapultStone(sgd); }

    // Zeichnet den fliegenden Stein
    void Draw(DrawPoint drawOffset);

    /// Event-Handler
    void HandleEvent(const unsigned id) override;

    GO_Type GetGOT() const override { return GOT_CATAPULTSTONE; }
};

#endif // !CATAPULT_STONE_H_
