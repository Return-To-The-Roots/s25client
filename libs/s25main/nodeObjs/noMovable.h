// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "noCoordBase.h"
#include "gameTypes/Direction.h"
#include "gameTypes/MapCoordinates.h"
#include <array>
#include <cstdint>

class SerializedGameData;
class GameEvent;

/// Anzahl Animationsschritte bei dem jeweiligen Anstieg
const std::array<unsigned short, 7> ASCENT_ANIMATION_STEPS = {16, 16, 16, 16, 24, 32, 48};

struct EventState
{
    unsigned elapsed, length;
    EventState() : elapsed(0), length(0) {}
    EventState(unsigned elapsed, unsigned length) : elapsed(elapsed), length(length) {}
    explicit EventState(SerializedGameData& sgd);
};

class noMovable : public noCoordBase
{
    Direction curMoveDir; /// Current move direction
    uint8_t ascent;       /// Current ascent (0-2 runter, 3 gerade, 4-6 hoch)
    /// If stopped, how long did it walk and how much it should have walked, 0 if not stopped
    EventState pauseEv;
    /// Is it currently moving (for debugging)
    bool moving;

protected:
    const GameEvent* current_ev;
    /// Pausiert ein gerade laufendes Wesen
    void pauseWalking();

public:
    noMovable(NodalObjectType nop, MapPoint pos);
    noMovable(SerializedGameData& sgd, unsigned obj_id);

    void serialize(SerializedGameData& sgd) const override;

    /// Sets the position. Usually not required as all position changes are done by the walk functions
    void setPos(const MapPoint& pos) { this->pos = pos; }

    /// Returns the direction in which the object is moving/which it is facing
    Direction getCurMoveDir() const { return curMoveDir; }
    uint8_t getAscent() const { return ascent; }
    const EventState& getPausedEvent() { return pauseEv; }
    /// "Turns" the object in that direction without starting to walk
    void faceDir(Direction newDir);
    /// In aktueller Richtung ein Stück zurücklegen
    void walk();
    // Starten zu Laufen, Event anmelden
    void startMoving(Direction dir, unsigned gf_length);
    // Interpoliert die Position zwischen zwei Knoten punkten
    DrawPoint calcRelative(DrawPoint curPt, DrawPoint nextPt) const;
    /// Interpoliert fürs Laufen zwischen zwei Kartenpunkten
    DrawPoint calcWalkingRelative() const;
    // Steht er in der zwischen 2 Wegpunkten?
    bool isStoppedBetweenNodes() const { return pauseEv.elapsed > 0; }
    /// Gibt die Position zurück, wo wir uns hinbewegen (selbe Position, wenn Schiff steht)
    MapPoint getDestinationForCurrentMove() const;
    /// Gibt zurück, ob sich das angegebene Objekt zwischen zwei Punkten bewegt
    bool isMoving() const override;
};
