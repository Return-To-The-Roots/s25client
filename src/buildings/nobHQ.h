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

#ifndef NOB_HQ_H_
#define NOB_HQ_H_

#include "nobBaseWarehouse.h"
#include "gameData/MilitaryConsts.h"
class SerializedGameData;

class nobHQ : public nobBaseWarehouse
{
    /// True if tent graphic should be used
    bool isTent_;

public:
    nobHQ(const MapPoint pt, const unsigned char player, const Nation nation, const bool isTent = false);
    nobHQ(SerializedGameData& sgd, const unsigned obj_id);

protected:
    void DestroyBuilding() override;
    void Serialize_nobHQ(SerializedGameData& sgd) const;

public:
    void Serialize(SerializedGameData& sgd) const override { Serialize_nobHQ(sgd); }

    GO_Type GetGOT() const override { return GOT_NOB_HQ; }

    void Draw(DrawPoint drawPt) override;

    unsigned GetMilitaryRadius() const override { return HQ_RADIUS; }

    void HandleEvent(const unsigned id) override;
    bool IsTent() const { return isTent_; }
    void SetIsTent(const bool isTent) { isTent_ = isTent; }
};

#endif
