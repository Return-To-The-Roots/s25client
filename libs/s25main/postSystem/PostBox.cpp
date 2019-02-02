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

#include "rttrDefines.h" // IWYU pragma: keep
#include "PostBox.h"
#include "PostMsg.h"
#include <algorithm>

PostBox::PostBox() : numMessages(0)
{
    std::fill(messages.begin(), messages.end(), (PostMsg*)nullptr);
}

PostBox::~PostBox()
{
    for(unsigned i = 0; i < numMessages; i++)
        delete messages[i];
}

void PostBox::AddMsg(const PostMsg* msg)
{
    if(numMessages == MAX_MESSAGES)
        DeleteMsg(0, false);
    messages[numMessages] = msg;
    numMessages++;
    if(evNewMsg)
        evNewMsg(*msg, numMessages);
}

bool PostBox::DeleteMsg(const PostMsg* msg)
{
    if(!msg)
        return false;
    for(unsigned i = 0; i < numMessages; ++i)
    {
        if(messages[i] == msg)
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
    deletePtr(messages[idx]);
    // Shift messages
    for(unsigned i = idx + 1; i < numMessages; i++)
        messages[i - 1] = messages[i];
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
    return messages[idx];
}

void PostBox::SetCurrentMissionGoal(const std::string& goal)
{
    currentMissionGoal = goal;
}

std::string PostBox::GetCurrentMissionGoal() const
{
    return currentMissionGoal;
}
