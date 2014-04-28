// $Id: EventManager.cpp 9372 2014-04-28 15:25:24Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#include "main.h"
#include "EventManager.h"

#include "GameWorld.h"
#include "GameClient.h"
#include "SerializedGameData.h"

#include <list>

//#include <execinfo.h>

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


EventManager::~EventManager()
{
    for(std::map<unsigned, std::list<Event*> >::iterator it = eis.begin(); it != eis.end(); ++it)
    {
        for(std::list<Event*>::iterator e_it = it->second.begin(); e_it != it->second.end(); ++e_it)
            delete (*e_it);
    }

    eis.clear();
}

///////////////////////////////////////////////////////////////////////////////
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
    Event* event = new Event(obj, GAMECLIENT.GetGFNumber(), gf_length, id);
    eis[event->gf_next].push_back(event);

    return event;
}

EventManager::EventPointer EventManager::AddEvent(SerializedGameData* sgd, const unsigned obj_id)
{
    Event* event = new Event(sgd, obj_id);
    eis[event->gf_next].push_back(event);

    return event;
}

EventManager::EventPointer EventManager::AddEvent(GameObject* obj, const unsigned int gf_length, const unsigned int id, const unsigned gf_elapsed)
{
    assert(gf_length >= gf_elapsed);

    /*  if (IsEventActive(obj, id))
        {
            LOG.lprintf("EventManager::AddEvent2(): already active: %u %u\n", obj->GetGOT(), id);
        }*/

    // Anfang des Events in die Vergangenheit zurückverlegen
    Event* event = new Event(obj, GAMECLIENT.GetGFNumber() - gf_elapsed, gf_length, id);
    eis[event->gf_next].push_back(event);

    return event;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  führt alle Events des aktuellen GameFrames aus.
 *
 *  @author OLiver
 */
void EventManager::NextGF()
{
    unsigned int gfnr = GAMECLIENT.GetGFNumber();

    assert( (eis.size() ? eis.begin()->first >= gfnr : true) );

    // Events abfragen
    std::map<unsigned, std::list<Event*> >::iterator it = eis.find(gfnr);
    if(it != eis.end())
    {
        for(std::list<Event*>::iterator e_it = it->second.begin(); e_it != it->second.end(); ++e_it)
        {
            Event* e = (*e_it);
            if(e)
            {
                assert(e->obj);
                assert(e->obj->GetObjId() < GameObject::GetObjIDCounter());


                if (e->obj)
                    e->obj->HandleEvent(e->id);

                delete e;
            }
        }
        eis.erase(it);
    }

    // Kill-List durchgehen und Objekte in den Bytehimmel befördern
    for (std::list<GameObject*>::iterator it = kill_list.begin(); it != kill_list.end(); ++it)
    {
        (*it)->Destroy();
        delete (*it);
    }

    kill_list.clear();
}

void EventManager::Event::Destroy(void)
{
}

void EventManager::Event::Serialize_Event(SerializedGameData* sgd) const
{
    Serialize_GameObject(sgd);

    sgd->PushObject(obj, false);
    sgd->PushUnsignedInt(gf);
    sgd->PushUnsignedInt(gf_length);
    sgd->PushUnsignedInt(id);
}

EventManager::Event::Event(SerializedGameData* sgd, const unsigned obj_id) : GameObject(sgd, obj_id),
    obj(sgd->PopObject<GameObject>(GOT_UNKNOWN)),
    gf(sgd->PopUnsignedInt()),
    gf_length(sgd->PopUnsignedInt()),
    id(sgd->PopUnsignedInt())
{
    gf_next = gf + gf_length;
}


void EventManager::Serialize(SerializedGameData* sgd) const
{
    // Kill-Liste muss leer sein!
    assert(!kill_list.size());

    std::list<const Event*> save_events;
    // Nur Events speichern, die noch nicth vorher von anderen Objekten gespeichert wurden!
    for(std::map< unsigned, std::list<Event*> >::const_iterator it = eis.begin(); it != eis.end(); ++it)
    {
        for(std::list<Event*>::const_iterator e_it = it->second.begin(); e_it != it->second.end(); ++e_it)
        {
            if ((*e_it) && !sgd->GetConstGameObject((*e_it)->GetObjId()))
                save_events.push_back(*e_it);
        }
    }

    sgd->PushObjectList(save_events, true);
}

void EventManager::Deserialize(SerializedGameData* sgd)
{
    // Events laden
    // Nicht zur Eventliste hinzufügen, da dies ohnehin schon in Create_GameObject geschieht!!
    unsigned size = sgd->PopUnsignedInt();
    // einzelne Objekte
    for(unsigned i = 0; i < size; ++i)
        sgd->PopObject<Event>(GOT_EVENT);
}

/// Ist ein Event mit bestimmter id für ein bestimmtes Objekt bereits vorhanden?
bool EventManager::IsEventActive(const GameObject* const obj, const unsigned id) const
{
    for(std::map< unsigned, std::list<Event*> >::const_iterator it = eis.begin(); it != eis.end(); ++it)
    {
        for(std::list<Event*>::const_iterator e_it = it->second.begin(); e_it != it->second.end(); ++e_it)
        {
            if ((*e_it) && ((*e_it)->id == id) && ((*e_it)->obj == obj))
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
    for(std::map< unsigned, std::list<Event*> >::iterator it = eis.begin(); it != eis.end(); ++it)
    {
        for(std::list<Event*>::iterator e_it = it->second.begin(); e_it != it->second.end();)
        {
            if(*e_it && (*e_it)->obj == obj)
            {
                e_it = it->second.erase(e_it);
            }
            else
                ++e_it;
        }
        if(it->second.size() == 0)
            eis.erase(it);
    }
}

void EventManager::RemoveEvent(EventPointer ep)
{
    if (ep == NULL)
    {
        return;
    }

    std::map<unsigned, std::list<Event*> >::iterator it = eis.find(ep->gf_next);
    if(it != eis.end())
    {
        std::list<Event*>::iterator e_it = std::find(it->second.begin(), it->second.end(), ep);
        do
        {
            if(e_it == it->second.end())
                break;

            e_it = it->second.erase(e_it);
            if(e_it != it->second.end())
                e_it = std::find(e_it, it->second.end(), ep);
        }
        while(e_it != it->second.end());

        if(it->second.size() == 0)
            eis.erase(it);
    }

    delete ep;
}

