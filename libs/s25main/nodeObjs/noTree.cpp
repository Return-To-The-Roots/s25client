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

#include "noTree.h"

#include "EventManager.h"
#include "FOWObjects.h"
#include "GameInterface.h"
#include "GlobalGameSettings.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "addons/const_addons.h"
#include "network/GameClient.h"
#include "noAnimal.h"
#include "noDisappearingMapEnvObject.h"
#include "ogl/glSmartBitmap.h"
#include "random/Random.h"
#include "world/GameWorldGame.h"
#include <array>

unsigned short noTree::DRAW_COUNTER = 0;

noTree::noTree(const MapPoint pos, const unsigned char type, const unsigned char size)
    : noCoordBase(NOP_TREE, pos), type(type), size(size), event(nullptr), produce_animal_event(nullptr)
{
    // Wenn der Baum klein ist, muss später mal wachsen
    if(!size)
    {
        event = GetEvMgr().AddEvent(this, WAIT_LENGTH);
        state = STATE_GROWING_WAIT;
    } else
        state = STATE_NOTHING;

    // Every nth tree produces animals, but no palm and pineapple trees
    const std::array<unsigned, 6> TREESPERANIMALSPAWN = {20, 13, 10, 6, 4, 2};
    produce_animals =
      (type < 3 || type > 5) && (RANDOM_RAND(GetObjId(), TREESPERANIMALSPAWN[gwg->GetGGS().getSelection(AddonId::MORE_ANIMALS)]) == 0);

    // Falls das der Fall ist, dann wollen wir doch gleich mal eins produzieren
    if(produce_animals)
        produce_animal_event = GetEvMgr().AddEvent(this, 6000 + RANDOM.Rand(__FILE__, __LINE__, GetObjId(), 2000), 3);
}

noTree::~noTree() = default;

void noTree::Destroy_noTree()
{
    GetEvMgr().RemoveEvent(produce_animal_event);
    Destroy_noCoordBase();
}

void noTree::Serialize_noTree(SerializedGameData& sgd) const
{
    Serialize_noCoordBase(sgd);

    sgd.PushUnsignedChar(type);
    sgd.PushUnsignedChar(size);
    sgd.PushUnsignedChar(static_cast<unsigned char>(state));
    sgd.PushEvent(event);
    sgd.PushEvent(produce_animal_event);
    sgd.PushBool(produce_animals);
}

noTree::noTree(SerializedGameData& sgd, const unsigned obj_id)
    : noCoordBase(sgd, obj_id), type(sgd.PopUnsignedChar()), size(sgd.PopUnsignedChar()), state(State(sgd.PopUnsignedChar())),
      event(sgd.PopEvent()), produce_animal_event(sgd.PopEvent()), produce_animals(sgd.PopBool())
{}

void noTree::Draw(DrawPoint drawPt)
{
    switch(state)
    {
        case STATE_NOTHING:
        case STATE_FALLING_WAIT:
        {
            // Wenn er ausgewachsen ist, dann animiert zeichnen
            LOADER.tree_cache[type][GAMECLIENT.GetGlobalAnimation(8, 7 - GetX() % 2, 3 + GetY() % 3, GetX() * GetY() * 10 * type)].draw(
              drawPt);

            // je mehr Bäume gezeichnet, desto mehr Vogelgezwitscher
            ++DRAW_COUNTER;
        }
        break;
        case STATE_GROWING_WAIT:
        {
            // normal zeichnen, wächst nicht
            LOADER.tree_cache[type][8 + size].draw(drawPt);
        }
        break;
        case STATE_GROWING_GROW:
        {
            // alten Baum ausblenden
            unsigned transparency = (GAMECLIENT.Interpolate(0xFF, event)) << 24;

            LOADER.tree_cache[type][8 + size].draw(drawPt, 0xFFFFFFFF - transparency);

            if(size == 2)
            {
                LOADER.tree_cache[type][0].draw(drawPt, transparency | 0xFFFFFF);
            } else
            {
                LOADER.tree_cache[type][8 + size + 1].draw(drawPt, transparency | 0xFFFFFF);
            }
        }
        break;
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

            LOADER.tree_cache[type][11 + i].draw(drawPt);
        }
        break;
        case STATE_FALLING_FALLEN: { LOADER.tree_cache[type][14].draw(drawPt);
        }
        break;
    }
}

void noTree::HandleEvent(const unsigned id)
{
    // Ein Tier-Produzier-Event?
    if(id == 3)
    {
        // Neues Tier erzeugen
        ProduceAnimal();
        // nächstes Event anmelden
        produce_animal_event = GetEvMgr().AddEvent(this, 6000 + RANDOM.Rand(__FILE__, __LINE__, GetObjId(), 2000), 3);

        return;
    }

    switch(state)
    {
        case STATE_GROWING_WAIT:
        {
            // Der Baum hat gewartet, also wächst er jetzt
            event = GetEvMgr().AddEvent(this, GROWING_LENGTH);
            state = STATE_GROWING_GROW;
        }
        break;
        case STATE_GROWING_GROW:
        {
            // Wenn er ausgewachsen ist, dann nicht, ansonsten nochmal ein "Warteevent" anmelden, damit er noch weiter wächst
            if(++size != 3)
            {
                event = GetEvMgr().AddEvent(this, WAIT_LENGTH);
                // Erstmal wieder bis zum nächsten Wachsstumsschub warten
                state = STATE_GROWING_WAIT;
            } else
            {
                // bin nun ausgewachsen
                state = STATE_NOTHING;
                event = nullptr;
            }
        }
        break;
        case STATE_FALLING_WAIT:
        {
            // Jetzt umfallen
            state = STATE_FALLING_FALL;

            event = GetEvMgr().AddEvent(this, 15);
        }
        break;
        case STATE_FALLING_FALL:
        {
            // Baum ist gefallen, nach bestimmer Zeit verschwinden
            state = STATE_FALLING_FALLEN;
            event = GetEvMgr().AddEvent(this, 28);
        }
        break;
        case STATE_FALLING_FALLEN:
        {
            // Baum verschwindet nun und es bleibt ein Baumstumpf zurück
            event = nullptr;
            GetEvMgr().AddToKillList(this);
            gwg->SetNO(pos, new noDisappearingMapEnvObject(pos, 531), true);
            gwg->RecalcBQAroundPoint(pos);

            // Minimap Bescheid geben (Baum gefallen)
            if(gwg->GetGameInterface())
                gwg->GetGameInterface()->GI_UpdateMinimap(pos);
        }
        break;
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
    event = GetEvMgr().AddEvent(this, 105);
    state = STATE_FALLING_WAIT;
}

void noTree::DontFall()
{
    if(state == STATE_FALLING_WAIT)
        GetEvMgr().RemoveEvent(event);
}

void noTree::ProduceAnimal()
{
    // neues Tier erzeugen, zufälliger Typ
    static const std::array<Species, 6> possibleSpecies = {{SPEC_RABBITWHITE, SPEC_RABBITGREY, SPEC_FOX, SPEC_STAG, SPEC_DEER, SPEC_SHEEP}};
    auto* animal = new noAnimal(possibleSpecies[RANDOM.Rand(__FILE__, __LINE__, GetObjId(), possibleSpecies.size())], pos);
    // In die Landschaft setzen
    gwg->AddFigure(pos, animal);
    // Und ihm die Pforten geben..
    animal->StartLiving();
}
