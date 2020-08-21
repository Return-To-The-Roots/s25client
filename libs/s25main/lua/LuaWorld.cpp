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

#include "LuaWorld.h"
#include "lua/LuaHelpers.h"
#include "world/GameWorldGame.h"
#include "nodeObjs/noAnimal.h"
#include "nodeObjs/noEnvObject.h"
#include "nodeObjs/noStaticObject.h"
#include "gameTypes/MapCoordinates.h"
#include "s25util/strAlgos.h"
#include <kaguya/kaguya.hpp>

KAGUYA_MEMBER_FUNCTION_OVERLOADS(AddEnvObjectWrapper, LuaWorld, AddEnvObject, 3, 4)
KAGUYA_MEMBER_FUNCTION_OVERLOADS(AddStaticObjectWrapper, LuaWorld, AddStaticObject, 3, 5)

void LuaWorld::Register(kaguya::State& state)
{
#pragma region ConstDefs
#define ADD_LUA_CONST(name) state["SPEC_" + s25util::toUpper(#name)] = Species::name
    ADD_LUA_CONST(PolarBear);
    ADD_LUA_CONST(RabbitWhite);
    ADD_LUA_CONST(RabbitGrey);
    ADD_LUA_CONST(Fox);
    ADD_LUA_CONST(Stag);
    ADD_LUA_CONST(Deer);
    ADD_LUA_CONST(Duck);
    ADD_LUA_CONST(Sheep);
#undef ADD_LUA_CONST
#pragma endregion ConstDefs

    state["World"].setClass(kaguya::UserdataMetatable<LuaWorld>()
                              .addFunction("AddEnvObject", AddEnvObjectWrapper())
                              .addFunction("AddStaticObject", AddStaticObjectWrapper())
                              .addFunction("AddAnimal", &LuaWorld::AddAnimal));
}

bool LuaWorld::AddEnvObject(int x, int y, unsigned id, unsigned file /* = 0xFFFF */)
{
    MapPoint pt = gw.MakeMapPoint(Position(x, y));
    noBase* obj = gw.GetNode(pt).obj;
    if(obj)
    {
        const GO_Type got = obj->GetGOT();
        RTTR_Assert(got != GOT_NOTHING);
        if(got != GOT_STATICOBJECT && got != GOT_ENVOBJECT)
            return false;
    }

    gw.DestroyNO(pt, false);
    gw.SetNO(pt, new noEnvObject(pt, id, file));
    gw.RecalcBQAroundPoint(pt);
    return true;
}

bool LuaWorld::AddStaticObject(int x, int y, unsigned id, unsigned file /* = 0xFFFF */, unsigned size /* = 1 */)
{
    lua::assertTrue(size <= 2, "Invalid size");

    MapPoint pt = gw.MakeMapPoint(Position(x, y));
    noBase* obj = gw.GetNode(pt).obj;
    if(obj)
    {
        const GO_Type got = obj->GetGOT();
        RTTR_Assert(got != GOT_NOTHING);
        if(got != GOT_STATICOBJECT && got != GOT_ENVOBJECT)
            return false;
    }

    gw.DestroyNO(pt, false);
    gw.SetNO(pt, new noStaticObject(pt, id, file, size));
    gw.RecalcBQAroundPoint(pt);
    return true;
}

void LuaWorld::AddAnimal(int x, int y, lua::SafeEnum<Species> species)
{
    MapPoint pos = gw.MakeMapPoint(Position(x, y));
    auto* animal = new noAnimal(species, pos);
    gw.AddFigure(pos, animal);
    animal->StartLiving();
}
