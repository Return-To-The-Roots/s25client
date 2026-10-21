// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <stdexcept>

class Game;
class ILocalGameState;
class MapInfo;
class Replay;

/// The map or Lua script of a game could not be read
struct GameSetupError : std::runtime_error
{
    using std::runtime_error::runtime_error;
};

/// Fill the world of a freshly created game from the savegame or map in mapInfo.
/// Pass the replay a game is loaded from, if any: its map and Lua script are then taken from the data
/// embedded in mapInfo instead of mapInfo.filepath/luaFilepath, and older replays need the world set up
/// the way the version they were recorded with did it.
/// Throws GameSetupError if the map or Lua script could not be read, SerializedGameData::Error if the savegame is
/// corrupt.
void SetupGameWorld(Game& game, const MapInfo& mapInfo, ILocalGameState& localGameState, const Replay* replay);
