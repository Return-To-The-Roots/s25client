// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#include "defines.h" // IWYU pragma: keep
#include "gameTypes/MapNode.h"
#include "SerializedGameData.h"
#include "nodeObjs/noBase.h"

void MapNode::Serialize(SerializedGameData& sgd, const unsigned numPlayers) const
{
    for(unsigned z = 0; z < roads_real.size(); ++z)
        sgd.PushUnsignedChar(roads_real[z]);

    sgd.PushUnsignedChar(altitude);
    sgd.PushUnsignedChar(shadow);
    sgd.PushUnsignedChar(t1);
    sgd.PushUnsignedChar(t2);
    sgd.PushUnsignedChar(resources);
    sgd.PushBool(reserved);
    sgd.PushUnsignedChar(owner);
    for(unsigned b = 0; b < boundary_stones.size(); ++b)
        sgd.PushUnsignedChar(boundary_stones[b]);
    sgd.PushUnsignedChar(static_cast<unsigned char>(bq));
    RTTR_Assert(numPlayers < fow.size());
    for(unsigned z = 0; z < numPlayers; ++z)
        fow[z].Serialize(sgd);
    sgd.PushObject(obj, false);
    sgd.PushObjectContainer(figures, false);
    sgd.PushUnsignedShort(sea_id);
    sgd.PushUnsignedInt(harbor_id);
}

void MapNode::Deserialize(SerializedGameData& sgd, const unsigned numPlayers)
{
    for(unsigned z = 0; z < roads_real.size(); ++z)
    {
        roads_real[z] = sgd.PopUnsignedChar();
        RTTR_Assert(roads_real[z] < 4);
    }

    altitude = sgd.PopUnsignedChar();
    shadow = sgd.PopUnsignedChar();
    t1 = TerrainType(sgd.PopUnsignedChar());
    RTTR_Assert(t1 < TT_COUNT);
    t2 = TerrainType(sgd.PopUnsignedChar());
    RTTR_Assert(t2 < TT_COUNT);
    resources = sgd.PopUnsignedChar();
    reserved = sgd.PopBool();
    owner = sgd.PopUnsignedChar();
    for(unsigned b = 0; b < boundary_stones.size(); ++b)
        boundary_stones[b] = sgd.PopUnsignedChar();
    bq = BuildingQuality(sgd.PopUnsignedChar());
    RTTR_Assert(numPlayers < fow.size());
    for(unsigned z = 0; z < numPlayers; ++z)
        fow[z].Deserialize(sgd);
    obj = sgd.PopObject<noBase>(GOT_UNKNOWN);
    sgd.PopObjectContainer(figures, GOT_UNKNOWN);
    sea_id = sgd.PopUnsignedShort();
    harbor_id = sgd.PopUnsignedInt();
}
