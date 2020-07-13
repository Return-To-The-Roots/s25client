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

#include "PostManager.h"
#include "PostBox.h"
#include "PostMsg.h"
#include <algorithm>
#include <stdexcept>

PostManager::PostManager() = default;
PostManager::~PostManager() = default;

PostBox& PostManager::AddPostBox(unsigned player)
{
    if(player >= postBoxes.size())
        throw std::out_of_range("Invalid player for new postbox");
    if(GetPostBox(player))
        throw std::logic_error("Postbox already exists");
    postBoxes[player] = std::make_unique<PostBox>();
    return *postBoxes[player];
}

PostBox* PostManager::GetPostBox(unsigned player) const
{
    return (player < postBoxes.size()) ? postBoxes[player].get() : nullptr;
}

void PostManager::RemovePostBox(unsigned player)
{
    postBoxes[player].reset();
}

void PostManager::SendMsg(unsigned player, std::unique_ptr<PostMsg> msg)
{
    PostBox* box = GetPostBox(player);
    if(box)
        box->AddMsg(std::move(msg));
}

void PostManager::SetMissionGoal(unsigned player, const std::string& newGoal)
{
    PostBox* box = GetPostBox(player);
    if(box)
        box->SetCurrentMissionGoal(newGoal);
}
