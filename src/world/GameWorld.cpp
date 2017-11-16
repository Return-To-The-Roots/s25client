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

#include "rttrDefines.h" // IWYU pragma: keep
#include "GameWorld.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "buildings/noBuildingSite.h"
#include "lua/LuaInterfaceGame.h"
#include "luaIncludes.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Map.h"
#include "world/MapLoader.h"
#include "world/MapSerializer.h"
#include "gameData/BuildingProperties.h"
#include "libsiedler2/prototypen.h"
#include <boost/filesystem.hpp>

GameWorld::GameWorld(const std::vector<PlayerInfo>& playerInfos, const GlobalGameSettings& gameSettings, EventManager& em)
    : GameWorldGame(playerInfos, gameSettings, em)
{}

/// Lädt eine Karte
bool GameWorld::LoadMap(const std::string& mapFilePath, const std::string& luaFilePath)
{
    // Map laden
    libsiedler2::Archiv mapArchiv;

    // Karteninformationen laden
    if(libsiedler2::loader::LoadMAP(mapFilePath, mapArchiv) != 0)
        return false;

    const glArchivItem_Map& map = *static_cast<glArchivItem_Map*>(mapArchiv[0]);

    BuildingProperties::Init();

    if(bfs::exists(luaFilePath))
    {
        lua.reset(new LuaInterfaceGame(*this));
        if(!lua->LoadScript(luaFilePath))
            lua.reset();
    }

    std::vector<Nation> players;
    for(unsigned i = 0; i < GetNumPlayers(); i++)
    {
        GamePlayer& player = GetPlayer(i);
        if(player.isUsed())
            players.push_back(player.nation);
        else
            players.push_back(NAT_INVALID);
    }

    MapLoader loader(*this, players);
    if(!loader.Load(map, GetGGS().exploration))
        return false;
    if(!loader.PlaceHQs(*this, GetGGS().randomStartPosition))
        return false;

    CreateTradeGraphs();
    return true;
}

void GameWorld::Serialize(SerializedGameData& sgd) const
{
    // Headinformationen
    sgd.PushPoint(GetSize());
    sgd.PushUnsignedChar(static_cast<unsigned char>(GetLandscapeType()));

    sgd.PushUnsignedInt(GameObject::GetObjIDCounter());

    MapSerializer::Serialize(*this, GetNumPlayers(), sgd);

    sgd.PushObjectContainer(harbor_building_sites_from_sea, true);

    if(!lua)
        sgd.PushUnsignedInt(0);
    else
    {
        sgd.PushString(lua->GetScript());
        Serializer luaSaveState = lua->Serialize();
        sgd.PushUnsignedInt(0xC0DEBA5E); // Start Lua identifier
        sgd.PushUnsignedInt(luaSaveState.GetLength());
        sgd.PushRawData(luaSaveState.GetData(), luaSaveState.GetLength());
        sgd.PushUnsignedInt(0xC001C0DE); // End Lua identifier
    }
}

void GameWorld::Deserialize(SerializedGameData& sgd)
{
    // Headinformationen
    const MapExtent size = sgd.PopPoint<MapExtent::ElementType>();
    const LandscapeType lt = LandscapeType(sgd.PopUnsignedChar());

    // Initialisierungen
    Init(size, lt);
    GameObject::ResetCounters(sgd.PopUnsignedInt());

    BuildingProperties::Init();

    MapSerializer::Deserialize(*this, GetNumPlayers(), sgd);

    sgd.PopObjectContainer(harbor_building_sites_from_sea, GOT_BUILDINGSITE);

    std::string luaScript = sgd.PopString();
    if(!luaScript.empty())
    {
        if(sgd.PopUnsignedInt() != 0xC0DEBA5E)
            throw SerializedGameData::Error("Invalid id for lua data");
        // If there is a script, there is also save data. Pop that first
        unsigned luaSaveSize = sgd.PopUnsignedInt();
        Serializer luaSaveState;
        sgd.PopRawData(luaSaveState.GetDataWritable(luaSaveSize), luaSaveSize);
        luaSaveState.SetLength(luaSaveSize);
        if(sgd.PopUnsignedInt() != 0xC001C0DE)
            throw SerializedGameData::Error("Invalid end-id for lua data");

        // Now init and load lua
        lua.reset(new LuaInterfaceGame(*this));
        if(!lua->LoadScriptString(luaScript))
            lua.reset();
        else
            lua->Deserialize(luaSaveState);
    }
}
