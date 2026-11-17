// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PostManager.h"
#include "PostBox.h"
#include "PostMsg.h"
#include <algorithm>
#include <stdexcept>

PostManager::PostManager() = default;
PostManager::~PostManager() = default;

PostBox& PostManager::addPostBox(unsigned player)
{
    if(player >= postBoxes.size())
        throw std::out_of_range("Invalid player for new postbox");
    if(getPostBox(player))
        throw std::logic_error("Postbox already exists");
    postBoxes[player] = std::make_unique<PostBox>();
    return *postBoxes[player];
}

PostBox* PostManager::getPostBox(unsigned player) const
{
    return (player < postBoxes.size()) ? postBoxes[player].get() : nullptr;
}

void PostManager::removePostBox(unsigned player)
{
    postBoxes[player].reset();
}

void PostManager::sendMsg(unsigned player, std::unique_ptr<PostMsg> msg) const
{
    PostBox* box = getPostBox(player);
    if(box)
        box->addMsg(std::move(msg));
}

void PostManager::setMissionGoal(unsigned player, const std::string& newGoal) const
{
    PostBox* box = getPostBox(player);
    if(box)
        box->setCurrentMissionGoal(newGoal);
}
