// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "nofGeologist.h"
#include "EventManager.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "SoundManager.h"
#include "addons/const_addons.h"
#include "helpers/EnumRange.h"
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

nofGeologist::nofGeologist(const MapPoint pos, const unsigned char player, noRoadNode* goal)
    : nofFlagWorker(Job::Geologist, pos, player, goal), signs(0), node_goal(0, 0)
{
    std::fill(resAlreadyFound.begin(), resAlreadyFound.end(), false);
}

void nofGeologist::Serialize(SerializedGameData& sgd) const
{
    nofFlagWorker::Serialize(sgd);

    sgd.PushUnsignedShort(signs);

    sgd.PushUnsignedInt(available_nodes.size());
    for(const auto& available_node : available_nodes)
    {
        sgd.PushMapPoint(available_node);
    }

    sgd.PushMapPoint(node_goal);

    for(const bool found : resAlreadyFound)
        sgd.PushBool(found);
}

nofGeologist::nofGeologist(SerializedGameData& sgd, const unsigned obj_id)
    : nofFlagWorker(sgd, obj_id), signs(sgd.PopUnsignedShort())
{
    unsigned available_nodes_count = sgd.PopUnsignedInt();
    for(unsigned i = 0; i < available_nodes_count; ++i)
    {
        MapPoint p = sgd.PopMapPoint();
        available_nodes.push_back(p);
    }

    node_goal = sgd.PopMapPoint();

    if(sgd.GetGameDataVersion() < 6)
    {
        for(const auto res : helpers::enumRange<ResourceType>())
        {
            if(res != ResourceType::Fish && res != ResourceType::Water)
                resAlreadyFound[res] = sgd.PopBool();
        }
    } else
    {
        for(bool& found : resAlreadyFound)
            found = sgd.PopBool();
    }
}

