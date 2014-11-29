// $Id: noTree.cpp 9497 2014-11-29 10:41:59Z marcus $
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
#include "noTree.h"

#include "Loader.h"
#include "GameClient.h"
#include "EventManager.h"
#include "GameWorld.h"
#include "noDisappearingMapEnvObject.h"
#include "noAnimal.h"
#include "Random.h"
#include "SerializedGameData.h"
#include "FOWObjects.h"
#include "GameInterface.h"

#include "glSmartBitmap.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

unsigned noTree::INSTANCE_COUNTER = 0;
unsigned short noTree::DRAW_COUNTER = 0;

noTree::noTree(const unsigned short x, const unsigned short y, const unsigned char type, const unsigned char size)
    : noCoordBase(NOP_TREE, x, y), type(type), size(size), event(0), produce_animal_event(0)
{
    // Wenn der Baum klein ist, muss später mal wachsen
    if(!size)
    {
        event = em->AddEvent(this, WAIT_LENGTH);
        state = STATE_GROWING_WAIT;
    }
    else
        state = STATE_NOTHING;

    // neuer Baum, neue Instanz
    ++INSTANCE_COUNTER;

    // Jeder 20. Baum produziert Tiere, aber keine Palmen und Ananas!
	const unsigned TREESPERANIMALSPAWN[] = {20, 13, 10, 6, 4, 2};
    produce_animals = (type < 3 || type > 5) && (INSTANCE_COUNTER % TREESPERANIMALSPAWN[GAMECLIENT.GetGGS().getSelection(ADDON_MORE_ANIMALS)] == 0);

    // Falls das der Fall ist, dann wollen wir doch gleich mal eins produzieren
    if(produce_animals)
        produce_animal_event = em->AddEvent(this, 6000 + RANDOM.Rand(__FILE__, __LINE__, obj_id, 2000), 3);
}

noTree::~noTree()
{

}

void noTree::Destroy_noTree()
{
    em->RemoveEvent(produce_animal_event);
    Destroy_noCoordBase();
}

void noTree::Serialize_noTree(SerializedGameData* sgd) const
{
    Serialize_noCoordBase(sgd);

    sgd->PushUnsignedChar(type);
    sgd->PushUnsignedChar(size);
    sgd->PushUnsignedChar(static_cast<unsigned char>(state));
    sgd->PushObject(event, true);
    sgd->PushObject(produce_animal_event, true);
    sgd->PushBool(produce_animals);
}

noTree::noTree(SerializedGameData* sgd, const unsigned obj_id) : noCoordBase(sgd, obj_id),
    type(sgd->PopUnsignedChar()),
    size(sgd->PopUnsignedChar()),
    state(State(sgd->PopUnsignedChar())),
    event(sgd->PopObject<EventManager::Event>(GOT_EVENT)),
    produce_animal_event(sgd->PopObject<EventManager::Event>(GOT_EVENT)),
    produce_animals(sgd->PopBool())
{
}


void noTree::Draw( int x,   int y)
{
    switch(state)
    {
        case STATE_NOTHING:
        case STATE_FALLING_WAIT:
        {
            // Wenn er ausgewachsen ist, dann animiert zeichnen
            Loader::tree_cache[type][GAMECLIENT.GetGlobalAnimation(8, 7 - GetX() % 2, 3 + GetY() % 3, GetX()*GetY() * 10 * type)].draw(x, y);

            // je mehr Bäume gezeichnet, desto mehr Vogelgezwitscher
            ++DRAW_COUNTER;
        } break;
        case STATE_GROWING_WAIT:
        {
            // normal zeichnen, wächst nicht
            Loader::tree_cache[type][8 + size].draw(x, y);
        } break;
        case STATE_GROWING_GROW:
        {
            // alten Baum ausblenden
            unsigned transparency = (GAMECLIENT.Interpolate(0xFF, event)) << 24;

            Loader::tree_cache[type][8 + size].draw(x, y, 0xFFFFFFFF - transparency);

            if (size == 2)
            {
                Loader::tree_cache[type][0].draw(x, y, transparency | 0xFFFFFF);
            }
            else
            {
                Loader::tree_cache[type][8 + size + 1].draw(x, y, transparency | 0xFFFFFF);
            }
        } break;
        case STATE_FALLING_FALL:
        {
            // Umfallen beschleunigen --> für erste Frames mehr Zeit
            unsigned short i = GAMECLIENT.Interpolate(9, event);

            if(i < 4)
                i = 0;
            else if(i < 7)
                i = 1;
            else
                i = 2;

            Loader::tree_cache[type][11 + i].draw(x, y);
        } break;
        case STATE_FALLING_FALLEN:
        {
            Loader::tree_cache[type][14].draw(x, y);
        } break;
    }
}

