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

#include "world/MapSerializer.h"
#include "CatapultStone.h"
#include "SerializedGameData.h"
#include "lua/GameDataLoader.h"
#include "world/World.h"
#include <mygettext/mygettext.h>

void MapSerializer::Serialize(const World& world, const unsigned numPlayers, SerializedGameData& sgd)
{
    // Headinformationen
    sgd.PushPoint(world.GetSize());
    sgd.PushString(world.GetDescription().get(world.GetLandscapeType()).name);

    sgd.PushUnsignedInt(GameObject::GetObjIDCounter());

    // Alle Weltpunkte serialisieren
    for(const auto& node : world.nodes)
    {
        node.Serialize(sgd, numPlayers, world.GetDescription());
    }

    // Katapultsteine serialisieren
    sgd.PushObjectContainer(world.catapult_stones, true);
    // Meeresinformationen serialisieren
    sgd.PushUnsignedInt(world.seas.size());
    for(auto sea : world.seas)
    {
        sgd.PushUnsignedInt(sea.nodes_count);
    }
    // Hafenpositionen serialisieren
    sgd.PushUnsignedInt(world.harbor_pos.size());
    for(const auto& curHarborPos : world.harbor_pos)
    {
        sgd.PushMapPoint(curHarborPos.pos);
        for(unsigned z = 0; z < 6; ++z)
            sgd.PushUnsignedShort(curHarborPos.cps[z].seaId);
        for(unsigned z = 0; z < 6; ++z)
        {
            sgd.PushUnsignedInt(curHarborPos.neighbors[z].size());

            for(auto c : curHarborPos.neighbors[z])
            {
                sgd.PushUnsignedInt(c.id);
                sgd.PushUnsignedInt(c.distance);
            }
        }
    }
}

void MapSerializer::Deserialize(World& world, const unsigned numPlayers, SerializedGameData& sgd)
{
    // Initialisierungen
    GameDataLoader gdLoader(world.GetDescriptionWriteable());
    if(!gdLoader.Load())
        throw SerializedGameData::Error(_("Failed to load game data!"));

    // Headinformationen
    const MapExtent size = sgd.PopPoint<MapExtent::ElementType>();
    DescIdx<LandscapeDesc> lt(0);
    if(sgd.GetGameDataVersion() < 3)
    {
        uint8_t gfxSet = sgd.PopUnsignedChar();
        for(DescIdx<LandscapeDesc> i(0); i.value < world.GetDescription().landscapes.size(); i.value++)
        {
            if(world.GetDescription().get(i).s2Id == gfxSet)
            {
                lt = i;
                break;
            }
        }
    } else
    {
        std::string sLandscape = sgd.PopString();
        lt = world.GetDescription().landscapes.getIndex(sLandscape);
        if(!lt)
            throw SerializedGameData::Error(std::string("Invalid landscape: ") + sLandscape);
    }
    world.Init(size, lt);
    GameObject::ResetCounters(sgd.PopUnsignedInt());

    std::vector<DescIdx<TerrainDesc>> landscapeTerrains;
    if(sgd.GetGameDataVersion() < 3)
    {
        // Assumes the order of the terrain in the description file is the same as in the prior RTTR versions
        for(DescIdx<TerrainDesc> t(0); t.value < world.GetDescription().terrain.size(); t.value++)
        {
            if(world.GetDescription().get(t).landscape == lt)
                landscapeTerrains.push_back(t);
        }
    }
    // Alle Weltpunkte
    MapPoint curPos(0, 0);
    for(auto& node : world.nodes)
    {
        node.Deserialize(sgd, numPlayers, world.GetDescription(), landscapeTerrains);
        if(node.harborId)
        {
            HarborPos p(curPos);
            world.harbor_pos.push_back(p);
        }
        curPos.x++;
        if(curPos.x >= world.GetWidth())
        {
            curPos.x = 0;
            curPos.y++;
        }
    }

    // Katapultsteine deserialisieren
    sgd.PopObjectContainer(world.catapult_stones, GOT_CATAPULTSTONE);

    // Meeresinformationen deserialisieren
    world.seas.resize(sgd.PopUnsignedInt());
    for(auto& sea : world.seas)
    {
        sea.nodes_count = sgd.PopUnsignedInt();
    }

    // Hafenpositionen serialisieren
    world.harbor_pos.resize(sgd.PopUnsignedInt());
    for(auto& curHarborPos : world.harbor_pos)
    {
        curHarborPos.pos = sgd.PopMapPoint();
        for(unsigned z = 0; z < 6; ++z)
            curHarborPos.cps[z].seaId = sgd.PopUnsignedShort();
        for(unsigned z = 0; z < 6; ++z)
        {
            curHarborPos.neighbors[z].resize(sgd.PopUnsignedInt());
            for(auto& c : curHarborPos.neighbors[z])
            {
                c.id = sgd.PopUnsignedInt();
                c.distance = sgd.PopUnsignedInt();
            }
        }
    }
}
