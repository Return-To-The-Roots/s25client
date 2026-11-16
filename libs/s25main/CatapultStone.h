// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

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
    CatapultStone(MapPoint dest_building, MapPoint dest_map, DrawPoint start, DrawPoint dest, unsigned fly_duration);

    CatapultStone(SerializedGameData& sgd, unsigned obj_id);

    /// Zerstören
    void destroy() override;
    void Serialize(SerializedGameData& sgd) const override;

    // Zeichnet den fliegenden Stein
    void draw(DrawPoint drawOffset);

    /// Event-Handler
    void handleEvent(unsigned id) override;

    GO_Type getGOT() const final { return GO_Type::Catapultstone; }
};
