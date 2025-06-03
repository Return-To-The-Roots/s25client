// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "LuaWorld.h"
#include "lua/LuaHelpers.h"
#include "world/GameWorld.h"
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
                              .addFunction("AddAnimal", &LuaWorld::AddAnimal)
                              .addFunction("SetComputerBarrier", &LuaWorld::SetComputerBarrier));
}

static bool isValidObject(unsigned file, unsigned id)
{
    try
    {
        return noStaticObject::getTextures(file, id).bmp != nullptr;
    } catch(const std::runtime_error&)
    {
        return false;
    }
}

bool LuaWorld::AddEnvObject(int x, int y, unsigned id, unsigned file /* = 0xFFFF */)
{
    lua::assertTrue(isValidObject(file, id), "Invalid object (file/id)");

    MapPoint pt = gw.MakeMapPoint(Position(x, y));
    noBase* obj = gw.GetNode(pt).obj;
    if(obj)
    {
        const GO_Type got = obj->GetGOT();
        RTTR_Assert(got != GO_Type::Nothing);
        if(got != GO_Type::Staticobject && got != GO_Type::Envobject)
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
    lua::assertTrue(isValidObject(file, id), "Invalid object (file/id)");

    MapPoint pt = gw.MakeMapPoint(Position(x, y));
    noBase* obj = gw.GetNode(pt).obj;
    if(obj)
    {
        const GO_Type got = obj->GetGOT();
        RTTR_Assert(got != GO_Type::Nothing);
        if(got != GO_Type::Staticobject && got != GO_Type::Envobject)
            return false;
    }

    gw.DestroyNO(pt, false);
    gw.SetNO(pt, new noStaticObject(pt, id, file, size));
    if(size > 1)
        gw.RecalcBQAroundPointBig(pt);
    else if(size == 1)
        gw.RecalcBQAroundPoint(pt);
    return true;
}

void LuaWorld::AddAnimal(int x, int y, lua::SafeEnum<Species> species)
{
    MapPoint pos = gw.MakeMapPoint(Position(x, y));
    gw.AddFigure(pos, std::make_unique<noAnimal>(species, pos)).StartLiving();
}

void LuaWorld::SetComputerBarrier(unsigned radius, unsigned short x, unsigned short y)
{
    gw.SetComputerBarrier({x, y}, radius);
}
