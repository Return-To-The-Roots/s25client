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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "defines.h"
#include "EventManager.h"

#include "GameClient.h"
#include "SerializedGameData.h"
#include "helpers/containerUtils.h"
#include "helpers/mapTraits.h"
#include "Log.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void EventManager::Event::Serialize_Event(SerializedGameData& sgd) const
{
    Serialize_GameObject(sgd);

    sgd.PushObject(obj, false);
    sgd.PushUnsignedInt(gf);
    sgd.PushUnsignedInt(gf_length);
    sgd.PushUnsignedInt(id);
}

EventManager::Event::Event(SerializedGameData& sgd, const unsigned obj_id) : GameObject(sgd, obj_id),
    obj(sgd.PopObject<GameObject>(GOT_UNKNOWN)),
    gf(sgd.PopUnsignedInt()),
    gf_length(sgd.PopUnsignedInt()),
    gf_next(gf + gf_length),
    id(sgd.PopUnsignedInt())
{
    assert(obj);
}


//////////////////////////////////////////////////////////////////////////

EventManager::~EventManager()
{
    Clear();
}

void EventManager::Clear()
{
    for(EventMap::iterator it = events.begin(); it != events.end(); ++it)
    {
        for(EventList::iterator e_it = it->second.begin(); e_it != it->second.end(); ++e_it)
            delete *e_it;
    }
    events.clear();

    for(GameObjList::iterator it = kill_list.begin(); it != kill_list.end(); ++it)
    {
        GameObject* obj = *it;
        *it = NULL;
        delete obj;
    }
    kill_list.clear();
}

EventManager::EventPointer EventManager::AddEvent(EventPointer event)
{
    // Should be in the future!
    // At the current GF is allowed as long as a std::list is used for the events of one GF because it retains iterator validy even on modify
    assert(event->gf_next >= GAMECLIENT.GetGFNumber());
    assert(!dynamic_cast<EventPointer>(event->obj)); // Why could this ever happen?
    events[event->gf_next].push_back(event);
    return event;
}

/**
 *  fügt ein Event der Eventliste hinzu.
 *
 *  @param[in] obj       Das Objekt
 *  @param[in] gf_length Die GameFrame-Länge
 *  @param[in] id        ID des Events
 *
 *  @author OLiver
 */
EventManager::EventPointer EventManager::AddEvent(GameObject* obj, const unsigned int gf_length, const unsigned int id)
{
    assert(obj);
    assert(gf_length);

    /*  if (IsEventActive(obj, id))
        {
            LOG.lprintf("EventManager::AddEvent1(): already active: %u %u\n", obj->GetGOT(), id);

        }*/

    // Event eintragen
    return AddEvent(new Event(obj, GAMECLIENT.GetGFNumber(), gf_length, id));
}

EventManager::EventPointer EventManager::AddEvent(SerializedGameData& sgd, const unsigned obj_id)
{
    return AddEvent(new Event(sgd, obj_id));
}

EventManager::EventPointer EventManager::AddEvent(GameObject* obj, const unsigned int gf_length, const unsigned int id, const unsigned gf_elapsed)
{
    assert(gf_length >= gf_elapsed);

    /*  if (IsEventActive(obj, id))
        {
            LOG.lprintf("EventManager::AddEvent2(): already active: %u %u\n", obj->GetGOT(), id);
        }*/

    // Anfang des Events in die Vergangenheit zurückverlegen
    assert(GAMECLIENT.GetGFNumber() >= gf_elapsed);
    return AddEvent(new Event(obj, GAMECLIENT.GetGFNumber() - gf_elapsed, gf_length, id));
}

/**
 *  führt alle Events des aktuellen GameFrames aus.
 *
 *  @author OLiver
 */
void EventManager::NextGF()
{
    unsigned int gfnr = GAMECLIENT.GetGFNumber();

    assert(events.empty() || events.begin()->first >= gfnr);

    // Events abfragen
    EventMap::iterator itCurEvents = events.find(gfnr);
    if(itCurEvents != events.end())
    {
        for(EventList::iterator e_it = itCurEvents->second.begin(); e_it != itCurEvents->second.end(); ++e_it)
        {
            Event* ev = (*e_it);
            assert(ev->obj);
            assert(ev->obj->GetObjId() < GameObject::GetObjIDCounter());

            curActiveEvent = ev;
            ev->obj->HandleEvent(ev->id);

            delete ev;
        }
        curActiveEvent = NULL;
        events.erase(itCurEvents);
    }

    // Kill-List durchgehen und Objekte in den Bytehimmel befördern
    for (GameObjList::iterator it = kill_list.begin(); it != kill_list.end(); ++it)
    {
        GameObject* obj = *it;
        *it = NULL;
        obj->Destroy();
        delete obj;
    }

    kill_list.clear();
}

