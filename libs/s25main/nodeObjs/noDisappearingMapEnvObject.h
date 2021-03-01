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

#include "noDisappearingEnvObject.h"
class SerializedGameData;

/// Verschwindendes Umwelt-Objekt ohne weiter Bedeutung (z.b. Baumstamm etc.)
class noDisappearingMapEnvObject : public noDisappearingEnvObject
{
public:
    noDisappearingMapEnvObject(MapPoint pos, unsigned short map_id);
    noDisappearingMapEnvObject(SerializedGameData& sgd, unsigned obj_id);

    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const final { return GO_Type::Disappearingmapenvobject; }

    /// An x,y zeichnen.
    void Draw(DrawPoint drawPt) override;

private:
    /// ID in der mapsx.lst
    const unsigned short map_id;
};
