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

class SerializedGameData;
class GameEvent;

// Klasse für ein brennendes Gebäude
class noFire : public noCoordBase
{
    /// Größe des Feuers: klein (0) oder groß (1)
    bool isBig;
    /// "Todesevent" (also bis es abgebrannt ist) speichern, damit dann interpoliert wird
    const GameEvent* dead_event;
    /// Wurden Feuersounds abgespielt
    bool was_sounding;
    /// Letzter Feuersound-Zeitpunkt
    unsigned last_sound;
    /// Intervall zum nächsten Feuersound
    unsigned next_interval;

public:
    noFire(MapPoint pos, bool isBig);
    noFire(SerializedGameData& sgd, unsigned obj_id);

    ~noFire() override;
    /// Aufräummethoden
protected:
    void Destroy_noFire();

public:
    void Destroy() override { Destroy_noFire(); }

    /// Serialisierungsfunktionen
protected:
    void Serialize_noFire(SerializedGameData& sgd) const;

public:
    void Serialize(SerializedGameData& sgd) const override { Serialize_noFire(sgd); }

    GO_Type GetGOT() const override { return GOT_FIRE; }

    BlockingManner GetBM() const override { return BlockingManner::FlagsAround; }

    /// Zeichnen
    void Draw(DrawPoint drawPt) override;
    /// Benachrichtigen, wenn neuer gf erreicht wurde
    void HandleEvent(unsigned id) override;
};
