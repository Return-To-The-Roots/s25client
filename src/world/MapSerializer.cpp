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

#include "defines.h"
#include "world/MapSerializer.h"
#include "world/World.h"
#include "SerializedGameData.h"
#include "buildings/noBuildingSite.h"
#include "CatapultStone.h"

void MapSerializer::Serialize(const World& world, SerializedGameData& sgd)
{
    // Alle Weltpunkte serialisieren
    for(std::vector<MapNode>::const_iterator it = world.nodes.begin(); it != world.nodes.end(); ++it)
    {
        it->Serialize(sgd);
    }

    // Katapultsteine serialisieren
    sgd.PushObjectContainer(world.catapult_stones, true);
    // Meeresinformationen serialisieren
    sgd.PushUnsignedInt(world.seas.size());
    for(unsigned i = 0; i < world.seas.size(); ++i)
    {
        sgd.PushUnsignedInt(world.seas[i].nodes_count);
    }
    // Hafenpositionen serialisieren
    sgd.PushUnsignedInt(world.harbor_pos.size());
    for(unsigned i = 0; i < world.harbor_pos.size(); ++i)
    {
        sgd.PushMapPoint(world.harbor_pos[i].pos);
        for(unsigned z = 0; z < 6; ++z)
            sgd.PushUnsignedShort(world.harbor_pos[i].cps[z].sea_id);
        for(unsigned z = 0; z < 6; ++z)
        {
            sgd.PushUnsignedInt(world.harbor_pos[i].neighbors[z].size());

            for(unsigned c = 0; c < world.harbor_pos[i].neighbors[z].size(); ++c)
            {
                sgd.PushUnsignedInt(world.harbor_pos[i].neighbors[z][c].id);
                sgd.PushUnsignedInt(world.harbor_pos[i].neighbors[z][c].distance);
            }
        }
    }
}

void MapSerializer::Deserialize(World& world, SerializedGameData& sgd)
{
    // Alle Weltpunkte
    MapPoint curPos(0, 0);
    for(std::vector<MapNode>::iterator it = world.nodes.begin(); it != world.nodes.end(); ++it)
    {
        it->Deserialize(sgd);
        if(it->harbor_id)
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
    for(unsigned i = 0; i < world.seas.size(); ++i)
    {
        world.seas[i].nodes_count = sgd.PopUnsignedInt();
    }

    // Hafenpositionen serialisieren
    world.harbor_pos.resize(sgd.PopUnsignedInt());
    for(unsigned i = 0; i < world.harbor_pos.size(); ++i)
    {
        world.harbor_pos[i].pos = sgd.PopMapPoint();
        for(unsigned z = 0; z < 6; ++z)
            world.harbor_pos[i].cps[z].sea_id = sgd.PopUnsignedShort();
        for(unsigned z = 0; z < 6; ++z)
        {
            world.harbor_pos[i].neighbors[z].resize(sgd.PopUnsignedInt());
            for(unsigned c = 0; c < world.harbor_pos[i].neighbors[z].size(); ++c)
            {
                world.harbor_pos[i].neighbors[z][c].id = sgd.PopUnsignedInt();
                world.harbor_pos[i].neighbors[z][c].distance = sgd.PopUnsignedInt();
            }
        }
    }
}