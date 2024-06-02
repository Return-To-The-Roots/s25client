// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "noCoordBase.h"

class SerializedGameData;
class GameEvent;

class noGrapefield : public noCoordBase
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
    noGrapefield(MapPoint pos);
    noGrapefield(SerializedGameData& sgd, unsigned obj_id);

    ~noGrapefield() override;

    void Destroy() override;
    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const final { return GO_Type::Grapefield; }

    void Draw(DrawPoint drawPt) override;
    void HandleEvent(unsigned id) override;

    BlockingManner GetBM() const override { return BlockingManner::FlagsAround; }

    /// Kann man es abernten?
    bool IsHarvestable() const { return size == 3 && state == State::Normal; }

    /// Gibt die ID des abgeernteten Weinbergs in der wine_bobs zurück
    unsigned GetHarvestID() const;

    /// Bauer beginnt dieses Feld abzuernten
    void BeginHarvesting();
    /// Bauer wird beim Abernten unterbrochen
    void EndHarvesting();
};
