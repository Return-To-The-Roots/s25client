// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "helpers/EnumArray.h"
#include "nodeObjs/noCoordBase.h"
#include "gameTypes/Inventory.h"
#include "gameTypes/JobTypes.h"
#include <array>
class SerializedGameData;

/// Unsichtbares Objekt, welches die fliehenden Leute aus einem ehemaligen abgebrannten Lagerhaus/HQ spuckt
class BurnedWarehouse : public noCoordBase
{
public:
    using PeopleArray = PeopleArray<unsigned>;

    BurnedWarehouse(MapPoint pos, unsigned char player, const PeopleArray& people);
    BurnedWarehouse(SerializedGameData& sgd, unsigned obj_id);

    ~BurnedWarehouse() override;

    void Destroy() override;
    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const final { return GO_Type::Burnedwarehouse; }

    /// Benachrichtigen, wenn neuer GF erreicht wurde.
    void HandleEvent(unsigned id) override;

    void Draw(DrawPoint /*drawPt*/) override {}

private:
    /// Spieler des ehemaligen Lagerhauses
    const unsigned char player;
    /// Aktuelle Rausgeh-Phase
    unsigned go_out_phase;
    // Leute, die noch rauskommen müssen
    PeopleArray people;
};
