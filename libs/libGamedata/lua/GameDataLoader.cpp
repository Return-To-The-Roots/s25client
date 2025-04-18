// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GameDataLoader.h"
#include "CheckedLuaTable.h"
#include "RttrConfig.h"
#include "files.h"
#include "helpers/containerUtils.h"
#include "helpers/format.hpp"
#include "gameData/EdgeDesc.h"
#include "gameData/LandscapeDesc.h"
#include "gameData/TerrainDesc.h"
#include "gameData/WorldDescription.h"
#include "s25util/Log.h"
#include <kaguya/kaguya.hpp>
#include <boost/filesystem.hpp>
#include <stdexcept>

namespace bfs = boost::filesystem;

GameDataLoader::GameDataLoader(WorldDescription& worldDesc, const boost::filesystem::path& basePath)
    : worldDesc_(worldDesc), basePath_(basePath.lexically_normal().make_preferred()), curIncludeDepth_(0)
{
    Register(lua);

    lua["rttr"] = this;
    lua["include"] = kaguya::function([this](const std::string& file) { Include(file); });
}

GameDataLoader::GameDataLoader(WorldDescription& worldDesc)
    : GameDataLoader(worldDesc, RTTRCONFIG.ExpandPath(s25::folders::gamedata) / "world")
{}

GameDataLoader::~GameDataLoader() = default;

bool GameDataLoader::Load()
{
    curFile_ = basePath_ / "default.lua";
    curIncludeDepth_ = 0;
    try
    {
        return loadScript(curFile_);
    } catch(const std::exception& e)
    {
        LOG.write("Failed to load game data!\nReason: %1%\nCurrent file being processed: %2%\n") % e.what() % curFile_;
        return false;
    }
}

void GameDataLoader::Register(kaguya::State& state)
{
    state["RTTRGameData"].setClass(kaguya::UserdataMetatable<GameDataLoader, LuaInterfaceBase>()
                                     .addFunction("AddLandscape", &GameDataLoader::AddLandscape)
                                     .addFunction("AddTerrainEdge", &GameDataLoader::AddTerrainEdge)
                                     .addFunction("AddTerrain", &GameDataLoader::AddTerrain));
}

class LuaIncludeError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

void GameDataLoader::Include(const std::string& filepath)
{
    try
    {
        constexpr int maxIncludeDepth = 10;
        // Protect against cycles and stack overflows
        if(curIncludeDepth_ >= maxIncludeDepth)
            throw LuaIncludeError(helpers::format("Maximum include depth of %1% is reached!", maxIncludeDepth));
        const auto isAllowedChar = [](const char c) {
            return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '/'
                   || c == '.';
        };
        if(helpers::contains_if(filepath, [isAllowedChar](const char c) { return !isAllowedChar(c); }))
            throw LuaIncludeError("It contains disallowed chars. Allowed: alpha-numeric, underscore, slash and dot.");
        if(bfs::path(filepath).is_absolute())
            throw LuaIncludeError("Path to file must be relative to current file");
        bfs::path absFilePath = bfs::absolute(filepath, curFile_.parent_path());
        if(!bfs::is_regular_file(absFilePath))
            throw LuaIncludeError("File not found!");

        // Normalize for below check against basePath
        absFilePath = absFilePath.lexically_normal().make_preferred();
        if(absFilePath.extension() != ".lua")
            throw LuaIncludeError("File must have .lua as the extension!");
        if(absFilePath.string().find(basePath_.string()) != 0)
            throw LuaIncludeError("File is outside the lua data directory!");
        const auto oldCurFile = curFile_;
        curFile_ = absFilePath;
        ++curIncludeDepth_;
        if(!loadScript(absFilePath))
            throw std::runtime_error(helpers::format("Include file '%1%' cannot be included", filepath));
        curFile_ = oldCurFile;
        RTTR_Assert(curIncludeDepth_ > 0);
        --curIncludeDepth_;
    } catch(const LuaIncludeError& e)
    {
        throw std::runtime_error(helpers::format("Include file '%1%' cannot be included: %2%", filepath, e.what()));
    }
}

void GameDataLoader::AddLandscape(const kaguya::LuaTable& data)
{
    worldDesc_.landscapes.add(LandscapeDesc(data, worldDesc_));
}

void GameDataLoader::AddTerrainEdge(const kaguya::LuaTable& data)
{
    worldDesc_.edges.add(EdgeDesc(data, worldDesc_));
}

void GameDataLoader::AddTerrain(const kaguya::LuaTable& data)
{
    worldDesc_.terrain.add(TerrainDesc(data, worldDesc_));
}

void loadGameData(WorldDescription& worldDesc)

{
    GameDataLoader gdLoader(worldDesc);
    if(!gdLoader.Load())
        throw std::runtime_error("Failed to load game data");
}
