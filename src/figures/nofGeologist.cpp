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
#include "nofGeologist.h"

#include "gameData/GameConsts.h"
#include "nodeObjs/noFlag.h"
#include "Loader.h"
#include "macros.h"
#include "GameWorld.h"
#include "Random.h"
#include "EventManager.h"
#include "GameClient.h"
#include "nodeObjs/noSign.h"
#include "buildings/nobBaseWarehouse.h"
#include "SoundManager.h"
#include "SerializedGameData.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

nofGeologist::nofGeologist(const MapPoint pos, const unsigned char player, noRoadNode* goal)
    : nofFlagWorker(JOB_GEOLOGIST, pos, player, goal),  signs(0), resAlreadyFound(std::vector<bool>(5))
{
    node_goal.x = 0;
    node_goal.y = 0;
}

void nofGeologist::Serialize_nofGeologist(SerializedGameData* sgd) const
{
    Serialize_nofFlagWorker(sgd);

    sgd->PushUnsignedShort(signs);

    sgd->PushUnsignedInt(available_nodes.size());
    for(std::vector< MapPoint >::const_iterator it = available_nodes.begin(); it != available_nodes.end(); ++it)
    {
        sgd->PushMapPoint(*it);
    }

    sgd->PushMapPoint(node_goal);

    for(unsigned i = 0; i < 5; ++i)
        sgd->PushBool(resAlreadyFound[i]);

}

nofGeologist::nofGeologist(SerializedGameData* sgd, const unsigned obj_id) : nofFlagWorker(sgd, obj_id),
    signs(sgd->PopUnsignedShort()), resAlreadyFound(std::vector<bool>(5))
{
    unsigned available_nodes_count = sgd->PopUnsignedInt();
    for(unsigned i = 0; i < available_nodes_count; ++i)
    {
        MapPoint p = sgd->PopMapPoint();
        available_nodes.push_back(p);
    }

    node_goal = sgd->PopMapPoint();

    for(unsigned i = 0; i < 5; ++i)
        resAlreadyFound[i] = sgd->PopBool();
}

void nofGeologist::Draw(int x, int y)
{
    switch(state)
    {
        default: break;
        case STATE_FIGUREWORK:
        case STATE_GEOLOGIST_GOTONEXTNODE:
        case STATE_GOTOFLAG:
        {
            // normales Laufen zeichnen
            DrawWalkingBobJobs(x, y, JOB_GEOLOGIST);
        } break;
        case STATE_GEOLOGIST_DIG:
        {
            // 1x grab, 1x "spring-grab", 2x grab, 1x "spring-grab", 2x grab, 1x "spring-grab", 4x grab
            unsigned short i = GAMECLIENT.Interpolate(84, current_ev);
            unsigned sound = 0, sound_id;

            if(i < 6)
            {
                LOADER.GetImageN("rom_bobs", 324 + i)->Draw(x, y, 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLORS[gwg->GetPlayer(player)->color]);
                if(i == 4) { sound = 1; sound_id = 0; }
            }
            else if(i < 16)
            {
                LOADER.GetImageN("rom_bobs", 314 + i - 6)->Draw(x, y, 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLORS[gwg->GetPlayer(player)->color]);
                if(i == 14) { sound = 2; sound_id = 1; }
            }
            else if(i < 28)
            {
                LOADER.GetImageN("rom_bobs", 324 + (i - 16) % 6)->Draw(x, y, 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLORS[gwg->GetPlayer(player)->color]);
                if(i == 20) { sound = 1; sound_id = 2; }
                else if(i == 26) { sound = 1; sound_id = 3; }
            }
            else if(i < 38)
            {
                LOADER.GetImageN("rom_bobs", 314 + i - 28)->Draw(x, y, 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLORS[gwg->GetPlayer(player)->color]);
                if(i == 36) { sound = 2; sound_id = 4; }
            }
            else if(i < 50)
            {
                LOADER.GetImageN("rom_bobs", 324 + (i - 38) % 6)->Draw(x, y, 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLORS[gwg->GetPlayer(player)->color]);
                if(i == 42) { sound = 1; sound_id = 5; }
                else if(i == 48) { sound = 1; sound_id = 6; }
            }
            else if(i < 60)
            {
                LOADER.GetImageN("rom_bobs", 314 + i - 50)->Draw(x, y, 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLORS[gwg->GetPlayer(player)->color]);
                if(i == 58) { sound = 2; sound_id = 7; }
            }
            else
            {
                LOADER.GetImageN("rom_bobs", 324 + (i - 60) % 6)->Draw(x, y, 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLORS[gwg->GetPlayer(player)->color]);
                if(i == 64) { sound = 1; sound_id = 8; }
                else if(i == 70) { sound = 1; sound_id = 9; }
                else if(i == 76) { sound = 1; sound_id = 10; }
                else if(i == 82) { sound = 1; sound_id = 11; }
            }

            if(sound)
                SOUNDMANAGER.PlayNOSound((sound == 1) ? 81 : 56, this, sound_id);

        } break;
        case STATE_GEOLOGIST_CHEER:
        {
            unsigned short i = GAMECLIENT.Interpolate(16, current_ev);

            if(i < 7)
            {
                LOADER.GetImageN("rom_bobs", 357 +
                                 i)->Draw(x, y, 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLORS[gwg->GetPlayer(player)->color]);
            }
            else
            {
                unsigned char ids[9] = { 1, 0, 1, 2, 1, 0, 1, 2, 1};
                LOADER.GetImageN("rom_bobs", 361 + ids[i - 7])->Draw(x, y, 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLORS[gwg->GetPlayer(player)->color]);
            }

            if(i == 4)SOUNDMANAGER.PlayNOSound(107, this, 12); //yippy

        } break;
    }

    /*char number[256];
    sprintf(number,"%u",obj_id);
    NormalFont->Draw(x,y,number,0,0xFFFF0000);*/
}


