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

#ifndef LuaWorld_h__
#define LuaWorld_h__

#include <kaguya/kaguya.hpp>

class GameWorldGame;

class LuaWorld
{
    GameWorldGame& gw;
public:
    LuaWorld(GameWorldGame& gw): gw(gw){}
    static void Register(kaguya::State& state);
    bool AddEnvObject(int x, int y, unsigned id, unsigned file);
    bool AddEnvObject2(int x, int y, unsigned id){ return AddEnvObject(x, y, id, 0xFFFF); }
    bool AddStaticObject(int x, int y, unsigned id, unsigned file, unsigned size);
    bool AddStaticObject2(int x, int y, unsigned id, unsigned file = 0xFFFF){ return AddStaticObject(x, y, id, file, 0); }
    bool AddStaticObject3(int x, int y, unsigned id){ return AddStaticObject(x, y, id, 0xFFFF, 0); }
};

#endif // LuaWorld_h__
