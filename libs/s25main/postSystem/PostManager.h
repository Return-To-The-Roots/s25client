// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef PostManager_h__
#define PostManager_h__

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
    void SendMsg(unsigned player, std::unique_ptr<PostMsg> msg);
    void SetMissionGoal(unsigned player, const std::string& newGoal);

private:
    std::array<std::unique_ptr<PostBox>, MAX_PLAYERS> postBoxes;
};

#endif // PostManager_h__