void nofGeologist::GoalReached()
{
    // An der Flagge angekommen

    // Geologe muss nun 15 Schildchen aufstellen

    signs = 15;

    // Für jeden Ressourcentyp nur einmal eine Post-Nachricht schicken
    for(unsigned i = 0; i < 5; ++i)
        resAlreadyFound[i] = false;

    // Umgebung absuchen
    LookForNewNodes();

    // ersten Punkt suchen
    /*dir = GetNextNode();*/
    GoToNextNode();
}

void nofGeologist::Walked()
{
    if(state == STATE_GEOLOGIST_GOTONEXTNODE)
    {
        // Ist mein Zielpunkt überhaupt noch geeignet zum Graben (kann ja mittlerweile auch was drauf gebaut worden sein)
        if(!IsNodeGood(node_goal))
        {
            // alten Punkt wieder freigeben
            gwg->GetNode(node_goal).reserved = false;;
            // wenn nicht, dann zu einem neuen Punkt gehen
            GoToNextNode();
            return;
        }

        // Bin ich am Zielpunkt?
        if(pos == node_goal)
        {
            // anfangen zu graben
            current_ev = em->AddEvent(this, 100, 1);
            state = STATE_GEOLOGIST_DIG;
        }
        else
        {
            // Weg zum nächsten Punkt suchen
            unsigned char dir = gwg->FindHumanPath(pos, node_goal, 20);

            // Wenns keinen gibt
            if(dir == 0xFF)
            {
                // alten Punkt wieder freigeben
                gwg->GetNode(node_goal).reserved = false;
                // dann neuen Punkt suchen
                dir = GetNextNode();
                // falls es keinen gibt, dann zurück zur Flagge gehen und es übernimmt der andere "Walked"-Zweig
                if(dir == 0xFF)
                {
                    state = STATE_GOTOFLAG;
                    Walked();
                    return;
                }
            }
            
            StartWalking(dir);
        }
    }
    else if(state == STATE_GOTOFLAG)
    {
        GoToFlag();
    }
}

void nofGeologist::HandleDerivedEvent(const unsigned int id)
{
    switch(state)
    {
        default:
            break;
        case STATE_GEOLOGIST_DIG:
        {
            // Ressourcen an diesem Punkt untersuchen
            unsigned char resources = gwg->GetNode(pos).resources;


            if((resources >= 0x41 && resources <= 0x47) || (resources >= 0x49 && resources <= 0x4F) ||
                    (resources >= 0x51 && resources <= 0x57) || (resources >= 0x59 && resources <= 0x5F) ||
                    (resources >= 0x21 && resources <= 0x2F))
            {
                // Es wurde was gefunden, erstmal Jubeln
                state = STATE_GEOLOGIST_CHEER;
                current_ev = em->AddEvent(this, 15, 1);
            }
            else
            {
                // leeres Schild hinstecken und ohne Jubel weiterziehen
                SetSign(resources);
                GoToNextNode();
                /// Punkt wieder freigeben
                gwg->GetNode(pos).reserved = false;;
            }

        } break;
        case STATE_GEOLOGIST_CHEER:
        {
            // Schild reinstecken
            SetSign(gwg->GetNode(pos).resources);
            // Und weiterlaufen
            GoToNextNode();
            /// Punkt wieder freigeben
            gwg->GetNode(pos).reserved = false;;
            /// Sounds evtl löschen
            SOUNDMANAGER.WorkingFinished(this);
        } break;
    }
}