void noTree::HandleEvent(const unsigned int id)
{
    // Ein Tier-Produzier-Event?
    if(id == 3)
    {
        // Neues Tier erzeugen
        ProduceAnimal();
        // nächstes Event anmelden
        produce_animal_event = em->AddEvent(this, 6000 + RANDOM.Rand(__FILE__, __LINE__, obj_id, 2000), 3);

        return;
    }

    switch(state)
    {
        case STATE_GROWING_WAIT:
        {
            // Der Baum hat gewartet, also wächst er jetzt
            event = em->AddEvent(this, GROWING_LENGTH);
            state = STATE_GROWING_GROW;

        } break;
        case STATE_GROWING_GROW:
        {
            // Wenn er ausgewachsen ist, dann nicht, ansonsten nochmal ein "Warteevent" anmelden, damit er noch weiter wächst
            if(++size != 3)
            {
                event = em->AddEvent(this, WAIT_LENGTH);
                // Erstmal wieder bis zum nächsten Wachsstumsschub warten
                state = STATE_GROWING_WAIT;
            }
            else
            {
                // bin nun ausgewachsen
                state = STATE_NOTHING;
                event = 0;
            }

        } break;
        case STATE_FALLING_WAIT:
        {
            // Jetzt umfallen
            state = STATE_FALLING_FALL;

            event = em->AddEvent(this, 15);
        } break;
        case STATE_FALLING_FALL:
        {
            // Baum ist gefallen, nach bestimmer Zeit verschwinden
            state = STATE_FALLING_FALLEN;
            event = em->AddEvent(this, 28);
        } break;
        case STATE_FALLING_FALLEN:
        {
            // Baum verschwindet nun und es bleibt ein Baumstumpf zurück
            event = 0;
            em->AddToKillList(this);
            gwg->SetNO(new noDisappearingMapEnvObject(x, y, 531), x, y);
            gwg->RecalcBQAroundPoint(x, y);

            // Minimap Bescheid geben (Baum gefallen)
			if(gwg->GetGameInterface())
				gwg->GetGameInterface()->GI_UpdateMinimap(x, y);

        } break;
        default: break;
    }

}

FOWObject* noTree::CreateFOWObject() const
{
    return new fowTree(type, size);
}


void noTree::FallSoon()
{
    // Warten bis der Holzfäller fertig ist und der Baum dann umfällt
    event = em->AddEvent(this, 105);
    state = STATE_FALLING_WAIT;
}

void noTree::DontFall()
{
    if(state == STATE_FALLING_WAIT)
    {
        em->RemoveEvent(event);
        event = 0;
    }
}


void noTree::ProduceAnimal()
{
    // neues Tier erzeugen, zufälliger Typ
    Species possible_species[6] =
    {
        SPEC_RABBITWHITE,
        SPEC_RABBITGREY,
        SPEC_FOX,
        SPEC_STAG,
        SPEC_DEER,
        SPEC_SHEEP
    };
    noAnimal* animal = new noAnimal(possible_species[RANDOM.Rand(__FILE__, __LINE__, obj_id, 6)], x, y);
    // In die Landschaft setzen
    gwg->AddFigure(animal, x, y);
    // Und ihm die Pforten geben..
    animal->StartLiving();
}

