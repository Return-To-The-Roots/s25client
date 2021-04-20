// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PostBox.h"
#include "PostMsg.h"
#include "commonDefines.h"
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
