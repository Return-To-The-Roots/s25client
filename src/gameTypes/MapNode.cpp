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

#include "rttrDefines.h" // IWYU pragma: keep
#include "gameTypes/MapNode.h"
#include "SerializedGameData.h"
#include "nodeObjs/noBase.h"
#include <algorithm>

MapNode::MapNode()
    : altitude(10), shadow(64), t1(TT_SNOW), t2(TT_SNOW), resources(0), reserved(false), owner(0), bq(BQ_NOTHING), seaId(0), harborId(0),
      obj(NULL)
{
    std::fill(roads.begin(), roads.end(), 0);
    std::fill(boundary_stones.begin(), boundary_stones.end(), 0);
}

void MapNode::Serialize(SerializedGameData& sgd, const unsigned numPlayers) const
{
    for(unsigned z = 0; z < roads.size(); ++z)
        sgd.PushUnsignedChar(roads[z]);

    sgd.PushUnsignedChar(altitude);
    sgd.PushUnsignedChar(shadow);
    sgd.PushUnsignedChar(t1);
    sgd.PushUnsignedChar(t2);
    sgd.PushUnsignedChar(static_cast<uint8_t>(resources.getValue()));
    sgd.PushBool(reserved);
    sgd.PushUnsignedChar(owner);
    for(unsigned b = 0; b < boundary_stones.size(); ++b)
        sgd.PushUnsignedChar(boundary_stones[b]);
    sgd.PushUnsignedChar(static_cast<unsigned char>(bq));
    RTTR_Assert(numPlayers <= fow.size());
    for(unsigned z = 0; z < numPlayers; ++z)
        fow[z].Serialize(sgd);
    sgd.PushObject(obj, false);
    sgd.PushObjectContainer(figures, false);
    sgd.PushUnsignedShort(seaId);
    sgd.PushUnsignedInt(harborId);
}

void MapNode::Deserialize(SerializedGameData& sgd, const unsigned numPlayers)
{
    for(unsigned z = 0; z < roads.size(); ++z)
    {
        roads[z] = sgd.PopUnsignedChar();
        RTTR_Assert(roads[z] < 4);
    }

    altitude = sgd.PopUnsignedChar();
    shadow = sgd.PopUnsignedChar();
    t1 = TerrainType(sgd.PopUnsignedChar());
    RTTR_Assert(t1 < NUM_TTS);
    t2 = TerrainType(sgd.PopUnsignedChar());
    RTTR_Assert(t2 < NUM_TTS);
    resources = Resource(sgd.PopUnsignedChar());
    reserved = sgd.PopBool();
    owner = sgd.PopUnsignedChar();
    for(unsigned b = 0; b < boundary_stones.size(); ++b)
        boundary_stones[b] = sgd.PopUnsignedChar();
    bq = BuildingQuality(sgd.PopUnsignedChar());
    RTTR_Assert(numPlayers <= fow.size());
    for(unsigned z = 0; z < numPlayers; ++z)
        fow[z].Deserialize(sgd);
    obj = sgd.PopObject<noBase>(GOT_UNKNOWN);
    sgd.PopObjectContainer(figures, GOT_UNKNOWN);
    seaId = sgd.PopUnsignedShort();
    harborId = sgd.PopUnsignedInt();
}
