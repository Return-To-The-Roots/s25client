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

#include "gameTypes/MapNode.h"
#include "SerializedGameData.h"
#include "nodeObjs/noBase.h"
#include "gameData/TerrainDesc.h"
#include "gameData/WorldDescription.h"
#include <algorithm>

MapNode::MapNode()
    : altitude(10), shadow(64), t1(0), t2(0), resources(0), reserved(false), owner(0), bq(BuildingQuality::Nothing),
      seaId(0), harborId(0), obj(nullptr)
{
    std::fill(roads.begin(), roads.end(), PointRoad::None);
    std::fill(boundary_stones.begin(), boundary_stones.end(), 0);
}

void MapNode::Serialize(SerializedGameData& sgd, const unsigned numPlayers, const WorldDescription& desc) const
{
    for(PointRoad road : roads)
        sgd.PushEnum<uint8_t>(road);

    sgd.PushUnsignedChar(altitude);
    sgd.PushUnsignedChar(shadow);
    sgd.PushString(desc.get(t1).name);
    sgd.PushString(desc.get(t2).name);
    sgd.PushUnsignedChar(resources.getValue());
    sgd.PushBool(reserved);
    sgd.PushUnsignedChar(owner);
    for(unsigned char boundary_stone : boundary_stones)
        sgd.PushUnsignedChar(boundary_stone);
    sgd.PushEnum<uint8_t>(bq);
    RTTR_Assert(numPlayers <= fow.size());
    for(unsigned z = 0; z < numPlayers; ++z)
        fow[z].Serialize(sgd);
    sgd.PushObject(obj);
    sgd.PushObjectContainer(figures);
    sgd.PushUnsignedShort(seaId);
    sgd.PushUnsignedInt(harborId);
}

void MapNode::Deserialize(SerializedGameData& sgd, const unsigned numPlayers, const WorldDescription& desc,
                          const std::vector<DescIdx<TerrainDesc>>& landscapeTerrains)
{
    for(PointRoad& road : roads)
        road = sgd.Pop<PointRoad>();

    altitude = sgd.PopUnsignedChar();
    shadow = sgd.PopUnsignedChar();

    if(sgd.GetGameDataVersion() < 3)
    {
        // TODO: Remove this and lt param
        t1 = landscapeTerrains[sgd.PopUnsignedChar()];
        t2 = landscapeTerrains[sgd.PopUnsignedChar()];
    } else
    {
        std::string sName = sgd.PopString();
        t1 = desc.terrain.getIndex(sName);
        if(!t1)
            throw SerializedGameData::Error("Terrain with name '" + sName + "' not found");
        sName = sgd.PopString();
        t2 = desc.terrain.getIndex(sName);
        if(!t2)
            throw SerializedGameData::Error("Terrain with name '" + sName + "' not found");
    }
    resources = Resource(sgd.PopUnsignedChar());
    reserved = sgd.PopBool();
    owner = sgd.PopUnsignedChar();
    for(uint8_t& boundary_stone : boundary_stones)
        boundary_stone = sgd.PopUnsignedChar();
    bq = sgd.Pop<BuildingQuality>();
    RTTR_Assert(numPlayers <= fow.size());
    for(unsigned z = 0; z < numPlayers; ++z)
        fow[z].Deserialize(sgd);
    obj = sgd.PopObject<noBase>();
    sgd.PopObjectContainer(figures);
    seaId = sgd.PopUnsignedShort();
    harborId = sgd.PopUnsignedInt();
}
