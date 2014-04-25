// $Id: nofBuilder.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "nofBuilder.h"
#include "GameConsts.h"
#include "Loader.h"
#include "GameClient.h"
#include "GameClientPlayer.h"
#include "GameWorld.h"
#include "noBuildingSite.h"
#include "Random.h"

#include "nobShipYard.h"
#include "EventManager.h"
#include "nobMilitary.h"
#include "nobStorehouse.h"
#include "nobHarborBuilding.h"
#include "SoundManager.h"
#include "SerializedGameData.h"
#include "AIEventManager.h"

#include "glSmartBitmap.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


nofBuilder::nofBuilder(const unsigned short x, const unsigned short y, const unsigned char player, noRoadNode* building_site)
    : noFigure(JOB_BUILDER, x, y, player, building_site), state(STATE_FIGUREWORK), building_site(static_cast<noBuildingSite*>(building_site)), building_steps_available(0)
{
    // Sind wir schon an unsere Baustelle gleich hingesetzt worden (bei Häfen)?
    if(building_site)
    {
        if(x == building_site->GetX() && y == building_site->GetY())
            // Dann gleich mit dem Bauprozess beginnen
            GoalReached();
    }
}

void nofBuilder::Serialize_nofBuilder(SerializedGameData* sgd) const
{
    Serialize_noFigure(sgd);

    sgd->PushUnsignedChar(static_cast<unsigned char>(state));
    sgd->PushObject(building_site, true);
    sgd->PushSignedShort(rel_x);
    sgd->PushSignedShort(rel_y);
    sgd->PushSignedShort(next_rel_x);
    sgd->PushSignedShort(next_rel_y);
    sgd->PushUnsignedChar(building_steps_available);
}

nofBuilder::nofBuilder(SerializedGameData* sgd, const unsigned obj_id) : noFigure(sgd, obj_id),
    state(BuilderState(sgd->PopUnsignedChar())),
    building_site(sgd->PopObject<noBuildingSite>(GOT_BUILDINGSITE)),
    rel_x(sgd->PopSignedShort()),
    rel_y(sgd->PopSignedShort()),
    next_rel_x(sgd->PopSignedShort()),
    next_rel_y(sgd->PopSignedShort()),
    building_steps_available(sgd->PopUnsignedChar())
{
}

void nofBuilder::GoalReached()
{

    // Ansonsten an der Baustelle normal anfangen zu arbeiten
    state = STATE_WAITINGFREEWALK;

    // Sind jetzt an der Baustelle
    rel_x = rel_y = 0;

    // Anfangen um die Baustelle herumzulaufen
    StartFreewalk();

}

void nofBuilder::Walked()
{
}

void nofBuilder::AbrogateWorkplace()
{

    if(building_site)
    {
        state = STATE_FIGUREWORK;
        building_site->Abrogate();
        building_site = 0;
    }
}

void nofBuilder::LostWork()
{
    building_site = 0;


    if(state == STATE_FIGUREWORK)
        GoHome();
    else
    {
        // Event abmelden
        em->RemoveEvent(current_ev);

        StartWandering();
        Wander();
        state = STATE_FIGUREWORK;
    }
}

