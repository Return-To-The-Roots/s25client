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
    void PauseWalking();

public:
    noMovable(NodalObjectType nop, MapPoint pos);
    noMovable(SerializedGameData& sgd, unsigned obj_id);

    void Serialize(SerializedGameData& sgd) const override;

    /// Sets the position. Usually not required as all position changes are done by the walk functions
    void SetPos(const MapPoint& pos) { this->pos = pos; }

    /// Returns the direction in which the object is moving/which it is facing
    Direction GetCurMoveDir() const { return curMoveDir; }
    uint8_t GetAscent() const { return ascent; }
    const EventState& GetPausedEvent() { return pauseEv; }
    /// "Turns" the object in that direction without starting to walk
    void FaceDir(Direction newDir);
    /// In aktueller Richtung ein Stück zurücklegen
    void Walk();
    // Starten zu Laufen, Event anmelden
    void StartMoving(Direction dir, unsigned gf_length);
    // Interpoliert die Position zwischen zwei Knoten punkten
    DrawPoint CalcRelative(DrawPoint curPt, DrawPoint nextPt) const;
    /// Interpoliert fürs Laufen zwischen zwei Kartenpunkten
    DrawPoint CalcWalkingRelative() const;
    // Steht er in der zwischen 2 Wegpunkten?
    bool IsStoppedBetweenNodes() const { return pauseEv.elapsed > 0; }
    /// Gibt die Position zurück, wo wir uns hinbewegen (selbe Position, wenn Schiff steht)
    MapPoint GetDestinationForCurrentMove() const;
    /// Gibt zurück, ob sich das angegebene Objekt zwischen zwei Punkten bewegt
    bool IsMoving() const override;
};
