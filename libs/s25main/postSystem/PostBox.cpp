// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "commonDefines.h"
#include "PostBox.h"
#include "PostMsg.h"
#include <algorithm>

PostBox::PostBox() = default;
PostBox::~PostBox() = default;

void PostBox::AddMsg(std::unique_ptr<const PostMsg> msg)
{
    if(numMessages == MAX_MESSAGES)
        DeleteMsg(0, false);
    messages[numMessages] = std::move(msg);
    numMessages++;
    if(evNewMsg)
        evNewMsg(*messages[numMessages - 1u], numMessages);
}

bool PostBox::DeleteMsg(const PostMsg* msg)
{
    if(!msg)
        return false;
    for(unsigned i = 0; i < numMessages; ++i)
    {
        if(messages[i].get() == msg)
            return DeleteMsg(i);
    }
    return false;
}

bool PostBox::DeleteMsg(unsigned idx)
{
    return DeleteMsg(idx, true);
}

bool PostBox::DeleteMsg(unsigned idx, bool notify)
{
    if(idx >= numMessages)
        return false;
    // Shift messages
    for(unsigned i = idx + 1; i < numMessages; i++)
        messages[i - 1] = std::move(messages[i]);
    numMessages--;
    if(notify && evDelMsg)
        evDelMsg(numMessages);
    return true;
}

void PostBox::Clear()
{
    for(unsigned i = numMessages; i > 0; --i)
        DeleteMsg(i - 1);
}

const PostMsg* PostBox::GetMsg(unsigned idx) const
{
    if(idx >= numMessages)
        return nullptr;
    return messages[idx].get();
}

void PostBox::SetCurrentMissionGoal(const std::string& goal)
{
    currentMissionGoal = goal;
}

std::string PostBox::GetCurrentMissionGoal() const
{
    return currentMissionGoal;
}