void nofBuilder::HandleDerivedEvent(const unsigned int id)
{
    switch(state)
    {
        case STATE_WAITINGFREEWALK:
        {
            // Platz einnehmen
            rel_x = next_rel_x;
            rel_y = next_rel_y;

            // Ware aufnehmen, falls es eine gibt
            if(ChooseWare())
                state = STATE_BUILDFREEWALK;

            // Weiter drumrumlaufen
            StartFreewalk();
        } break;
        case STATE_BUILDFREEWALK:
        {
            // Platz einnehmen
            rel_x = next_rel_x;
            rel_y = next_rel_y;

            // Gibts noch was zu bauen?
            if(building_steps_available)
            {
                // dann mal schön bauen
                current_ev = em->AddEvent(this, 40, 1);
                state = STATE_BUILD;
            }
            else if(building_site->IsBuildingComplete())
            {
                // fertig mit Bauen!

                // Baustelle abreißen und Gebäude hinsetzen

                // Gebäudetyp merken und das Volk des Gebäudes
                BuildingType building_type = building_site->GetBuildingType();
                Nation building_nation = building_site->GetNation();

                state = STATE_FIGUREWORK;

                // Baustelle abmelden
                gwg->GetPlayer(player)->RemoveBuildingSite(building_site);

                // ggf. Baustellenfenster schließen
                gwg->ImportantObjectDestroyed(building_site->GetX(), building_site->GetY());

                // Baustelle kommt in den Bytehimmel
                gwg->SetNO(NULL, building_site->GetX(), building_site->GetY());
                delete building_site;

                // KI-Event schicken
                GAMECLIENT.SendAIEvent(new AIEvent::Building(AIEvent::BuildingFinished, x, y, building_type), player);

                // Art des Gebäudes unterscheiden (Lagerhäuser,Militär,normal)
                if(building_type == BLD_STOREHOUSE || building_type == BLD_HARBORBUILDING)
                {
                    // Lagerhäuser
                    nobBaseWarehouse* wh;
                    if(building_type == BLD_STOREHOUSE)
                        wh = new nobStorehouse(x, y, player, building_nation);
                    else
                        wh = new nobHarborBuilding(x, y, player, building_nation);


                    gwg->SetNO(wh, x, y);
                    // Bei Häfen zusätzlich der Wirtschaftsverwaltung Bescheid sagen
                    // Achtung: das kann NIOHT in den Konstruktor von nobHarborBuilding!
                    if(wh->GetGOT() == GOT_NOB_HARBORBUILDING)
                        gwg->GetPlayer(player)->AddHarbor(static_cast<nobHarborBuilding*>(wh));

                    // Mich dort gleich einquartieren und nicht erst zurücklaufen
                    wh->AddFigure(this);
                    gwg->RemoveFigure(this, x, y);

                    // Evtl Träger aus dem HQ wieder verwenden
                    gwg->GetPlayer(player)->FindWarehouseForAllRoads();
                    gwg->GetPlayer(player)->FindWarehouseForAllJobs(JOB_HELPER);

                    // Evtl gabs verlorene Waren, die jetzt in das HQ wieder reinkönnen
                    gwg->GetPlayer(player)->FindClientForLostWares();

                    return;

                }
                else if(building_type >= BLD_BARRACKS && building_type <= BLD_FORTRESS)
                    // Militärgebäude
                    gwg->SetNO(new nobMilitary(building_type, x, y, player, building_nation), x, y);
                else if(building_type == BLD_SHIPYARD)
                    // Schiffsbauer kriegt eine Extrawurst
                    gwg->SetNO(new nobShipYard(x, y, player, building_nation), x, y);
                else
                    // normale Gebäude
                    gwg->SetNO(new nobUsual(building_type, x, y, player, building_nation), x, y);


                // Nach Hause laufen bzw. auch rumirren
                rs_pos = 0;
                rs_dir = true;
                cur_rs = gwg->GetSpecObj<noRoadNode>(x, y)->routes[4];
                building_site = 0;

                GoHome();
                StartWalking(4);

            }
            else
            {
                // Brauchen neues Material

                // Ware aufnehmen, falls es eine gibt
                if(!ChooseWare())
                    state = STATE_WAITINGFREEWALK;

                // Weiter drumrumlaufen
                StartFreewalk();
            }

        } break;
        case STATE_BUILD:
        {
            // Sounds abmelden
            SoundManager::inst().WorkingFinished(this);

            // ein Bauschritt weniger, Haus um eins höher
            --building_steps_available;
            ++building_site->build_progress;
            // Fertig mit dem Bauschritt, dann an nächste Position gehen
            state = STATE_BUILDFREEWALK;
            StartFreewalk();
        } break;
        default: break;
    }
}

// Länge, die der Bauarbeiter in einem Free-Walk zurücklegt (in Pixeln)
const short FREEWALK_LENGTH[2] = {22, 11}; // waagerecht
const short FREEWALK_LENGTH_SLANTWISE[2] = {14, 7}; // schräg

