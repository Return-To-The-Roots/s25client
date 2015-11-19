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
#include "gameTypes/MapNode.h"
#include "GameClient.h"
#include "SerializedGameData.h"

void MapNode::Serialize(SerializedGameData& sgd) const
{
    for(unsigned z = 0; z < roads.size(); ++z)
    {
        if(roads_real[z])
            sgd.PushUnsignedChar(roads[z]);
        else
            sgd.PushUnsignedChar(0);
    }

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
    for(unsigned z = 0; z < GAMECLIENT.GetPlayerCount(); ++z)
    {
        const MapNode::FoWData& curFoW = fow[z];
        sgd.PushUnsignedChar(static_cast<unsigned char>(curFoW.visibility));
        // Nur im FoW können FOW-Objekte stehen
        if(curFoW.visibility == VIS_FOW)
        {
            sgd.PushUnsignedInt(curFoW.last_update_time);
            sgd.PushFOWObject(curFoW.object);
            for(unsigned r = 0; r < curFoW.roads.size(); ++r)
                sgd.PushUnsignedChar(curFoW.roads[r]);
            sgd.PushUnsignedChar(curFoW.owner);
            for(unsigned b = 0; b < curFoW.boundary_stones.size(); ++b)
                sgd.PushUnsignedChar(curFoW.boundary_stones[b]);
        }
    }
    sgd.PushObject(obj, false);
    sgd.PushObjectContainer(figures, false);
    sgd.PushUnsignedShort(sea_id);
    sgd.PushUnsignedInt(harbor_id);
}

void MapNode::Deserialize(SerializedGameData& sgd)
{
    for(unsigned z = 0; z < roads.size(); ++z)
    {
        roads[z] = sgd.PopUnsignedChar();
        roads_real[z] = roads[z] ? true : false;
    }

    altitude = sgd.PopUnsignedChar();
    shadow = sgd.PopUnsignedChar();
    t1 = TerrainType(sgd.PopUnsignedChar());
    t2 = TerrainType(sgd.PopUnsignedChar());
    resources = sgd.PopUnsignedChar();
    reserved = sgd.PopBool();
    owner = sgd.PopUnsignedChar();
    for(unsigned b = 0; b < boundary_stones.size(); ++b)
        boundary_stones[b] = sgd.PopUnsignedChar();
    bq = BuildingQuality(sgd.PopUnsignedChar());
    for(unsigned z = 0; z < GAMECLIENT.GetPlayerCount(); ++z)
    {
        MapNode::FoWData& curFoW = fow[z];
        curFoW.visibility = Visibility(sgd.PopUnsignedChar());
        // Nur im FoW können FOW-Objekte stehen
        if(curFoW.visibility == VIS_FOW)
        {
            curFoW.last_update_time = sgd.PopUnsignedInt();
            curFoW.object = sgd.PopFOWObject();
            for(unsigned r = 0; r < curFoW.roads.size(); ++r)
                curFoW.roads[r] = sgd.PopUnsignedChar();
            curFoW.owner = sgd.PopUnsignedChar();
            for(unsigned b = 0; b < curFoW.boundary_stones.size(); ++b)
                curFoW.boundary_stones[b] = sgd.PopUnsignedChar();
        }
        else
        {
            curFoW.last_update_time = 0;
            curFoW.object = NULL;
            for(unsigned r = 0; r < curFoW.roads.size(); ++r)
                curFoW.roads[r] = 0;
            curFoW.owner = 0;
            for(unsigned b = 0; b < curFoW.boundary_stones.size(); ++b)
                curFoW.boundary_stones[b] = 0;
        }
    }
    obj = sgd.PopObject<noBase>(GOT_UNKNOWN);
    sgd.PopObjectContainer(figures, GOT_UNKNOWN);
    sea_id = sgd.PopUnsignedShort();
    harbor_id = sgd.PopUnsignedInt();
}