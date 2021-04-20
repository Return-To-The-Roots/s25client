// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AIEventManager.h"
#include "AIEvents.h"

AIEventManager::AIEventManager() = default;
AIEventManager::~AIEventManager() = default;

void AIEventManager::AddAIEvent(std::unique_ptr<AIEvent::Base> ev)
{
    events.push(std::move(ev));
}

std::unique_ptr<AIEvent::Base> AIEventManager::GetEvent()
{
    if(events.empty())
        return nullptr;

    std::unique_ptr<AIEvent::Base> ev = std::move(events.front());
    events.pop();
    return ev;
}
