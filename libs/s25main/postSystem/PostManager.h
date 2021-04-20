// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameData/MaxPlayers.h"
#include <array>
#include <memory>
#include <string>

class PostMsg;
class PostBox;

class PostManager
{
public:
    PostManager();
    ~PostManager();
    PostBox& AddPostBox(unsigned player);
    PostBox* GetPostBox(unsigned player) const;
    void RemovePostBox(unsigned player);
    void SendMsg(unsigned player, std::unique_ptr<PostMsg> msg) const;
    void SetMissionGoal(unsigned player, const std::string& newGoal) const;

private:
    std::array<std::unique_ptr<PostBox>, MAX_PLAYERS> postBoxes;
};
