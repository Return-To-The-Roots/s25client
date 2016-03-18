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

#include "defines.h" // IWYU pragma: keep
#include "LuaWorld.h"
#include "world/GameWorldGame.h"
#include "nodeObjs/noEnvObject.h"
#include "nodeObjs/noStaticObject.h"
#include "gameTypes/MapTypes.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

void LuaWorld::Register(kaguya::State& state)
{
    state["World"].setClass(kaguya::ClassMetatable<LuaWorld>()
        .addMemberFunction("AddEnvObject", &LuaWorld::AddEnvObject)
        .addMemberFunction("AddEnvObject", &LuaWorld::AddEnvObject2)
        .addMemberFunction("AddStaticObject", &LuaWorld::AddStaticObject)
        .addMemberFunction("AddStaticObject", &LuaWorld::AddStaticObject2)
        .addMemberFunction("AddStaticObject", &LuaWorld::AddStaticObject3)
        );
}

bool LuaWorld::AddEnvObject(int x, int y, unsigned id, unsigned file /* = 0xFFFF */)
{
    MapPoint pt = gw.MakeMapPoint(Point<int>(x, y));
    noBase* obj = gw.GetNode(pt).obj;
    if(obj && (obj->GetGOT() != GOT_NOTHING) && (obj->GetGOT() != GOT_STATICOBJECT) && (obj->GetGOT() != GOT_ENVOBJECT))
        return false;

    gw.DestroyNO(pt, false);
    gw.SetNO(pt, new noEnvObject(pt, id, file));
    gw.RecalcBQAroundPoint(pt);
    return true;
}

bool LuaWorld::AddStaticObject(int x, int y, unsigned id, unsigned file /* = 0xFFFF */, unsigned size /* = 0 */)
{
    if(size > 2)
        throw std::runtime_error("Invalid size");
    
    MapPoint pt = gw.MakeMapPoint(Point<int>(x, y));
    noBase* obj = gw.GetNode(pt).obj;
    if(obj && (obj->GetGOT() != GOT_NOTHING) && (obj->GetGOT() != GOT_STATICOBJECT) && (obj->GetGOT() != GOT_ENVOBJECT))
        return false;

    gw.DestroyNO(pt, false);
    gw.SetNO(pt, new noStaticObject(pt, id, file, size));
    gw.RecalcBQAroundPoint(pt);
    return true;
}
