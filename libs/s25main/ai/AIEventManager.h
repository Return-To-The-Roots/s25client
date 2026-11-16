// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <queue>

namespace AIEvent {
class Base;
}

class AIEventManager
{
public:
    AIEventManager();
    ~AIEventManager();
    void AddAIEvent(std::unique_ptr<AIEvent::Base> ev);
    std::unique_ptr<AIEvent::Base> getEvent();
    bool EventAvailable() const { return !events.empty(); }
    unsigned getEventNum() const { return events.size(); }

protected:
    std::queue<std::unique_ptr<AIEvent::Base>> events;
};
