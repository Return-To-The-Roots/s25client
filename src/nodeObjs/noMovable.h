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

#ifndef NO_MOVABLE_H_
#define NO_MOVABLE_H_

#include "noCoordBase.h"
#include "gameTypes/Direction.h"
#include "gameTypes/MapCoordinates.h"

class SerializedGameData;
class GameEvent;

/// Anzahl Animationsschritte bei dem jeweiligen Anstieg
const unsigned short ASCENT_ANIMATION_STEPS[7] = {16, 16, 16, 16, 24, 32, 48};

class noMovable : public noCoordBase
{
    Direction curMoveDir; // Richtung, in die es gerade läcft
protected:
    unsigned char ascent; // Anstieg beim Laufen (0-2 runter, 3 gerade, 4-6 hoch)
    const GameEvent* current_ev;
    /// Falls er unterwegs angehalten ist: wie weit war er schon gelaufen (0 wenn nicht)
    unsigned pause_walked_gf;
    /// Wenn er angehalten hat, wie lange das Laufevent war
    unsigned pause_event_length;
    /// Läuft es gerade (zum Debuggen)
    bool moving;

protected:
    /// Pausiert ein gerade laufendes Wesen
    void PauseWalking();

public:
    noMovable(const NodalObjectType nop, const MapPoint pt);
    noMovable(SerializedGameData& sgd, const unsigned obj_id);

protected:
    void Destroy_noMovable() { Destroy_noCoordBase(); }

public:
    void Destroy() override { Destroy_noMovable(); }

protected:
    void Serialize_noMovable(SerializedGameData& sgd) const;

public:
    void Serialize(SerializedGameData& sgd) const override { Serialize_noMovable(sgd); }

    /// Returns the direction in which the object is moving/which it is facing
    Direction GetCurMoveDir() const { return curMoveDir; }
    /// Return true if the object is moving north which means it actually belongs to the target point
    /// as it has to be drawn from that point
    bool IsMovingUpwards() const { return curMoveDir == Direction::NORTHWEST || curMoveDir == Direction::NORTHEAST; }
    /// "Turns" the object in that direction without starting to walk
    void FaceDir(Direction newDir);
    /// In aktueller Richtung ein Stück zurücklegen
    void Walk();
    // Starten zu Laufen, Event anmelden
    void StartMoving(const Direction dir, unsigned gf_length);
    // Interpoliert die Position zwischen zwei Knoten punkten
    DrawPoint CalcRelative(const DrawPoint& curPt, const DrawPoint& nextPt) const;
    /// Interpoliert fürs Laufen zwischen zwei Kartenpunkten
    DrawPoint CalcWalkingRelative() const;
    // Steht er in der zwischen 2 Wegpunkten?
    bool IsStandingBetweenNodes() const { return (pause_walked_gf > 0) ? true : false; }
    /// Gibt die Position zurück, wo wir uns hinbewegen (selbe Position, wenn Schiff steht)
    MapPoint GetDestinationForCurrentMove() const;
    /// Gibt zurück, ob sich das angegebene Objekt zwischen zwei Punkten bewegt
    bool IsMoving() const override;
};

#endif
