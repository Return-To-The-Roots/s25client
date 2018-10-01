// Copyright (c) 2017 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef GameDataLoader_h__
#define GameDataLoader_h__

#include "LuaInterfaceBase.h"

struct WorldDescription;

class GameDataLoader : public LuaInterfaceBase
{
public:
    GameDataLoader(WorldDescription& worldDesc, const std::string& basePath);
    GameDataLoader(WorldDescription& worldDesc);
    ~GameDataLoader() override;

    bool Load();

    static void Register(kaguya::State& state);

private:
    void Include(const std::string& filePath);
    void AddLandscape(const kaguya::LuaTable& data);
    void AddTerrainEdge(const kaguya::LuaTable& data);
    void AddTerrain(const kaguya::LuaTable& data);

    WorldDescription& worldDesc_;
    std::string basePath_, curFile_;
    int curIncludeDepth_;
    bool errorInIncludeFile_;
};

#endif // GameDataLoader_h__