void nofGeologist::Draw(DrawPoint drawPt)
{
    switch(state)
    {
        default: break;
        case State::FigureWork:
        case State::GeologistGotonextnode:
        case State::GoToFlag:
        {
            // normales Laufen zeichnen
            DrawWalkingBobJobs(drawPt, Job::Geologist);
        }
        break;
        case State::GeologistDig:
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
                LOADER.GetPlayerImage("rom_bobs", 314 + i - 6)
                  ->DrawFull(drawPt, COLOR_WHITE, gwg->GetPlayer(player).color);
                if(i == 14)
                {
                    sound = 2;
                    sound_id = 1;
                }
            } else if(i < 28)
            {
                LOADER.GetPlayerImage("rom_bobs", 324 + (i - 16) % 6)
                  ->DrawFull(drawPt, COLOR_WHITE, gwg->GetPlayer(player).color);
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
                LOADER.GetPlayerImage("rom_bobs", 314 + i - 28)
                  ->DrawFull(drawPt, COLOR_WHITE, gwg->GetPlayer(player).color);
                if(i == 36)
                {
                    sound = 2;
                    sound_id = 4;
                }
            } else if(i < 50)
            {
                LOADER.GetPlayerImage("rom_bobs", 324 + (i - 38) % 6)
                  ->DrawFull(drawPt, COLOR_WHITE, gwg->GetPlayer(player).color);
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
                LOADER.GetPlayerImage("rom_bobs", 314 + i - 50)
                  ->DrawFull(drawPt, COLOR_WHITE, gwg->GetPlayer(player).color);
                if(i == 58)
                {
                    sound = 2;
                    sound_id = 7;
                }
            } else
            {
                LOADER.GetPlayerImage("rom_bobs", 324 + (i - 60) % 6)
                  ->DrawFull(drawPt, COLOR_WHITE, gwg->GetPlayer(player).color);
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
        case State::GeologistCheer:
        {
            unsigned short i = GAMECLIENT.Interpolate(16, current_ev);

            if(i < 7)
            {
                LOADER.GetPlayerImage("rom_bobs", 357 + i)->DrawFull(drawPt, COLOR_WHITE, gwg->GetPlayer(player).color);
            } else
            {
                std::array<unsigned char, 9> ids = {1, 0, 1, 2, 1, 0, 1, 2, 1};
                LOADER.GetPlayerImage("rom_bobs", 361 + ids[i - 7])
                  ->DrawFull(drawPt, COLOR_WHITE, gwg->GetPlayer(player).color);
            }

            if(i == 4)
                SOUNDMANAGER.PlayNOSound(107, this, 12); // yippy
        }
        break;
    }

    /*std::array<char, 256> number;
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
    if(state == State::GeologistGotonextnode)
    {
        // Check if the flag still exists (not destroyed) and the goal node is still available (something could be build
        // there)
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
            state = State::GeologistDig;
        } else
        {
            // Weg zum nächsten Punkt suchen
            const auto dir = gwg->FindHumanPath(pos, node_goal, 20);

            // Wenns keinen gibt
            if(!dir)
                GoToNextNode();
            else
                StartWalking(*dir);
        }
    } else if(state == State::GoToFlag)
    {
        GoToFlag();
    }
}

void nofGeologist::HandleDerivedEvent(const unsigned /*id*/)
{
    switch(state)
    {
        default: break;
        case State::GeologistDig:
        {
            // Check what is here
            Resource foundRes = gwg->GetNode(pos).resources;
            // We don't care for fish (and most likely never find it)
            if(foundRes.getType() == ResourceType::Fish)
                foundRes.setType(ResourceType::Nothing);

            if(foundRes.getAmount() > 0u)
            {
                // Es wurde was gefunden, erstmal Jubeln
                state = State::GeologistCheer;
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
        case State::GeologistCheer:
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
    return PathConditionHuman(*gwg).IsNodeOk(pt) && obj.GetGOT() != GO_Type::Sign
           && obj.GetType() != NodalObjectType::Flag && obj.GetType() != NodalObjectType::Tree;
}

namespace {
struct GetMapPointWithRadius
{
    using result_type = std::pair<MapPoint, unsigned>;

    result_type operator()(const MapPoint pt, unsigned r) { return std::make_pair(pt, r); }
};
} // namespace

void nofGeologist::LookForNewNodes()
{
    std::vector<GetMapPointWithRadius::result_type> pts =
      gwg->GetPointsInRadius(flag->GetPos(), 15, GetMapPointWithRadius());
    unsigned curMaxRadius = 15;
    bool found = false;
    for(const auto& it : pts)
    {
        if(it.second > curMaxRadius)
            break;
        if(IsValidTargetNode(it.first))
        {
            available_nodes.push_back(it.first);
            if(!found)
            {
                found = true;
                // if we found a valid node, look only in other nodes within 2 more "circles"
                curMaxRadius = std::min(10u, it.second + 2);
            }
        }
    }
}

bool nofGeologist::IsValidTargetNode(const MapPoint pt) const
{
    return (IsNodeGood(pt) && !gwg->GetNode(pt).reserved && (pos == pt || gwg->FindHumanPath(pos, pt, 20)));
}

helpers::OptionalEnum<Direction> nofGeologist::GetNextNode()
{
    // Überhaupt noch Schilder zum Aufstellen
    if(!signs)
    {
        node_goal = MapPoint::Invalid();
        return boost::none;
    }

    do
    {
        // Sind überhaupt Punkte verfügbar?
        while(!available_nodes.empty())
        {
            // Dann einen Punkt zufällig auswählen
            int randNode = RANDOM_RAND(available_nodes.size());
            node_goal = available_nodes[randNode];
            // und aus der Liste entfernen
            available_nodes.erase(available_nodes.begin() + randNode);
            // Gucken, ob er gut ist und ob man hingehen kann und ob er noch nicht reserviert wurde!
            if(!IsNodeGood(node_goal) || gwg->GetNode(node_goal).reserved)
                continue;

            helpers::OptionalEnum<Direction> ret_dir;
            if(pos != node_goal)
            {
                ret_dir = gwg->FindHumanPath(pos, node_goal, 20);
                if(!ret_dir)
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
    return boost::none;
}

void nofGeologist::GoToNextNode()
{
    // Wenn es keine Flagge mehr gibt, dann gleich rumirren
    if(!flag)
    {
        StartWandering();
        Wander();
        state = State::FigureWork;
        return;
    }

    // ersten Punkt suchen
    const auto dir = GetNextNode();

    if(dir)
    {
        // Wenn es einen Punkt gibt, dann hingehen
        state = State::GeologistGotonextnode;
        StartWalking(*dir);
        --signs;
    } else if(node_goal == pos)
    {
        // Already there
        state = State::GeologistGotonextnode;
        --signs;
        Walked();
    } else
    {
        // ansonsten zur Flagge zurückgehen
        state = State::GoToFlag;
        Walked();
    }
}

void nofGeologist::SetSign(Resource resources)
{
    RTTR_Assert(resources.getType() != ResourceType::Fish); // Shall never happen

    // Bestimmte Objekte können gelöscht werden
    NodalObjectType noType = gwg->GetNO(pos)->GetType();
    if(noType != NodalObjectType::Nothing && noType != NodalObjectType::Environment)
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
            case ResourceType::Iron: msg = _("Found iron ore"); break;
            case ResourceType::Gold: msg = _("Found gold"); break;
            case ResourceType::Coal: msg = _("Found coal"); break;
            case ResourceType::Granite: msg = _("Found granite"); break;
            case ResourceType::Water: msg = _("Found water"); break;
            default: RTTR_Assert(false); return;
        }

        if(resources.getType() != ResourceType::Water || gwg->GetGGS().getSelection(AddonId::EXHAUSTIBLE_WATER) != 1)
        {
            SendPostMessage(player,
                            std::make_unique<PostMsg>(GetEvMgr().GetCurrentGF(), msg, PostCategory::Geologist, pos));
        }

        gwg->GetNotifications().publish(ResourceNote(player, pos, resources));
        if(gwg->HasLua())
            gwg->GetLua().EventResourceFound(this->player, pos, resources.getType(), resources.getAmount());
    }
    resAlreadyFound[resources.getType()] = true;
}

void nofGeologist::LostWork()
{
    flag = nullptr;

    switch(state)
    {
        default: break;
        // Wenn wir noch hingehen, dann zurückgehen
        case State::FigureWork:
        {
            GoHome();
        }
        break;
        case State::GoToFlag:
        {
            // dann sofort rumirren, wenn wir zur Flagge gehen
            StartWandering();
            state = State::FigureWork;
        }
        break;
    }
}

struct IsSignOfType
{
    const ResourceType type;
    const World& gwb;

    IsSignOfType(ResourceType type, const World& gwb) : type(type), gwb(gwb) {}

    bool operator()(const MapPoint& pt, unsigned /*distance*/)
    {
        const auto* sign = gwb.GetSpecObj<noSign>(pt);
        return sign && sign->GetSignType() == type;
    }
};

bool nofGeologist::IsSignInArea(ResourceType type) const
{
    return gwg->CheckPointsInRadius(pos, 7, IsSignOfType(type, *gwg), false);
}
