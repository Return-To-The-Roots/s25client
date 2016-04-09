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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "defines.h" // IWYU pragma: keep
#include "GameWorld.h"
#include "Loader.h"
#include "GameClient.h"
#include "world/MapLoader.h"
#include "world/MapSerializer.h"
#include "lua/LuaInterfaceGame.h"
#include "SerializedGameData.h"
#include "ogl/glArchivItem_Map.h"
#include "ogl/glArchivItem_Sound.h"
#include "buildings/noBuildingSite.h"
#include "WindowManager.h"

#include "libsiedler2/src/prototypen.h"
#include "luaIncludes.h"
#include <boost/filesystem.hpp>

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

/// Lädt eine Karte
bool GameWorld::LoadMap(const std::string& mapFilePath, const std::string& luaFilePath)
{
    // Map laden
    libsiedler2::ArchivInfo mapArchiv;

    // Karteninformationen laden
    if(libsiedler2::loader::LoadMAP(mapFilePath, mapArchiv) != 0)
        return false;

    const glArchivItem_Map& map = *dynamic_cast<glArchivItem_Map*>(mapArchiv.get(0));

    if (bfs::exists(luaFilePath))
    {
        lua.reset(new LuaInterfaceGame(*this));
        if(!lua->LoadScript(luaFilePath))
            lua.reset();
    }

    MapLoader loader(*this);

    loader.Load(map);

    CreateTradeGraphs();

    tr.GenerateOpenGL(*this);

    return true;
}

void GameWorld::Serialize(SerializedGameData& sgd) const
{
    // Headinformationen
    sgd.PushUnsignedShort(GetWidth());
    sgd.PushUnsignedShort(GetHeight());
    sgd.PushUnsignedChar(static_cast<unsigned char>(GetLandscapeType()));

    // Obj-ID-Counter reinschreiben
    sgd.PushUnsignedInt(GameObject::GetObjIDCounter());

    MapSerializer::Serialize(*this, sgd);

    sgd.PushObjectContainer(harbor_building_sites_from_sea, true);

    if(!lua)
        sgd.PushUnsignedInt(0);
    else
    {
        sgd.PushString(lua->GetScript());
        Serializer luaSaveState = lua->Serialize();
        sgd.PushUnsignedInt(0xC0DEBA5E);  // Start Lua identifier
        sgd.PushUnsignedInt(luaSaveState.GetLength());
        sgd.PushRawData(luaSaveState.GetData(), luaSaveState.GetLength());
        sgd.PushUnsignedInt(0xC001C0DE); // End Lua identifier
    }
}

void GameWorld::Deserialize(SerializedGameData& sgd)
{
    // Headinformationen
    const unsigned short width = sgd.PopUnsignedShort();
    const unsigned short height = sgd.PopUnsignedShort();
    const LandscapeType lt = LandscapeType(sgd.PopUnsignedChar());

    // Initialisierungen
    Init(width, height, lt);

    // Obj-ID-Counter setzen
    GameObject::SetObjIDCounter(sgd.PopUnsignedInt());

    MapSerializer::Deserialize(*this, sgd);

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

    // BQ neu berechnen
    for(unsigned y = 0; y < GetHeight(); ++y)
    {
        for(unsigned x = 0; x < GetWidth(); ++x)
        {
            CalcAndSetBQ(MapPoint(x, y), GAMECLIENT.GetPlayerID());
        }
    }

    tr.GenerateOpenGL(*this);
}


void GameWorld::ImportantObjectDestroyed(const MapPoint pt)
{
    WINDOWMANAGER.Close(CreateGUIID(pt));
}

void GameWorld::MilitaryBuildingCaptured(const MapPoint  /*pt*/, const unsigned char player)
{
    if(player == GAMECLIENT.GetPlayerID())
        LOADER.GetSoundN("sound", 110)->Play(255, false);
}
