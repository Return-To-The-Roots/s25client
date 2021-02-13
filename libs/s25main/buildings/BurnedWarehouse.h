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

#include "helpers/EnumArray.h"
#include "nodeObjs/noCoordBase.h"
#include "gameTypes/JobTypes.h"
#include <array>
class SerializedGameData;

/// Unsichtbares Objekt, welches die fliehenden Leute aus einem ehemaligen abgebrannten Lagerhaus/HQ spuckt
class BurnedWarehouse : public noCoordBase
{
public:
    using PeopleArray = helpers::EnumArray<uint32_t, Job>;

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
