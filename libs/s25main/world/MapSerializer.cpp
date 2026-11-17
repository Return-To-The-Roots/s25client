// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "world/MapSerializer.h"
#include "CatapultStone.h"
#include "Game.h"
#include "SerializedGameData.h"
#include "buildings/noBuildingSite.h"
#include "helpers/IdRange.h"
#include "helpers/Range.h"
#include "lua/GameDataLoader.h"
#include "world/GameWorldBase.h"
#include <boost/container/static_vector.hpp>
#include <mygettext/mygettext.h>

void MapSerializer::serialize(const GameWorldBase& world, SerializedGameData& sgd)
{
    // Headinformationen
    helpers::pushPoint(sgd, world.getSize());
    sgd.PushString(world.getDescription().get(world.getLandscapeType()).name);

    sgd.PushUnsignedInt(GameObject::getObjIDCounter());

    // Alle Weltpunkte serialisieren
    const unsigned numPlayers = world.getNumPlayers();
    for(const auto& node : world.nodes)
    {
        node.serialize(sgd, numPlayers, world.getDescription());
    }

    // Katapultsteine serialisieren
    sgd.pushObjectContainer(world.catapult_stones, true);
    // Meeresinformationen serialisieren
    sgd.PushUnsignedInt(world.seas.size());
    for(const auto& sea : world.seas)
    {
        sgd.PushUnsignedInt(sea.nodes_count);
    }
    // Hafenpositionen serialisieren
    sgd.PushUnsignedInt(world.harborData.size());
    for(const auto& curHarborPos : world.harborData)
    {
        helpers::pushPoint(sgd, curHarborPos.pos);
        helpers::pushContainer(sgd, curHarborPos.seaIds);
        for(const auto& curNeighbors : curHarborPos.neighbors)
        {
            sgd.PushUnsignedInt(curNeighbors.size());

            for(const auto& c : curNeighbors)
            {
                sgd.PushUnsignedInt(c.id.value());
                sgd.PushUnsignedInt(c.sea.value());
                sgd.PushUnsignedInt(c.distance);
            }
        }
    }

    sgd.pushObjectContainer(world.harbor_building_sites_from_sea, true);

    if(!world.hasLua())
        sgd.PushLongString("");
    else
    {
        sgd.PushLongString(world.getLua().getScript());
        Serializer luaSaveState;
        try
        {
            if(!world.getLua().serialize(luaSaveState))
                throw SerializedGameData::Error(_("Failed to save lua state!"));
        } catch(const std::exception& e)
        {
            throw SerializedGameData::Error(std::string(_("Failed to save lua state!")) + _("Error: ") + e.what());
        }
        sgd.PushUnsignedInt(0xC0DEBA5E); // Start Lua identifier
        sgd.PushUnsignedInt(luaSaveState.GetLength());
        sgd.PushRawData(luaSaveState.GetData(), luaSaveState.GetLength());
        sgd.PushUnsignedInt(0xC001C0DE); // End Lua identifier
    }
}

