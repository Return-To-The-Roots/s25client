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

class noGrainfield : public noCoordBase
{
private:
    /// Typ des Getreidefelds (2 verschiedene Typen)
    unsigned char type;

    /// Status
    enum class State : uint8_t
    {
        GrowingWaiting, /// Wachsphase, wartet auf den nächsten Wachstumsschub
        Growing,        /// wächst
        Normal,         /// ist ausgewachsen und verdorrt nach einer Weile
        Withering       /// verdorrt (verschwindet)
    } state;
    friend constexpr auto maxEnumValue(State) { return State::Withering; }

    /// Größe des Feldes (0-3), 3 ist ausgewachsen
    unsigned char size;

    /// Wachs-Event
    const GameEvent* event;

public:
    noGrainfield(MapPoint pos);
    noGrainfield(SerializedGameData& sgd, unsigned obj_id);

    ~noGrainfield() override;

    void Destroy() override;
    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const final { return GO_Type::Grainfield; }

    void Draw(DrawPoint drawPt) override;
    void HandleEvent(unsigned id) override;

    BlockingManner GetBM() const override { return BlockingManner::FlagsAround; }

    /// Kann man es abernten?
    bool IsHarvestable() const { return size == 3 && state == State::Normal; }

    /// Gibt die ID des abgeernteten Getreidefelds in der map_last zurück
    unsigned GetHarvestMapLstID() const { return 532 + type * 5 + 4; }

    /// Bauer beginnt dieses Feld abzuernten
    void BeginHarvesting();
    /// Bauer wird beim Abernten unterbrochen
    void EndHarvesting();
};
