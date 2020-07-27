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

#include "GameWorld.h"
#include "GlobalGameSettings.h"
#include "SerializedGameData.h"
#include "buildings/noBuildingSite.h"
#include "lua/LuaInterfaceGame.h"
#include "ogl/glArchivItem_Map.h"
#include "world/MapLoader.h"
#include "world/MapSerializer.h"
#include "libsiedler2/prototypen.h"
#include <boost/filesystem.hpp>
#include <mygettext/mygettext.h>

GameWorld::GameWorld(const std::vector<PlayerInfo>& playerInfos, const GlobalGameSettings& gameSettings, EventManager& em)
    : GameWorldGame(playerInfos, gameSettings, em)
{}

/// Lädt eine Karte
bool GameWorld::LoadMap(const std::shared_ptr<Game>& game, ILocalGameState& localgameState, const std::string& mapFilePath,
                        const std::string& luaFilePath)
{
    // Map laden
    libsiedler2::Archiv mapArchiv;

    // Karteninformationen laden
    if(libsiedler2::loader::LoadMAP(mapFilePath, mapArchiv) != 0)
        return false;

    const glArchivItem_Map& map = *static_cast<glArchivItem_Map*>(mapArchiv[0]);

    if(bfs::exists(luaFilePath))
    {
        SetLua(std::make_unique<LuaInterfaceGame>(game, localgameState));
        if(!GetLua().loadScript(luaFilePath) || !GetLua().CheckScriptVersion())
        {
            SetLua(nullptr);
            return false;
        }
    }

    MapLoader loader(*this);
    if(!loader.Load(map, GetGGS().exploration))
        return false;
    if(!loader.PlaceHQs(*this, GetGGS().randomStartPosition))
        return false;

    CreateTradeGraphs();
    return true;
}

void GameWorld::Serialize(SerializedGameData& sgd) const
{
    MapSerializer::Serialize(*this, GetNumPlayers(), sgd);

    sgd.PushObjectContainer(harbor_building_sites_from_sea, true);

    if(!HasLua())
        sgd.PushUnsignedInt(0);
    else
    {
        sgd.PushLongString(GetLua().getScript());
        Serializer luaSaveState;
        try
        {
            if(!GetLua().Serialize(luaSaveState))
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

void GameWorld::Deserialize(const std::shared_ptr<Game>& game, ILocalGameState& localgameState, SerializedGameData& sgd)
{
    MapSerializer::Deserialize(*this, GetNumPlayers(), sgd);

    sgd.PopObjectContainer(harbor_building_sites_from_sea, GOT_BUILDINGSITE);

    std::string luaScript = sgd.PopLongString();
    if(!luaScript.empty())
    {
        if(sgd.PopUnsignedInt() != 0xC0DEBA5E)
            throw SerializedGameData::Error(_("Invalid id for lua data"));
        // If there is a script, there is also save data. Pop that first
        unsigned luaSaveSize = sgd.PopUnsignedInt();
        Serializer luaSaveState;
        sgd.PopRawData(luaSaveState.GetDataWritable(luaSaveSize), luaSaveSize);
        luaSaveState.SetLength(luaSaveSize);
        if(sgd.PopUnsignedInt() != 0xC001C0DE)
            throw SerializedGameData::Error(_("Invalid end-id for lua data"));

        // Now init and load lua
        SetLua(std::make_unique<LuaInterfaceGame>(game, localgameState));
        if(!GetLua().loadScriptString(luaScript))
        {
            SetLua(nullptr);
            throw SerializedGameData::Error(_("Lua script failed to load."));
        }
        if(!GetLua().CheckScriptVersion())
        {
            SetLua(nullptr);
            throw SerializedGameData::Error(_("Wrong version for lua script."));
        }
        try
        {
            if(!GetLua().Deserialize(luaSaveState))
                throw SerializedGameData::Error(_("Failed to load lua state!"));
        } catch(std::exception& e)
        {
            throw SerializedGameData::Error(std::string(_("Failed to load lua state!")) + _("Error: ") + e.what());
        }
    }
}
