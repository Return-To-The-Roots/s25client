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

#include "rttrDefines.h" // IWYU pragma: keep
#include "nofGeologist.h"

#include "EventManager.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "SoundManager.h"
#include "GlobalGameSettings.h"
#include "lua/LuaInterfaceGame.h"
#include "network/GameClient.h"
#include "notifications/ResourceNote.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "pathfinding/PathConditionHuman.h"
#include "postSystem/PostMsg.h"
#include "random/Random.h"
#include "world/GameWorldGame.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noSign.h"
#include "gameData/GameConsts.h"
#include "addons/Addon.h"

nofGeologist::nofGeologist(const MapPoint pos, const unsigned char player, noRoadNode* goal)
    : nofFlagWorker(JOB_GEOLOGIST, pos, player, goal), signs(0), node_goal(0, 0)
{
    std::fill(resAlreadyFound.begin(), resAlreadyFound.end(), false);
}

void nofGeologist::Serialize_nofGeologist(SerializedGameData& sgd) const
{
    Serialize_nofFlagWorker(sgd);

    sgd.PushUnsignedShort(signs);

    sgd.PushUnsignedInt(available_nodes.size());
    for(std::vector<MapPoint>::const_iterator it = available_nodes.begin(); it != available_nodes.end(); ++it)
    {
        sgd.PushMapPoint(*it);
    }

    sgd.PushMapPoint(node_goal);

    for(unsigned i = 0; i < 5; ++i)
        sgd.PushBool(resAlreadyFound[i]);
}

nofGeologist::nofGeologist(SerializedGameData& sgd, const unsigned obj_id) : nofFlagWorker(sgd, obj_id), signs(sgd.PopUnsignedShort())
{
    unsigned available_nodes_count = sgd.PopUnsignedInt();
    for(unsigned i = 0; i < available_nodes_count; ++i)
    {
        MapPoint p = sgd.PopMapPoint();
        available_nodes.push_back(p);
    }

    node_goal = sgd.PopMapPoint();

    for(unsigned i = 0; i < 5; ++i)
        resAlreadyFound[i] = sgd.PopBool();
}