void MapSerializer::deserialize(GameWorldBase& world, SerializedGameData& sgd, Game& game,
                                ILocalGameState& localgameState)
{
    // Initialisierungen
    GameDataLoader gdLoader(world.getDescriptionWriteable());
    if(!gdLoader.load())
        throw SerializedGameData::Error(_("Failed to load game data!"));

    // Headinformationen
    const auto size = helpers::popPoint<MapExtent>(sgd);
    DescIdx<LandscapeDesc> lt;
    if(sgd.getGameDataVersion() < 3)
    {
        uint8_t gfxSet = sgd.PopUnsignedChar();
        lt = world.getDescription().landscapes.find([gfxSet](const LandscapeDesc& l) { return l.s2Id == gfxSet; });
    } else
    {
        std::string sLandscape = sgd.PopString();
        lt = world.getDescription().landscapes.getIndex(sLandscape);
        if(!lt)
            throw SerializedGameData::Error(std::string("Invalid landscape: ") + sLandscape);
    }
    world.init(size, lt);
    GameObject::resetCounters(sgd.PopUnsignedInt());

    std::vector<DescIdx<TerrainDesc>> landscapeTerrains;
    if(sgd.getGameDataVersion() < 3)
    {
        // Assumes the order of the terrain in the description file is the same as in the prior RTTR versions
        landscapeTerrains =
          world.getDescription().terrain.findAll([lt](const TerrainDesc& t) { return t.landscape == lt; });
    }
    // Alle Weltpunkte
    MapPoint curPos(0, 0);
    const unsigned numPlayers = world.getNumPlayers();
    for(auto& node : world.nodes)
    {
        node.deserialize(sgd, numPlayers, world.getDescription(), landscapeTerrains);
        curPos.x++;
        if(curPos.x >= world.getWidth())
        {
            curPos.x = 0;
            curPos.y++;
        }
    }

    sgd.popObjectContainer(world.catapult_stones, GO_Type::Catapultstone);

    world.seas.resize(sgd.PopUnsignedInt());
    for(auto& sea : world.seas)
        sea.nodes_count = sgd.PopUnsignedInt();

    // Deserialize harbor data
    const unsigned numHarborPositions = sgd.PopUnsignedInt();
    world.harborData.clear();
    world.harborData.reserve(numHarborPositions);
    for([[maybe_unused]] const auto i : helpers::range(numHarborPositions))
    {
        world.harborData.emplace_back(sgd.popMapPoint());
        auto& curHarborPos = world.harborData.back();
        helpers::popContainer(sgd, curHarborPos.seaIds);
        for(auto& neighbor : curHarborPos.neighbors)
        {
            const unsigned numNeighbors = sgd.PopUnsignedInt();
            neighbor.reserve(numNeighbors);
            for([[maybe_unused]] const auto j : helpers::range(numNeighbors))
            {
                const auto id = HarborId(sgd.PopUnsignedInt());
                SeaId sea;
                if(sgd.getGameDataVersion() >= 15)
                    sea = SeaId(sgd.PopUnsignedInt());
                const auto distance = sgd.PopUnsignedInt();
                neighbor.emplace_back(id, sea, distance);
            }
        }
    }
    if(sgd.getGameDataVersion() < 13 && !world.harborData.empty())
    {
        // Workaround for save games without increased game data version after introducing the change
        if(!world.harborData.front().pos.isValid())
            world.harborData.erase(world.harborData.begin());
    }
    if(sgd.getGameDataVersion() < 15)
    {
        const auto getSeas = [&world](const HarborId& hbId) {
            boost::container::static_vector<SeaId, helpers::NumEnumValues_v<Direction>> seas;
            for(const auto sea : world.harborData[hbId].seaIds)
            {
                if(sea)
                    seas.push_back(sea);
            }
            return seas;
        };
        // Determine seas for neighbors
        for(const auto startHbId : helpers::idRange<HarborId>(world.getNumHarborPoints()))
        {
            const auto mySeas = getSeas(startHbId);
            for(auto& neighborsPerDir : world.harborData[startHbId].neighbors)
            {
                for(auto& neighbor : neighborsPerDir)
                {
                    // Easy case: Either harbor is only at a single sea, so that must be the one
                    if(mySeas.size() == 1u)
                    {
                        neighbor.sea = mySeas.front();
                        continue;
                    }
                    const auto otherSeas = getSeas(neighbor.id);
                    if(otherSeas.size() == 1u)
                    {
                        neighbor.sea = otherSeas.front();
                        continue;
                    }
                    // Find the sea where this distance matches
                    for(const auto sea : mySeas)
                    {
                        if(!helpers::contains(otherSeas, sea))
                            continue;
                        unsigned len = 0;
                        const auto startPoint = world.getCoastalPoint(startHbId, sea);
                        const auto nbPoint = world.getCoastalPoint(neighbor.id, sea);
                        if(neighbor.distance == 0 && startPoint == nbPoint)
                        {
                            neighbor.sea = sea;
                            break;
                        }
                        if(world.findShipPath(world.getCoastalPoint(startHbId, sea),
                                              world.getCoastalPoint(neighbor.id, sea), neighbor.distance, nullptr, &len)
                           && len == neighbor.distance)
                        {
                            neighbor.sea = sea;
                            break;
                        }
                    }
                    RTTR_Assert(neighbor.sea);
                }
            }
        }
    }

    sgd.popObjectContainer(world.harbor_building_sites_from_sea, GO_Type::Buildingsite);

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
        if(!lua->checkScriptVersion())
            throw SerializedGameData::Error(_("Wrong version for lua script."));
        try
        {
            if(!lua->deserialize(luaSaveState))
                throw SerializedGameData::Error(_("Lua load callback returned failure!"));
        } catch(const std::exception& e)
        {
            throw SerializedGameData::Error(std::string(_("Failed to load lua state!")) + _("Error: ") + e.what());
        }
        game.setLua(std::move(lua));
    }
}
