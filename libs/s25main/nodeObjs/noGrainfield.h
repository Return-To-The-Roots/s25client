// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
