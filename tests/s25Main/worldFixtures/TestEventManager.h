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
