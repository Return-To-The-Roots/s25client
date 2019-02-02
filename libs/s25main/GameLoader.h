// Copyright (c) 2005 - 2018 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef GAMELOADER_H_INCLUDED
#define GAMELOADER_H_INCLUDED

#pragma once

#include <memory>
#include <string>
#include <vector>

class Game;

class GameLoader
{
public:
    GameLoader(std::shared_ptr<Game> game);
    ~GameLoader();

    // These steps must be called in order
    void initNations();
    void initTextures();
    bool loadTextures();

    /// Execute all steps and return the interface
    bool load();
    const std::shared_ptr<Game>& getGame() const { return game; }

private:
    std::shared_ptr<Game> game;
    std::vector<bool> load_nations;
    std::vector<std::string> textures;
};

#endif // !GAMELOADER_H_INCLUDED