void nofBuilder::StartFreewalk()
{
    list<unsigned char> possible_directions;

    unsigned char waiting_walk = ((state == STATE_WAITINGFREEWALK) ? 0 : 1);

    // Wohin kann der Bauarbeiter noch laufen?

    // Nach links
    if(rel_x - FREEWALK_LENGTH[waiting_walk] >= LEFT_MAX)
        possible_directions.push_back(0);
    // Nach rechts
    if(rel_x + FREEWALK_LENGTH[waiting_walk] <= RIGHT_MAX)
        possible_directions.push_back(3);
    // Nach links/oben
    if(rel_x - FREEWALK_LENGTH_SLANTWISE[waiting_walk] >= LEFT_MAX && rel_y - FREEWALK_LENGTH_SLANTWISE[waiting_walk] >= UP_MAX)
        possible_directions.push_back(1);
    // Nach links/unten
    if(rel_x - FREEWALK_LENGTH_SLANTWISE[waiting_walk] >= LEFT_MAX && rel_y + FREEWALK_LENGTH_SLANTWISE[waiting_walk] <= DOWN_MAX)
        possible_directions.push_back(5);
    // Nach rechts/oben
    if(rel_x + FREEWALK_LENGTH_SLANTWISE[waiting_walk] <= RIGHT_MAX && rel_y - FREEWALK_LENGTH_SLANTWISE[waiting_walk] >= UP_MAX)
        possible_directions.push_back(2);
    // Nach rechts/unten
    if(rel_x + FREEWALK_LENGTH_SLANTWISE[waiting_walk] <= RIGHT_MAX && rel_y + FREEWALK_LENGTH_SLANTWISE[waiting_walk] <= DOWN_MAX)
        possible_directions.push_back(4);

    // Zufällige Richtung von diesen auswählen
    dir = *possible_directions[RANDOM.Rand(__FILE__, __LINE__, obj_id, possible_directions.size())];

    // Und dort auch hinlaufen
    current_ev = em->AddEvent(this, (state == STATE_WAITINGFREEWALK) ? 24 : 17, 1);

    // Zukünftigen Platz berechnen
    next_rel_x = rel_x;
    next_rel_y = rel_y;

    switch(dir)
    {
        case 0: next_rel_x -= FREEWALK_LENGTH[waiting_walk]; break;
        case 1: next_rel_x -= FREEWALK_LENGTH_SLANTWISE[waiting_walk]; next_rel_y -= FREEWALK_LENGTH_SLANTWISE[waiting_walk]; break;
        case 2: next_rel_x += FREEWALK_LENGTH_SLANTWISE[waiting_walk]; next_rel_y -= FREEWALK_LENGTH_SLANTWISE[waiting_walk]; break;
        case 3: next_rel_x += FREEWALK_LENGTH[waiting_walk]; break;
        case 4: next_rel_x += FREEWALK_LENGTH_SLANTWISE[waiting_walk]; next_rel_y += FREEWALK_LENGTH_SLANTWISE[waiting_walk]; break;
        case 5: next_rel_x -= FREEWALK_LENGTH_SLANTWISE[waiting_walk]; next_rel_y += FREEWALK_LENGTH_SLANTWISE[waiting_walk]; break;
    }
}


