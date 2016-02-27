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
#ifndef AIEVENTMANAGER_H_INCLUDED
#define AIEVENTMANAGER_H_INCLUDED

#pragma once

#include <queue>
namespace AIEvent { class Base; }

class AIEventManager
{
    public:
        AIEventManager(void);
        ~AIEventManager(void);
        void AddAIEvent(AIEvent::Base* ev) { events.push(ev); }
        AIEvent::Base* GetEvent();
        bool EventAvailable() const { return !events.empty(); }
        unsigned GetEventNum() const { return events.size(); }

    protected:
        std::queue<AIEvent::Base*> events;
};


#endif // !AIEVENTMANAGER_H_INCLUDED
