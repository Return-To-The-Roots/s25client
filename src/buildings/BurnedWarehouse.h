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
#ifndef BURNED_WAREHOUSE_H_
#define BURNED_WAREHOUSE_H_

#include "nodeObjs/noCoordBase.h"
#include "gameTypes/JobTypes.h"
#include <boost/array.hpp>
class SerializedGameData;

/// Unsichtbares Objekt, welches die fliehenden Leute aus einem ehemaligen abgebrannten Lagerhaus/HQ spuckt
class BurnedWarehouse : public noCoordBase
{
    /// Spieler des ehemaligen Lagerhauses
    const unsigned char player;
    /// Aktuelle Rausgeh-Phase
    unsigned go_out_phase;
    // Leute, die noch rauskommen müssen
    boost::array<unsigned, JOB_TYPES_COUNT> people;

public:
    typedef boost::array<unsigned, JOB_TYPES_COUNT> PeopleArray;

    BurnedWarehouse(const MapPoint pt, const unsigned char player, const PeopleArray& people);
    BurnedWarehouse(SerializedGameData& sgd, const unsigned obj_id);

    ~BurnedWarehouse() override;

    void Destroy() override;

    /// Serialisierungsfunktionen
protected:
    void Serialize_BurnedWarehouse(SerializedGameData& sgd) const;

public:
    void Serialize(SerializedGameData& sgd) const override { Serialize_BurnedWarehouse(sgd); }

    GO_Type GetGOT() const override { return GOT_BURNEDWAREHOUSE; }

    /// Benachrichtigen, wenn neuer GF erreicht wurde.
    void HandleEvent(const unsigned id) override;

    void Draw(DrawPoint /*drawPt*/) override {}
};

#endif
