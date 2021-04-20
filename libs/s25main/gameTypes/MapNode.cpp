// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
    helpers::pushContainer(sgd, roads);
    sgd.PushUnsignedChar(altitude);
    sgd.PushUnsignedChar(shadow);
    sgd.PushString(desc.get(t1).name);
    sgd.PushString(desc.get(t2).name);
    sgd.PushUnsignedChar(resources.getValue());
    sgd.PushBool(reserved);
    sgd.PushUnsignedChar(owner);
    helpers::pushContainer(sgd, boundary_stones);
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
    helpers::popContainer(sgd, roads);

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
    helpers::popContainer(sgd, boundary_stones);
    bq = sgd.Pop<BuildingQuality>();
    RTTR_Assert(numPlayers <= fow.size());
    for(unsigned z = 0; z < numPlayers; ++z)
        fow[z].Deserialize(sgd);
    obj = sgd.PopObject<noBase>();
    sgd.PopObjectContainer(figures);
    seaId = sgd.PopUnsignedShort();
    harborId = sgd.PopUnsignedInt();
}
