// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PostBox.h"
#include "PostMsg.h"
#include "commonDefines.h"
#include <algorithm>

PostBox::PostBox() = default;
PostBox::~PostBox() = default;

void PostBox::addMsg(std::unique_ptr<const PostMsg> msg)
{
    if(numMessages == MAX_MESSAGES)
        deleteMsg(0, false);
    messages[numMessages] = std::move(msg);
    numMessages++;
    if(evNewMsg)
        evNewMsg(*messages[numMessages - 1u], numMessages);
}

bool PostBox::deleteMsg(const PostMsg* msg)
{
    if(!msg)
        return false;
    for(unsigned i = 0; i < numMessages; ++i)
    {
        if(messages[i].get() == msg)
            return deleteMsg(i);
    }
    return false;
}

bool PostBox::deleteMsg(unsigned idx)
{
    return deleteMsg(idx, true);
}

bool PostBox::deleteMsg(unsigned idx, bool notify)
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

void PostBox::clear()
{
    for(unsigned i = numMessages; i > 0; --i)
        deleteMsg(i - 1);
}

const PostMsg* PostBox::getMsg(unsigned idx) const
{
    if(idx >= numMessages)
        return nullptr;
    return messages[idx].get();
}

void PostBox::setCurrentMissionGoal(const std::string& goal)
{
    currentMissionGoal = goal;
}

std::string PostBox::getCurrentMissionGoal() const
{
    return currentMissionGoal;
}
