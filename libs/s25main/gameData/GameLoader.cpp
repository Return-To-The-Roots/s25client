// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "GameLoader.h"
#include "Game.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "RttrForeachPt.h"
#include "addons/const_addons.h"
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
    loader.ClearOverrideFolders();
    loader.AddOverrideFolder("<RTTR_RTTR>/LSTS/GAME");
    loader.AddOverrideFolder("<RTTR_USERDATA>/LSTS/GAME");
    if(game->ggs_.isEnabled(AddonId::CATAPULT_GRAPHICS))
        loader.AddAddonFolder(AddonId::CATAPULT_GRAPHICS);

    const LandscapeDesc& lt = game->world_.GetDescription().get(game->world_.GetLandscapeType());
    if(!loader.LoadFilesAtGame(lt.mapGfxPath, lt.isWinter, usedNations) || !loader.LoadFiles(textures) || !loader.LoadOverrideFiles())
    {
        return false;
    }

    loader.fillCaches();
    return true;
}

bool GameLoader::load()
{
    initNations();
    initTextures();
    return loadTextures();
}