void nofBuilder::Draw(int x, int y)
{
    switch(state)
    {
        case STATE_FIGUREWORK:
        {
            DrawWalkingBobJobs(x, y, JOB_BUILDER);
        } break;
        case STATE_BUILDFREEWALK:
        case STATE_WAITINGFREEWALK:
        {
            // Interpolieren und Door-Point von Baustelle draufaddieren
            x += (GAMECLIENT.Interpolate(rel_x, next_rel_x, current_ev) + building_site->GetDoorPointX());
            y += (GAMECLIENT.Interpolate(rel_y, next_rel_y, current_ev) + building_site->GetDoorPointY());

            Loader::bob_jobs_cache[building_site->GetNation()][JOB_BUILDER][dir][GAMECLIENT.Interpolate(12, current_ev) % 8].draw(x, y, COLOR_WHITE, COLORS[gwg->GetPlayer(player)->color]);
//          LOADER.GetBobN("jobs")->Draw(23,dir,false,GAMECLIENT.Interpolate(12,current_ev)%8,x,y,COLORS[gwg->GetPlayer(player)->color]);
//          DrawShadow(x,y,GAMECLIENT.Interpolate(12,current_ev)%8,dir);

            /*LOADER.GetBobN("jobs")->Draw(23,dir,false,GAMECLIENT.Interpolate((state==STATE_WAITINGFREEWALK)?8:5,current_ev),x,y,COLORS[gwg->GetPlayer(player)->color]);
            DrawShadow(x,y,GAMECLIENT.Interpolate(16,current_ev)%8);*/
        } break;
        case STATE_BUILD:
        {
            unsigned index = GAMECLIENT.Interpolate(28, current_ev);

            // Je nachdem, wie weit der Bauarbeiter links bzw rechts oder in der Mitte steht, so wird auch die Animation angezeigt
            if(rel_x < -5)
            {
                // von links mit Hammer
                if(index < 12 || index > 19)
                {
                    // Bauarbeiter steht
                    LOADER.GetImageN("rom_bobs", 353 + index % 4)->Draw(x + building_site->GetDoorPointX() + rel_x, y + building_site->GetDoorPointY() + rel_y, 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLORS[gwg->GetPlayer(building_site->GetPlayer())->color]);

                    if(index % 4 == 2)
                        SoundManager::inst().PlayNOSound(78, this, index, 160 - rand() % 60);
                }
                else
                {
                    // er kniet
                    LOADER.GetImageN("rom_bobs", 283 + index % 4)->Draw(x + building_site->GetDoorPointX() + rel_x, y + building_site->GetDoorPointY() + rel_y, 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLORS[gwg->GetPlayer(building_site->GetPlayer())->color]);

                    if(index % 4 == 2)
                        SoundManager::inst().PlayNOSound(72, this, index, 160 - rand() % 60);
                }

            }
            else if(rel_x < 5)
            {
                // in der Mitte mit "Händen"
                LOADER.GetImageN("rom_bobs", 287 + (index / 2) % 4)->Draw(x + building_site->GetDoorPointX() + rel_x, y + building_site->GetDoorPointY() + rel_y, 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLORS[gwg->GetPlayer(building_site->GetPlayer())->color]);
            }
            else
            {
                // von rechts mit Hammer
                if(index < 12 || index > 19)
                {
                    // Bauarbeiter steht
                    LOADER.GetImageN("rom_bobs", 279 + index % 4)->Draw(x + building_site->GetDoorPointX() + rel_x, y + building_site->GetDoorPointY() + rel_y, 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLORS[gwg->GetPlayer(building_site->GetPlayer())->color]);

                    if(index % 4 == 2)
                        SoundManager::inst().PlayNOSound(78, this, index, 160 - rand() % 60);
                }
                else
                {
                    // er kniet
                    LOADER.GetImageN("rom_bobs", 283 + index % 4)->Draw(x + building_site->GetDoorPointX() + rel_x, y + building_site->GetDoorPointY() + rel_y, 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLORS[gwg->GetPlayer(building_site->GetPlayer())->color]);

                    if(index % 4 == 2)
                        SoundManager::inst().PlayNOSound(72, this, index, 160 - rand() % 60);
                }
            }

        } break;

    }

    //char number[256];
    //sprintf(number,"%u",obj_id);
    //NormalFont->Draw(x,y,number,0,0xFFFF0000);
}

bool nofBuilder::ChooseWare()
{
    // Brauch ich ein Brett(Rohbau und wenn kein Stein benötigt wird) oder Stein?
    if(building_site->GetBuildProgress(false) < BUILDING_COSTS[building_site->GetNation()][building_site->GetBuildingType()].boards * 8 || !BUILDING_COSTS[building_site->GetNation()][building_site->GetBuildingType()].stones)
    {
        // Brett
        if(building_site->boards)
        {
            // ein Brett weniger liegt da
            --building_site->boards;
            ++building_site->used_boards;
            // wir können 8 Bauschritt ausführen
            building_steps_available = 8;

            return true;
        }
    }
    else
    {
        // Stein
        if(building_site->stones)
        {
            // ein Stein weniger liegt da
            --building_site->stones;
            ++building_site->used_stones;
            // wir können 8 Bauschritt ausführen
            building_steps_available = 8;

            return true;
        }
    }

    return false;
}
