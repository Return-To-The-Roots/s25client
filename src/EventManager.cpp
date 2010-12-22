// $Id: EventManager.cpp 6582 2010-07-16 11:23:35Z FloSoft $
//
// Copyright (c) 2005 - 2010 Settlers Freaks (sf-team at siedler25.org)
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


///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
	#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
#endif


EventManager::~EventManager()
{
	for(list<Event*>::iterator it = eis.begin(); it.valid(); ++it)
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
	
	//assert(event->GetObjId() != 1560584 );
	return event;
}

EventManager::EventPointer EventManager::AddEvent(SerializedGameData * sgd, const unsigned obj_id)
{
	Event * event = new Event(sgd,obj_id);
	eis.push_back(event);
	//assert(event->GetObjId() != 1560864 );
	//assert(event->GetObjId() != 1560584 );
	return event;
}

EventManager::EventPointer EventManager::AddEvent(GameObject *obj, const unsigned int gf_length, const unsigned int id, const unsigned gf_elapsed)
{
	// Anfang des Events in die Vergangenheit zurückverlegen
	Event * event = new Event(obj, GAMECLIENT.GetGFNumber()-gf_elapsed, gf_length, id);
	eis.push_back(event);
	//assert(event->GetObjId() != 1560864 );
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
	// Events abfragen
	for(list<Event*>::iterator it = eis.begin(); it.valid(); ++it)
	{
		if((*it)->gf+(*it)->gf_length == GAMECLIENT.GetGFNumber())
		{
			assert((*it)->obj);
			assert((*it)->obj->GetObjId() < GameObject::GetObjIDCounter());


			// normale Events
			Event * go = *it;
			(*it)->obj->HandleEvent((*it)->id);
			eis.erase(&it);
			delete go;
			
 			
		}
		//else
		//{
		//	// Events sind chronologisch georndet --> wenn das schon nicht mehr erreicht wurden, dann der Rest
		//	// erst recht nicht
		//	break;
		//}
	}

	// Kill-List durchgehen und Objekte in den Bytehimmel befördern
	for(list<GameObject*>::iterator it = kill_list.begin(); it.valid(); ++it)
	{
		(*it)->Destroy();
		delete (*it);

		kill_list.erase(&it);
	}
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
}


void EventManager::Serialize(SerializedGameData *sgd) const
{
	// Kill-Liste muss leer sein!
	assert(!kill_list.size());

	list<const Event*> save_events;
	// Nur Events speichern, die noch nicth vorher von anderen Objekten gespeichert wurden!
	for(list<Event*>::const_iterator it = eis.begin(); it.valid(); ++it)
	{
		if(!sgd->GetConstGameObject((*it)->GetObjId()))
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

	//// Die ganzen Events
	//sgd->PopObjectList(eis,GOT_EVENT);

	//for(list<Event*>::iterator a = eis.begin();a.valid();++a)
	//{
	//	unsigned hits = 0;
	//	for(list<Event*>::iterator b = eis.begin();b.valid();++b)
	//	{
	//		if(*a == *b)
	//			++hits;
	//	}
	//	assert(hits == 1);
	//}
}

/// Ist ein Event mit bestimmter id für ein bestimmtes Objekt bereits vorhanden?
bool EventManager::IsEventAcive(const GameObject * const obj, const unsigned id) const
{
	bool hit = true;
	for(list<Event*>::const_iterator it = eis.begin(); it.valid(); ++it)
	{
		if((*it)->id == id && (*it)->obj == obj)
		{
			if(hit)
				return true;
			hit = true;
		}
	}
	
	return false;
}