void EventManager::Serialize(SerializedGameData& sgd) const
{
    // Kill-Liste muss leer sein!
    assert(kill_list.empty());

    std::list<const Event*> save_events;
    // Nur Events speichern, die noch nicth vorher von anderen Objekten gespeichert wurden!
    for(EventMap::const_iterator it = events.begin(); it != events.end(); ++it)
    {
        for(EventList::const_iterator e_it = it->second.begin(); e_it != it->second.end(); ++e_it)
        {
            if (!sgd.IsObjectSerialized((*e_it)->GetObjId()))
                save_events.push_back(*e_it);
        }
    }

    sgd.PushObjectContainer(save_events, true);
}

void EventManager::Deserialize(SerializedGameData& sgd)
{
    // Events laden
    // Nicht zur Eventliste hinzufügen, da dies ohnehin schon in Create_GameObject geschieht!!
    unsigned size = sgd.PopUnsignedInt();
    // einzelne Objekte
    for(unsigned i = 0; i < size; ++i)
        sgd.PopObject<Event>(GOT_EVENT);
}

/// Ist ein Event mit bestimmter id für ein bestimmtes Objekt bereits vorhanden?
bool EventManager::IsEventActive(const GameObject* const obj, const unsigned id) const
{
    for(EventMap::const_iterator it = events.begin(); it != events.end(); ++it)
    {
        for(EventList::const_iterator e_it = it->second.begin(); e_it != it->second.end(); ++e_it)
        {
            if ((*e_it)->id == id && (*e_it)->obj == obj)
            {
                return true;
            }
        }
    }

    return false;
}

// only used for debugging purposes
void EventManager::RemoveAllEventsOfObject(GameObject* obj)
{
    for(EventMap::iterator it = events.begin(); it != events.end();)
    {
        EventList& curEvents = it->second;
        for(std::list<Event*>::iterator e_it = curEvents.begin(); e_it != curEvents.end();)
        {
            if((*e_it)->obj == obj)
            {
                e_it = curEvents.erase(e_it);
            }
            else
                ++e_it;
        }
        if(events.empty())
            it = helpers::erase(events, it);
        else
            ++it;
    }
}

bool EventManager::ObjectHasEvents(GameObject* obj)
{
    for(EventMap::iterator it = events.begin(); it != events.end(); ++it)
    {
        EventList& curEvents = it->second;
        for(std::list<Event*>::iterator e_it = curEvents.begin(); e_it != curEvents.end(); ++e_it)
        {
            if((*e_it)->obj == obj)
                return true;
        }
    }
    return false;
}

bool EventManager::ObjectIsInKillList(GameObject* obj)
{
    return helpers::contains(kill_list, obj);
}

void EventManager::RemoveEvent(EventPointer& ep)
{
    if (!ep)
        return;

    if(ep == curActiveEvent)
    {
        assert(false);
        LOG.lprintf("Bug detected: Active event deleted");
        ep = NULL;
        return;
    }

    EventMap::iterator itEventsAtTime = events.find(ep->gf_next);
    if(itEventsAtTime != events.end())
    {
        EventList& eventsAtTime = itEventsAtTime->second;
        EventList::iterator e_it = std::find(eventsAtTime.begin(), eventsAtTime.end(), ep);
        while(e_it != eventsAtTime.end())
        {
            e_it = eventsAtTime.erase(e_it);
            if(e_it != eventsAtTime.end())
                e_it = std::find(e_it, eventsAtTime.end(), ep);
        }

        if(eventsAtTime.empty())
            events.erase(itEventsAtTime);
    }

    delete ep;
    ep = NULL;
}

#include "figures/noFigure.h"

void EventManager::AddToKillList(GameObject* obj)
{
    assert(obj);
    assert(!helpers::contains(kill_list, obj));
    assert(!dynamic_cast<noFigure*>(obj) || static_cast<noFigure*>(obj)->HasNoGoal());
    kill_list.push_back(obj);
}