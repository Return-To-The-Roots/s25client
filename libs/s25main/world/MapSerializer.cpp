// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "world/MapSerializer.h"
#include "CatapultStone.h"
#include "Game.h"
#include "SerializedGameData.h"
#include "buildings/noBuildingSite.h"
#include "helpers/Range.h"
#include "lua/GameDataLoader.h"
#include "world/GameWorldBase.h"
#include "s25util/warningSuppression.h"
#include <mygettext/mygettext.h>

void MapSerializer::Serialize(const GameWorldBase& world, SerializedGameData& sgd)
{
    // Headinformationen
    helpers::pushPoint(sgd, world.GetSize());
    sgd.PushString(world.GetDescription().get(world.GetLandscapeType()).name);

    sgd.PushUnsignedInt(GameObject::GetObjIDCounter());

    // Alle Weltpunkte serialisieren
    const unsigned numPlayers = world.GetNumPlayers();
    for(const auto& node : world.nodes)
    {
        node.Serialize(sgd, numPlayers, world.GetDescription());
    }

    // Katapultsteine serialisieren
    sgd.PushObjectContainer(world.catapult_stones, true);
    // Meeresinformationen serialisieren
    sgd.PushUnsignedInt(world.seas.size());
    for(const auto& sea : world.seas)
    {
        sgd.PushUnsignedInt(sea.nodes_count);
    }
    // Hafenpositionen serialisieren
    sgd.PushUnsignedInt(world.harbor_pos.size());
    for(const auto& curHarborPos : world.harbor_pos)
    {
        helpers::pushPoint(sgd, curHarborPos.pos);
        helpers::pushContainer(sgd, curHarborPos.seaIds);
        for(const auto& curNeighbors : curHarborPos.neighbors)
        {
            sgd.PushUnsignedInt(curNeighbors.size());

            for(const auto& c : curNeighbors)
            {
                sgd.PushUnsignedInt(c.id);
                sgd.PushUnsignedInt(c.distance);
            }
        }
    }

    sgd.PushObjectContainer(world.harbor_building_sites_from_sea, true);

    if(!world.HasLua())
        sgd.PushLongString("");
    else
    {
        sgd.PushLongString(world.GetLua().getScript());
        Serializer luaSaveState;
        try
        {
            if(!world.GetLua().Serialize(luaSaveState))
                throw SerializedGameData::Error(_("Failed to save lua state!"));
        } catch(std::exception& e)
        {
            throw SerializedGameData::Error(std::string(_("Failed to save lua state!")) + _("Error: ") + e.what());
        }
        sgd.PushUnsignedInt(0xC0DEBA5E); // Start Lua identifier
        sgd.PushUnsignedInt(luaSaveState.GetLength());
        sgd.PushRawData(luaSaveState.GetData(), luaSaveState.GetLength());
        sgd.PushUnsignedInt(0xC001C0DE); // End Lua identifier
    }
}

void MapSerializer::Deserialize(GameWorldBase& world, SerializedGameData& sgd, Game& game,
                                ILocalGameState& localgameState)
{
    // Initialisierungen
    GameDataLoader gdLoader(world.GetDescriptionWriteable());
    if(!gdLoader.Load())
        throw SerializedGameData::Error(_("Failed to load game data!"));

    // Headinformationen
    const auto size = helpers::popPoint<MapExtent>(sgd);
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
    const unsigned numPlayers = world.GetNumPlayers();
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
    sgd.PopObjectContainer(world.catapult_stones, GO_Type::Catapultstone);

    // Meeresinformationen deserialisieren
    world.seas.resize(sgd.PopUnsignedInt());
    for(auto& sea : world.seas)
    {
        sea.nodes_count = sgd.PopUnsignedInt();
    }

    // Hafenpositionen serialisieren
    const unsigned numHarborPositions = sgd.PopUnsignedInt();
    world.harbor_pos.clear();
    world.harbor_pos.reserve(numHarborPositions);
    for(const auto i : helpers::range<unsigned>(numHarborPositions))
    {
        RTTR_UNUSED(i);
        world.harbor_pos.emplace_back(sgd.PopMapPoint());
        auto& curHarborPos = world.harbor_pos.back();
        helpers::popContainer(sgd, curHarborPos.seaIds);
        for(auto& neighbor : curHarborPos.neighbors)
        {
            const unsigned numNeighbors = sgd.PopUnsignedInt();
            neighbor.reserve(numNeighbors);
            for(const auto j : helpers::range<unsigned>(numNeighbors))
            {
                RTTR_UNUSED(j);
                const auto id = sgd.PopUnsignedInt();
                const auto distance = sgd.PopUnsignedInt();
                neighbor.emplace_back(id, distance);
            }
        }
    }

    sgd.PopObjectContainer(world.harbor_building_sites_from_sea, GO_Type::Buildingsite);

    const std::string luaScript = sgd.PopLongString();
    if(!luaScript.empty())
    {
        if(sgd.PopUnsignedInt() != 0xC0DEBA5E)
            throw SerializedGameData::Error(_("Invalid id for lua data"));
        // If there is a script, there is also save data. Store reference to that
        const auto luaSaveSize = sgd.PopUnsignedInt();
        Serializer luaSaveState(sgd.PopAndDiscard(luaSaveSize), luaSaveSize);
        if(sgd.PopUnsignedInt() != 0xC001C0DE)
            throw SerializedGameData::Error(_("Invalid end-id for lua data"));

        // Now init and load lua
        auto lua = std::make_unique<LuaInterfaceGame>(game, localgameState);
        if(!lua->loadScriptString(luaScript))
            throw SerializedGameData::Error(_("Lua script failed to load."));
        if(!lua->CheckScriptVersion())
            throw SerializedGameData::Error(_("Wrong version for lua script."));
        try
        {
            if(!lua->Deserialize(luaSaveState))
                throw SerializedGameData::Error(_("Lua load callback returned failure!"));
        } catch(const std::exception& e)
        {
            throw SerializedGameData::Error(std::string(_("Failed to load lua state!")) + _("Error: ") + e.what());
        }
        game.SetLua(std::move(lua));
    }
    world.CreateTradeGraphs();
}
