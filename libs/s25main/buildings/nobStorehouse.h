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

#include "nobBaseWarehouse.h"
class SerializedGameData;

class nobStorehouse : public nobBaseWarehouse
{
    friend class SerializedGameData;
    friend class BuildingFactory;
    nobStorehouse(MapPoint pos, unsigned char player, Nation nation);
    nobStorehouse(SerializedGameData& sgd, unsigned obj_id);

protected:
    void Serialize_nobStorehouse(SerializedGameData& sgd) const;

public:
    void Serialize(SerializedGameData& sgd) const override { Serialize_nobStorehouse(sgd); }

    GO_Type GetGOT() const override { return GOT_NOB_STOREHOUSE; }
    unsigned GetMilitaryRadius() const override { return 0; }
    bool IsAttackable(unsigned /*playerIdx*/) const override { return false; }

    void Draw(DrawPoint drawPt) override;

    void HandleEvent(unsigned id) override;
};