void nofGeologist::Draw(DrawPoint drawPt)
{
    switch(state)
    {
        default: break;
        case STATE_FIGUREWORK:
        case STATE_GEOLOGIST_GOTONEXTNODE:
        case STATE_GOTOFLAG:
        {
            // normales Laufen zeichnen
            DrawWalkingBobJobs(drawPt, JOB_GEOLOGIST);
        }
        break;
        case STATE_GEOLOGIST_DIG:
        {
            // 1x grab, 1x "spring-grab", 2x grab, 1x "spring-grab", 2x grab, 1x "spring-grab", 4x grab
            unsigned short i = GAMECLIENT.Interpolate(84, current_ev);
            unsigned sound = 0, sound_id;

            if(i < 6)
            {
                LOADER.GetPlayerImage("rom_bobs", 324 + i)->DrawFull(drawPt, COLOR_WHITE, gwg->GetPlayer(player).color);
                if(i == 4)
                {
                    sound = 1;
                    sound_id = 0;
                }
            } else if(i < 16)
            {
                LOADER.GetPlayerImage("rom_bobs", 314 + i - 6)->DrawFull(drawPt, COLOR_WHITE, gwg->GetPlayer(player).color);
                if(i == 14)
                {
                    sound = 2;
                    sound_id = 1;
                }
            } else if(i < 28)
            {
                LOADER.GetPlayerImage("rom_bobs", 324 + (i - 16) % 6)->DrawFull(drawPt, COLOR_WHITE, gwg->GetPlayer(player).color);
                if(i == 20)
                {
                    sound = 1;
                    sound_id = 2;
                } else if(i == 26)
                {
                    sound = 1;
                    sound_id = 3;
                }
            } else if(i < 38)
            {
                LOADER.GetPlayerImage("rom_bobs", 314 + i - 28)->DrawFull(drawPt, COLOR_WHITE, gwg->GetPlayer(player).color);
                if(i == 36)
                {
                    sound = 2;
                    sound_id = 4;
                }
            } else if(i < 50)
            {
                LOADER.GetPlayerImage("rom_bobs", 324 + (i - 38) % 6)->DrawFull(drawPt, COLOR_WHITE, gwg->GetPlayer(player).color);
                if(i == 42)
                {
                    sound = 1;
                    sound_id = 5;
                } else if(i == 48)
                {
                    sound = 1;
                    sound_id = 6;
                }
            } else if(i < 60)
            {
                LOADER.GetPlayerImage("rom_bobs", 314 + i - 50)->DrawFull(drawPt, COLOR_WHITE, gwg->GetPlayer(player).color);
                if(i == 58)
                {
                    sound = 2;
                    sound_id = 7;
                }
            } else
            {
                LOADER.GetPlayerImage("rom_bobs", 324 + (i - 60) % 6)->DrawFull(drawPt, COLOR_WHITE, gwg->GetPlayer(player).color);
                if(i == 64)
                {
                    sound = 1;
                    sound_id = 8;
                } else if(i == 70)
                {
                    sound = 1;
                    sound_id = 9;
                } else if(i == 76)
                {
                    sound = 1;
                    sound_id = 10;
                } else if(i == 82)
                {
                    sound = 1;
                    sound_id = 11;
                }
            }

            if(sound)
                SOUNDMANAGER.PlayNOSound((sound == 1) ? 81 : 56, this, sound_id);
        }
        break;
        case STATE_GEOLOGIST_CHEER:
        {
            unsigned short i = GAMECLIENT.Interpolate(16, current_ev);

            if(i < 7)
            {
                LOADER.GetPlayerImage("rom_bobs", 357 + i)->DrawFull(drawPt, COLOR_WHITE, gwg->GetPlayer(player).color);
            } else
            {
                unsigned char ids[9] = {1, 0, 1, 2, 1, 0, 1, 2, 1};
                LOADER.GetPlayerImage("rom_bobs", 361 + ids[i - 7])->DrawFull(drawPt, COLOR_WHITE, gwg->GetPlayer(player).color);
            }

            if(i == 4)
                SOUNDMANAGER.PlayNOSound(107, this, 12); // yippy
        }
        break;
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
    std::fill(resAlreadyFound.begin(), resAlreadyFound.end(), false);

    // Umgebung absuchen
    LookForNewNodes();

    // ersten Punkt suchen
    GoToNextNode();
}

void nofGeologist::Walked()
{
    if(state == STATE_GEOLOGIST_GOTONEXTNODE)
    {
        // Check if the flag still exists (not destroyed) and the goal node is still available (something could be build there)
        if(!flag || !IsNodeGood(node_goal))
        {
            // alten Punkt wieder freigeben
            gwg->SetReserved(node_goal, false);
            // wenn nicht, dann zu einem neuen Punkt gehen
            GoToNextNode();
            return;
        }

        // Bin ich am Zielpunkt?
        if(pos == node_goal)
        {
            // anfangen zu graben
            current_ev = GetEvMgr().AddEvent(this, 100, 1);
            state = STATE_GEOLOGIST_DIG;
        } else
        {
            // Weg zum nächsten Punkt suchen
            unsigned char dir = gwg->FindHumanPath(pos, node_goal, 20);

            // Wenns keinen gibt
            if(dir == INVALID_DIR)
                GoToNextNode();
            else
                StartWalking(Direction::fromInt(dir));
        }
    } else if(state == STATE_GOTOFLAG)
    {
        GoToFlag();
    }
}

void nofGeologist::HandleDerivedEvent(const unsigned /*id*/)
{
    switch(state)
    {
        default: break;
        case STATE_GEOLOGIST_DIG:
        {
            // Check what is here
            Resource foundRes = gwg->GetNode(pos).resources;
            // We don't care for fish (and most likely never find it)
            if(foundRes.getType() == Resource::Fish)
                foundRes.setType(Resource::Nothing);

            if(foundRes.getAmount() > 0u)
            {
                // Es wurde was gefunden, erstmal Jubeln
                state = STATE_GEOLOGIST_CHEER;
                current_ev = GetEvMgr().AddEvent(this, 15, 1);
            } else
            {
                // leeres Schild hinstecken und ohne Jubel weiterziehen
                SetSign(foundRes);
                /// Punkt wieder freigeben
                gwg->SetReserved(pos, false);
                GoToNextNode();
            }
        }
        break;
        case STATE_GEOLOGIST_CHEER:
        {
            // Schild reinstecken
            SetSign(gwg->GetNode(pos).resources);
            /// Punkt wieder freigeben
            gwg->SetReserved(pos, false);
            /// Sounds evtl löschen
            SOUNDMANAGER.WorkingFinished(this);
            // Und weiterlaufen
            GoToNextNode();
        }
        break;
    }
}

bool nofGeologist::IsNodeGood(const MapPoint pt) const
{
    // Es dürfen auch keine bestimmten Objekte darauf stehen und auch keine Schilder !!
    const noBase& obj = *gwg->GetNO(pt);
    return PathConditionHuman(*gwg).IsNodeOk(pt) && obj.GetGOT() != GOT_SIGN && obj.GetType() != NOP_FLAG && obj.GetType() != NOP_TREE;
}

namespace {
struct GetMapPointWithRadius
{
    typedef std::pair<MapPoint, unsigned> result_type;

    result_type operator()(const MapPoint pt, unsigned r) { return std::make_pair(pt, r); }
};
} // namespace

void nofGeologist::LookForNewNodes()
{
    std::vector<GetMapPointWithRadius::result_type> pts = gwg->GetPointsInRadius(flag->GetPos(), 15, GetMapPointWithRadius());
    unsigned curMaxRadius = 15;
    bool found = false;
    for(std::vector<GetMapPointWithRadius::result_type>::const_iterator it = pts.begin(); it != pts.end(); ++it)
    {
        if(it->second > curMaxRadius)
            break;
        if(IsValidTargetNode(it->first))
        {
            available_nodes.push_back(it->first);
            if(!found)
            {
                found = true;
                // if we found a valid node, look only in other nodes within 2 more "circles"
                curMaxRadius = std::min(10u, it->second + 2);
            }
        }
    }
}

bool nofGeologist::IsValidTargetNode(const MapPoint pt) const
{
    return (IsNodeGood(pt) && !gwg->GetNode(pt).reserved && (pos == pt || gwg->FindHumanPath(pos, pt, 20) != 0xFF));
}

unsigned char nofGeologist::GetNextNode()
{
    // Überhaupt noch Schilder zum Aufstellen
    if(!signs)
    {
        node_goal = MapPoint::Invalid();
        return INVALID_DIR;
    }

    do
    {
        // Sind überhaupt Punkte verfügbar?
        while(!available_nodes.empty())
        {
            // Dann einen Punkt zufällig auswählen
            int randNode = RANDOM.Rand(__FILE__, __LINE__, GetObjId(), available_nodes.size());
            node_goal = available_nodes[randNode];
            // und aus der Liste entfernen
            available_nodes.erase(available_nodes.begin() + randNode);
            // Gucken, ob er gut ist und ob man hingehen kann und ob er noch nicht reserviert wurde!
            if(!IsNodeGood(node_goal) || gwg->GetNode(node_goal).reserved)
                continue;

            unsigned char ret_dir;
            if(pos == node_goal)
                ret_dir = INVALID_DIR;
            else
            {
                ret_dir = gwg->FindHumanPath(pos, node_goal, 20);
                if(ret_dir == INVALID_DIR)
                    continue;
            }
            // Reservieren
            gwg->SetReserved(node_goal, true);
            return ret_dir;
        }

        // Nach neuen Punkten sucehn
        LookForNewNodes();
    } while(!available_nodes.empty());

    node_goal = MapPoint::Invalid();
    return INVALID_DIR;
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

    if(dir != INVALID_DIR)
    {
        // Wenn es einen Punkt gibt, dann hingehen
        state = STATE_GEOLOGIST_GOTONEXTNODE;
        StartWalking(Direction::fromInt(dir));
        --signs;
    } else if(node_goal == pos)
    {
        // Already there
        state = STATE_GEOLOGIST_GOTONEXTNODE;
        --signs;
        Walked();
    } else
    {
        // ansonsten zur Flagge zurückgehen
        state = STATE_GOTOFLAG;
        Walked();
    }
}

void nofGeologist::SetSign(Resource resources)
{
    RTTR_Assert(resources.getType() != Resource::Fish); // Shall never happen

    // Bestimmte Objekte können gelöscht werden
    NodalObjectType noType = gwg->GetNO(pos)->GetType();
    if(noType != NOP_NOTHING && noType != NOP_ENVIRONMENT)
        return;
    gwg->DestroyNO(pos, false);

    // Schild setzen
    gwg->SetNO(pos, new noSign(pos, resources));

    // If nothing found, there is nothing left to do
    if(resources.getAmount() == 0u)
        return;

    if(!resAlreadyFound[resources.getType()] && !IsSignInArea(resources.getType()))
    {
        const char* msg;
        switch(resources.getType())
        {
            case Resource::Iron: msg = _("Found iron ore"); break;
            case Resource::Gold: msg = _("Found gold"); break;
            case Resource::Coal: msg = _("Found coal"); break;
            case Resource::Granite: msg = _("Found granite"); break;
            case Resource::Water: msg = _("Found water"); break;
            default: RTTR_Assert(false); return;
        }

        if (resources.getType() != Resource::Water || gwg->GetGGS().getSelection(AddonId::EXHAUSTIBLE_WELLS) != 1) {
            SendPostMessage(player, new PostMsg(GetEvMgr().GetCurrentGF(), msg, PostCategory::Geologist, pos));
        }

        gwg->GetNotifications().publish(ResourceNote(player, pos, resources));
        if(gwg->HasLua())
            gwg->GetLua().EventResourceFound(this->player, pos, resources.getType(), resources.getAmount());
    }
    resAlreadyFound[resources.getType()] = true;
}

void nofGeologist::LostWork()
{
    flag = NULL;

    switch(state)
    {
        default:
            break;
        // Wenn wir noch hingehen, dann zurückgehen
        case STATE_FIGUREWORK: { GoHome();
        }
        break;
        case STATE_GOTOFLAG:
        {
            // dann sofort rumirren, wenn wir zur Flagge gehen
            StartWandering();
            state = STATE_FIGUREWORK;
        }
        break;
    }
}

struct IsSignOfType
{
    const Resource::Type type;
    const World& gwb;

    IsSignOfType(Resource::Type type, const World& gwb) : type(type), gwb(gwb) {}

    bool operator()(const MapPoint& pt, unsigned /*distance*/)
    {
        const noSign* sign = gwb.GetSpecObj<noSign>(pt);
        return sign && sign->GetSignType() == type;
    }
};

bool nofGeologist::IsSignInArea(Resource::Type type) const
{
    return gwg->CheckPointsInRadius(pos, 7, IsSignOfType(type, *gwg), false);
}
