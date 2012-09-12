// $Id: EventManager.cpp 8230 2012-09-12 19:44:22Z marcus $
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

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
	#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
#endif


EventManager::~EventManager()
{
	for(std::vector<Event*>::iterator it = eis.begin(); it != eis.end(); ++it)
		delete (*it);

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
EventManager::EventPointer EventManager::AddEvent(GameObject *obj, const unsigned int gf_length, const unsigned int id)
{
	assert(obj);
	assert(gf_length);
	//assert(!IsEventAcive(obj,0));

	// Event eintragen
	Event * event = new Event(obj, GAMECLIENT.GetGFNumber(), gf_length, id);
	eis.push_back(event);
	
	//assert(event->GetObjId() != 1220037 );
	return event;
}

EventManager::EventPointer EventManager::AddEvent(SerializedGameData * sgd, const unsigned obj_id)
{
	Event * event = new Event(sgd,obj_id);
	eis.push_back(event);
	//assert(event->GetObjId() != 1220037 );
	//assert(event->GetObjId() != 1560584 );

	return event;
}

EventManager::EventPointer EventManager::AddEvent(GameObject *obj, const unsigned int gf_length, const unsigned int id, const unsigned gf_elapsed)
{
	assert(gf_length >= gf_elapsed);

	// Anfang des Events in die Vergangenheit zurückverlegen
	Event * event = new Event(obj, GAMECLIENT.GetGFNumber()-gf_elapsed, gf_length, id);
	eis.push_back(event);
	//assert(event->GetObjId() != 1220037 );
	//assert(event->GetObjId() != 1560584 );
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

	// Events abfragen
	for (unsigned cnt = 0; cnt < eis.size();)
	{
		Event *e = eis[cnt];

		if (!e)
		{
			// marked as removed -> remove

			// this should not be a problem, as there is at least one element in eis: e
			eis[cnt] = eis.back();
			eis.pop_back();
		} else if (e->gf_next == gfnr)
		{
			assert(e->obj);
			assert(e->obj->GetObjId() < GameObject::GetObjIDCounter());

			// this should not be a problem, as there is at least one element in eis: e
			eis[cnt] = eis.back();
			eis.pop_back();

			if (e->obj)
			{
				e->obj->HandleEvent(e->id);
			}

			delete e;
		} else
		{
			++cnt;
		}
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

void EventManager::Event::Serialize_Event(SerializedGameData * sgd) const
{
	Serialize_GameObject(sgd);

	sgd->PushObject(obj,false);
	sgd->PushUnsignedInt(gf);
	sgd->PushUnsignedInt(gf_length);
	sgd->PushUnsignedInt(id);
}

EventManager::Event::Event(SerializedGameData * sgd, const unsigned obj_id) : GameObject(sgd,obj_id),
obj(sgd->PopObject<GameObject>(GOT_UNKNOWN)),
gf(sgd->PopUnsignedInt()),
gf_length(sgd->PopUnsignedInt()),
id(sgd->PopUnsignedInt())
{
	gf_next = gf + gf_length;
}


void EventManager::Serialize(SerializedGameData *sgd) const
{
	// Kill-Liste muss leer sein!
	assert(!kill_list.size());

	std::list<const Event*> save_events;
	// Nur Events speichern, die noch nicth vorher von anderen Objekten gespeichert wurden!
	for(std::vector<Event*>::const_iterator it = eis.begin(); it != eis.end(); ++it)
	{
		if ((*it) && !sgd->GetConstGameObject((*it)->GetObjId()))
			save_events.push_back(*it);
	}

	sgd->PushObjectList(save_events,true);
}

void EventManager::Deserialize(SerializedGameData *sgd)
{
	// Events laden
	// Nicht zur Eventliste hinzufügen, da dies ohnehin schon in Create_GameObject geschieht!!
	unsigned size = sgd->PopUnsignedInt();
	// einzelne Objekte
	for(unsigned i = 0;i<size;++i)
		sgd->PopObject<Event>(GOT_EVENT);
}

/// Ist ein Event mit bestimmter id für ein bestimmtes Objekt bereits vorhanden?
bool EventManager::IsEventAcive(const GameObject * const obj, const unsigned id) const
{
	for (std::vector<Event*>::const_iterator it = eis.begin(); it != eis.end(); ++it)
	{
		if ((*it) && ((*it)->id == id) && ((*it)->obj == obj))
		{
			return true;
		}
	}
	
	return false;
}

void EventManager::RemoveEvent(EventPointer ep)
{
	if (ep == NULL)
	{
		return;
	}

	unsigned sz = eis.size();

	for (unsigned cnt = 0; cnt < sz; cnt++)
	{
		if (eis[cnt] == ep)
		{
			// ATTENTION: we only mark this as removed, otherwise this leads to unexpected behaviour!
			eis[cnt] = NULL;

			delete ep;

			// as this event can only occur once in our list, stop here.
			break;
		}
	}
}

