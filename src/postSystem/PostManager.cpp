// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#include "defines.h" // IWYU pragma: keep
#include "PostManager.h"
#include "PostBox.h"
#include "PostMsg.h"
#include <algorithm>
#include <stdexcept>

PostManager::PostManager()
{
    std::fill(postBoxes.begin(), postBoxes.end(), (PostBox*) NULL);
}

PostManager::~PostManager()
{
    for(unsigned i = 0; i < postBoxes.size(); i++)
        delete postBoxes[i];
}

PostBox* PostManager::AddPostBox(unsigned player)
{
    if(player >= postBoxes.size())
        throw std::out_of_range("Invalid player for new postbox");
    if(GetPostBox(player))
        throw std::logic_error("Postbox already exists");
    postBoxes[player] = new PostBox();
    return postBoxes[player];
}

PostBox* PostManager::GetPostBox(unsigned player) const
{
    return (player < postBoxes.size()) ?  postBoxes[player] : NULL;
}

void PostManager::RemovePostBox(unsigned player)
{
    deletePtr(postBoxes[player]);
}

void PostManager::SendMsg(unsigned player, PostMsg* msg)
{
    PostBox* box = GetPostBox(player);
    if(box)
        box->AddMsg(msg);
    else
        deletePtr(msg);
}
