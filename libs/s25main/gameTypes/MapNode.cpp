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

void MapNode::serialize(SerializedGameData& sgd, const unsigned numPlayers, const WorldDescription& desc) const
{
    helpers::pushContainer(sgd, roads);
    sgd.pushUnsignedChar(altitude);
    sgd.pushUnsignedChar(shadow);
    sgd.pushString(desc.get(t1).name);
    sgd.pushString(desc.get(t2).name);
    sgd.pushUnsignedChar(resources.getValue());
    sgd.pushBool(reserved);
    sgd.pushUnsignedChar(owner);
    helpers::pushContainer(sgd, boundary_stones);
    RTTR_Assert(numPlayers <= fow.size());
    for(unsigned z = 0; z < numPlayers; ++z)
        fow[z].serialize(sgd);
    sgd.pushObject(obj);
    sgd.pushObjectContainer(figures);
    sgd.pushUnsignedShort(seaId.value());
    sgd.pushUnsignedInt(harborId.value());
}

void MapNode::deserialize(SerializedGameData& sgd, const unsigned numPlayers, const WorldDescription& desc,
                          const std::vector<DescIdx<TerrainDesc>>& landscapeTerrains)
{
    helpers::popContainer(sgd, roads);

    altitude = sgd.popUnsignedChar();
    shadow = sgd.popUnsignedChar();

    if(sgd.getGameDataVersion() < 3)
    {
        // TODO: Remove this and lt param
        t1 = landscapeTerrains[sgd.popUnsignedChar()];
        t2 = landscapeTerrains[sgd.popUnsignedChar()];
    } else
    {
        std::string sName = sgd.popString();
        t1 = desc.terrain.getIndex(sName);
        if(!t1)
            throw SerializedGameData::Error("Terrain with name '" + sName + "' not found");
        sName = sgd.popString();
        t2 = desc.terrain.getIndex(sName);
        if(!t2)
            throw SerializedGameData::Error("Terrain with name '" + sName + "' not found");
    }
    resources = Resource(sgd.popUnsignedChar());
    reserved = sgd.popBool();
    owner = sgd.popUnsignedChar();
    helpers::popContainer(sgd, boundary_stones);
    if(sgd.getGameDataVersion() < 9)
        bq = sgd.pop<BuildingQuality>();
    RTTR_Assert(numPlayers <= fow.size());
    for(unsigned z = 0; z < numPlayers; ++z)
        fow[z].deserialize(sgd);
    obj = sgd.popObject<noBase>();
    sgd.popObjectContainer(figures);
    seaId = SeaId(sgd.popUnsignedShort());
    harborId = HarborId(sgd.popUnsignedInt());
}
