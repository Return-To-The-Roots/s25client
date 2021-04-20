// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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

void PostManager::SendMsg(unsigned player, std::unique_ptr<PostMsg> msg) const
{
    PostBox* box = GetPostBox(player);
    if(box)
        box->AddMsg(std::move(msg));
}

void PostManager::SetMissionGoal(unsigned player, const std::string& newGoal) const
{
    PostBox* box = GetPostBox(player);
    if(box)
        box->SetCurrentMissionGoal(newGoal);
}
