// Copyright (c) 2005 - 2018 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/Nation.h"
#include <memory>
#include <string>
#include <vector>

class Game;
class Loader;

class GameLoader
{
public:
    GameLoader(Loader&, std::shared_ptr<Game> game);
    ~GameLoader();

    // These steps must be called in order
    void initNations();
    void initTextures();
    bool loadTextures();

    /// Execute all steps
    bool load();
    std::shared_ptr<Game> getGame() const { return game; }

private:
    Loader& loader;
    std::shared_ptr<Game> game;
    std::vector<Nation> usedNations;
    std::vector<std::string> textures;
};