bool nofGeologist::IsNodeGood(const MapPoint pt)
{
    // Es dürfen auch keine bestimmten Objekte darauf stehen und auch keine Schilder !!
    if(!gwg->IsNodeForFigures(pt) || gwg->GetNO(pt)->GetGOT() == GOT_SIGN
            || gwg->GetNO(pt)->GetType() == NOP_FLAG || gwg->GetNO(pt)->GetType() == NOP_TREE)
        return false;

    return true;
}

void nofGeologist::LookForNewNodes()
{
    unsigned short max_radius = 15;
    bool found = false;

    for(unsigned short r = 1; r < max_radius; ++r)
    {
        MapPoint test(flag->GetPos());

        // r Punkte rüber
        test.x -= r;

        // r schräg runter bzw hoch
        for(unsigned short i = 0; i < r; ++i)
        {
            TestNode(MapPoint(test.x, test.y + i)); // unten
            if(i) TestNode(MapPoint(test.x, test.y - i)); // oben

            test.x += (test.y & 1);
        }

        // Die obere bzw untere Reihe
        for(unsigned short i = 0; i < r + 1; ++i, ++test.x)
        {
            TestNode(MapPoint(test.x, test.y + r)); // unten
            TestNode(MapPoint(test.x, test.y - r)); // oben
        }

        test.x = flag->GetX() + r;

        // auf der anderen Seite wieder schräg hoch/runter
        for(unsigned short i = 0; i < r; ++i)
        {
            TestNode(MapPoint(test.x, test.y + i)); // unten
            if(i) TestNode(MapPoint(test.x, test.y - i)); // oben

            test.x -= !(test.y & 1);
        }

        // Wenn es in diesem Umkreis welche gibt, dann nur noch 2 Kreise zusätzlich weitergehen
        if(!found && !available_nodes.empty())
        {
            max_radius = std::min(10, r + 3);
            found = true;
        }

    }
}

void nofGeologist::TestNode(const MapPoint pt)
{
    // Prüfen, ob er überhaupt auf der Karte liegt und nicht irgendwo im Nirvana
    if(pt.x < gwg->GetWidth() && pt.y < gwg->GetHeight())
    {
        if(IsNodeGood(pt) && (gwg->FindHumanPath(this->pos, pt, 20)) != 0xFF && !gwg->GetNode(pt).reserved)
        {
            available_nodes.push_back(pt);
        }
    }
}

unsigned char nofGeologist::GetNextNode()
{
    // Überhaupt noch Schilder zum Aufstellen
    if(!signs)
        return 0xFF;
    do
    {
        // Sind überhaupt Punkte verfügbar?
        while(!available_nodes.empty())
        {
            // Dann einen Punkt zufällig auswählen
            int randNode = RANDOM.Rand(__FILE__, __LINE__, obj_id, available_nodes.size());
            node_goal = available_nodes[randNode];
            // und aus der Liste entfernen
            available_nodes.erase(available_nodes.begin() + randNode);
            // Gucken, ob er gut ist und ob man hingehen kann und ob er noch nicht reserviert wurde!
            unsigned char ret_dir;
            if(IsNodeGood(node_goal) && (ret_dir = gwg->FindHumanPath(pos, node_goal, 20)) != 0xFF && !gwg->GetNode(node_goal).reserved)
            {
                // Reservieren
                gwg->GetNode(node_goal).reserved = true;;
                return ret_dir;
            }
        }


        // Nach neuen Punkten sucehn
        LookForNewNodes();
    }
    while(!available_nodes.empty());

    return 0xFF;
}

void nofGeologist::GoToNextNode()
{
    // Wenn es keine Flagge mehr gibt, dann gleich rumirren
    if(!flag)
    {
        StartWandering();
        Wander();
        state = STATE_FIGUREWORK;
        return;
    }

    // ersten Punkt suchen
    unsigned char dir = GetNextNode();

    if(dir != 0xFF)
    {
        // Wenn es einen Punkt gibt, dann hingehen
        state = STATE_GEOLOGIST_GOTONEXTNODE;
        StartWalking(dir);
        --signs;
    }
    else
    {
        // ansonsten zur Flagge zurückgehen
        state = STATE_GOTOFLAG;
        Walked();
    }
}

