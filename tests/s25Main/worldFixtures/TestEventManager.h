// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "EventManager.h"
#include <limits>

class TestEventManager : public EventManager
{
public:
    TestEventManager(unsigned startGF = 0) : EventManager(startGF) {}
    /// Execute the next event increasing the GF to the events GF
    /// If maxGF is given the event is only executed if its GF is <= maxGF and the new GF will be at most maxGF (even if
    /// no event was executed Returns the number of GFs executed
    unsigned ExecuteNextEvent(unsigned maxGF = std::numeric_limits<unsigned>::max());
    /// Return all events of the given object
    std::vector<const GameEvent*> GetObjEvents(const GameObject& obj) const;
    /// Check if there is already an event of the given id for this object
    bool IsEventActive(const GameObject& obj, unsigned id) const;
    /// Remove the event and add a copy that is executed at the given GF
    const GameEvent* RescheduleEvent(const GameEvent* event, unsigned targetGF);
    std::vector<const GameEvent*> GetEvents() const;
};
