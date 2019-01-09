// Copyright (c) 2005 - 2018 Settlers Freaks (sf-team at siedler25.org)
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

#include "rttrDefines.h" // IWYU pragma: keep
#include "GameLoader.h"
#include "Game.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "addons/const_addons.h"
#include "desktops/dskGameInterface.h"
#include "helpers/containerUtils.h"
#include <set>

GameLoader::GameLoader(std::shared_ptr<Game> game) : game(game) {}

GameLoader::~GameLoader() {}

void GameLoader::initNations()
{
    load_nations.clear();
    load_nations.resize(NUM_NATS, false);
    for(unsigned i = 0; i < game->world.GetNumPlayers(); ++i)
        load_nations[game->world.GetPlayer(i).nation] = true;
}

void GameLoader::initTextures()
{
    textures.clear();
    std::set<DescIdx<TerrainDesc>> usedTerrains;
    RTTR_FOREACH_PT(MapPoint, game->world.GetSize())
    {
        const MapNode& node = game->world.GetNode(pt);
        usedTerrains.insert(node.t1);
        usedTerrains.insert(node.t2);
    }
    std::set<DescIdx<EdgeDesc>> usedEdges;
    std::set<DescIdx<LandscapeDesc>> usedLandscapes;

    for(DescIdx<TerrainDesc> tIdx : usedTerrains)
    {
        const TerrainDesc& t = game->world.GetDescription().get(tIdx);
        if(!helpers::contains(textures, t.texturePath))
            textures.push_back(t.texturePath);
        usedEdges.insert(t.edgeType);
        usedLandscapes.insert(t.landscape);
    }
    for(DescIdx<EdgeDesc> eIdx : usedEdges)
    {
        if(!eIdx)
            continue;
        const EdgeDesc& e = game->world.GetDescription().get(eIdx);
        if(!helpers::contains(textures, e.texturePath))
            textures.push_back(e.texturePath);
    }
    for(DescIdx<LandscapeDesc> lIdx : usedLandscapes)
    {
        const LandscapeDesc& e = game->world.GetDescription().get(lIdx);
        for(const RoadTextureDesc& r : e.roadTexDesc)
        {
            if(!helpers::contains(textures, r.texturePath))
                textures.push_back(r.texturePath);
        }
    }
}

bool GameLoader::loadTextures()
{
    LOADER.ClearOverrideFolders();
    LOADER.AddOverrideFolder("<RTTR_RTTR>/LSTS/GAME");
    LOADER.AddOverrideFolder("<RTTR_USERDATA>/LSTS/GAME");
    if(game->world.GetGGS().isEnabled(AddonId::CATAPULT_GRAPHICS))
        LOADER.AddAddonFolder(AddonId::CATAPULT_GRAPHICS);

    const LandscapeDesc& lt = game->world.GetDescription().get(game->world.GetLandscapeType());
    if(!LOADER.LoadFilesAtGame(lt.mapGfxPath, lt.isWinter, load_nations) || !LOADER.LoadFiles(textures) || !LOADER.LoadOverrideFiles())
    {
        return false;
    }

    LOADER.fillCaches();
    return true;
}

bool GameLoader::load()
{
    initNations();
    initTextures();
    if(!loadTextures())
        return false;
    return true;
}