void nofGeologist::SetSign(const unsigned char resources)
{
    // Bestimmte Objekte können gelöscht werden
    noBase* no = gwg->GetNO(pos);

    if(no->GetType() != NOP_NOTHING && no->GetType() != NOP_ENVIRONMENT)
        return;

    // Zierobjekte löschen
    if(no->GetType() == NOP_ENVIRONMENT)
    {
        no->Destroy();
        delete no;
    }

    // Schildtyp und -häufigkeit herausfinden
    unsigned char type, quantity;

    if(resources >= 0x41 && resources <= 0x47)
    {
        // Kohle
        type = 2;
        quantity = (resources - 0x40) / 3;
    }
    else if(resources >= 0x49 && resources <= 0x4F)
    {
        // Eisen
        type = 0;
        quantity = (resources - 0x48) / 3;
    }
    else if(resources >= 0x51 && resources <= 0x57)
    {
        // Gold
        type = 1;
        quantity = (resources - 0x50) / 3;
    }
    else if(resources >= 0x59 && resources <= 0x5F)
    {
        // Granit
        type = 3;
        quantity = (resources - 0x58) / 3;
    }
    else if(resources >= 0x21 && resources <= 0x27)
    {
        // Wasser
        type = 4;
        quantity = (resources - 0x20) / 3;
    }
    else
    {
        // nichts
        type = 5;
        quantity = 0;
    }

    if (type < 5)
    {
        if (!resAlreadyFound[type] && !IsSignInArea(type))
        {
            if(GAMECLIENT.GetPlayerID() == this->player)
            {
                switch(type)
                {
                    case 0: GAMECLIENT.SendPostMessage(new PostMsgWithLocation(_("Found iron ore"), PMC_GEOLOGIST, pos));
                        break;
                    case 1: GAMECLIENT.SendPostMessage(new PostMsgWithLocation(_("Found gold"), PMC_GEOLOGIST, pos));
                        break;
                    case 2: GAMECLIENT.SendPostMessage(new PostMsgWithLocation(_("Found coal"), PMC_GEOLOGIST, pos));
                        break;
                    case 3: GAMECLIENT.SendPostMessage(new PostMsgWithLocation(_("Found granite"), PMC_GEOLOGIST, pos));
                        break;
                    case 4: GAMECLIENT.SendPostMessage(new PostMsgWithLocation(_("Found water"), PMC_GEOLOGIST, pos));
                        break;
                    default:
                        ;
                }
            }
        }
        resAlreadyFound[type] = true;
    }

    gwg->LUA_EventResourceFound(this->player, pos, type, quantity);

    // Schild setzen
    gwg->SetNO(new noSign(pos, type, quantity), pos);


}

void nofGeologist::LostWork()
{
    flag = 0;

    switch(state)
    {
        default: break;
            // Wenn wir noch hingehen, dann zurückgehen
        case STATE_FIGUREWORK:
        {
            GoHome();
        } break;
        case STATE_GOTOFLAG:
        {
            // dann sofort rumirren, wenn wir zur Flagge gehen
            StartWandering();
            state = STATE_FIGUREWORK;
        } break;
        case STATE_GEOLOGIST_GOTONEXTNODE:
        case STATE_GEOLOGIST_DIG:
        case STATE_GEOLOGIST_CHEER:
        {
            gwg->GetNode(node_goal).reserved = false;;
        } break;
    }
}

bool nofGeologist::IsSignInArea(unsigned char type) const
{
    const unsigned short radius = 7;
    for(MapCoord tx = gwg->GetXA(pos, 0), r = 1; r <= radius; tx = gwg->GetXA(tx, pos.y, 0), ++r)
    {
        MapPoint t2(tx, pos.y);
        for(unsigned i = 2; i < 8; ++i)
        {
            for(MapCoord r2 = 0; r2 < r; t2 = gwg->GetNeighbour(t2,  i % 6), ++r2)
            {
                noSign* sign = 0;
                if ((sign = gwg->GetSpecObj<noSign>(t2)))
                {
                    if (sign->GetSignType() == type)
                        return true;
                }
            }
        }
    }
    return false;
}
