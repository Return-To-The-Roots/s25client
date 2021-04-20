// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GameLoader.h"
#include "Game.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "RttrForeachPt.h"
#include "addons/const_addons.h"
#include "files.h"
#include "helpers/containerUtils.h"
#include <set>
#include <utility>

GameLoader::GameLoader(Loader& loader, std::shared_ptr<Game> game) : loader(loader), game(std::move(game)) {}

GameLoader::~GameLoader() = default;

void GameLoader::initNations()
{
    // Use set to filter duplicates
    std::set<Nation> tmpUsedNations;
    for(unsigned i = 0; i < game->world_.GetNumPlayers(); ++i)
        tmpUsedNations.insert(game->world_.GetPlayer(i).nation);
    usedNations.assign(tmpUsedNations.begin(), tmpUsedNations.end());
}

void GameLoader::initTextures()
{
    textures.clear();
    std::set<DescIdx<TerrainDesc>> usedTerrains;
    RTTR_FOREACH_PT(MapPoint, game->world_.GetSize())
    {
        const MapNode& node = game->world_.GetNode(pt);
        usedTerrains.insert(node.t1);
        usedTerrains.insert(node.t2);
    }
    std::set<DescIdx<EdgeDesc>> usedEdges;
    std::set<DescIdx<LandscapeDesc>> usedLandscapes;

    for(DescIdx<TerrainDesc> tIdx : usedTerrains)
    {
        const TerrainDesc& t = game->world_.GetDescription().get(tIdx);
        if(!helpers::contains(textures, t.texturePath))
            textures.push_back(t.texturePath);
        usedEdges.insert(t.edgeType);
        usedLandscapes.insert(t.landscape);
    }
    for(DescIdx<EdgeDesc> eIdx : usedEdges)
    {
        if(!eIdx)
            continue;
        const EdgeDesc& e = game->world_.GetDescription().get(eIdx);
        if(!helpers::contains(textures, e.texturePath))
            textures.push_back(e.texturePath);
    }
    for(DescIdx<LandscapeDesc> lIdx : usedLandscapes)
    {
        const LandscapeDesc& e = game->world_.GetDescription().get(lIdx);
        for(const RoadTextureDesc& r : e.roadTexDesc)
        {
            if(!helpers::contains(textures, r.texturePath))
                textures.push_back(r.texturePath);
        }
    }
}

bool GameLoader::loadTextures()
{
    std::vector<AddonId> enabledAddons;
    for(const auto id : rttrEnum::values<AddonId>)
    {
        if(game->ggs_.isEnabled(id))
            enabledAddons.push_back(id);
    }

    const LandscapeDesc& lt = game->world_.GetDescription().get(game->world_.GetLandscapeType());
    if(!loader.LoadFilesAtGame(lt.mapGfxPath, lt.isWinter, usedNations, enabledAddons) || !loader.LoadFiles(textures))
        return false;

    loader.fillCaches();
    return true;
}

bool GameLoader::load()
{
    initNations();
    initTextures();
    return loadTextures();
}
