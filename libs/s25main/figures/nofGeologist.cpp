// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofGeologist.h"
#include "EventManager.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "Loader.h"
#include "ReturnMapPointWithRadius.h"
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
#include "world/GameWorld.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noSign.h"

nofGeologist::nofGeologist(const MapPoint pos, const unsigned char player, noRoadNode* goal)
    : nofFlagWorker(Job::Geologist, pos, player, goal), signs(0), node_goal(0, 0)
{
    std::fill(resAlreadyFound.begin(), resAlreadyFound.end(), false);
}

void nofGeologist::serialize(SerializedGameData& sgd) const
{
    nofFlagWorker::serialize(sgd);

    sgd.PushUnsignedShort(signs);

    sgd.PushUnsignedInt(available_nodes.size());
    for(const auto& available_node : available_nodes)
    {
        helpers::pushPoint(sgd, available_node);
    }

    helpers::pushPoint(sgd, node_goal);
    helpers::pushContainer(sgd, resAlreadyFound);
}

nofGeologist::nofGeologist(SerializedGameData& sgd, const unsigned obj_id)
    : nofFlagWorker(sgd, obj_id), signs(sgd.PopUnsignedShort())
{
    unsigned available_nodes_count = sgd.PopUnsignedInt();
    for(unsigned i = 0; i < available_nodes_count; ++i)
    {
        MapPoint p = sgd.popMapPoint();
        available_nodes.push_back(p);
    }

    node_goal = sgd.popMapPoint();

    if(sgd.getGameDataVersion() < 6)
    {
        for(const auto res : helpers::enumRange<ResourceType>())
        {
            if(res != ResourceType::Fish && res != ResourceType::Water)
                resAlreadyFound[res] = sgd.PopBool();
        }
    } else
        helpers::popContainer(sgd, resAlreadyFound);
}

void nofGeologist::draw(DrawPoint drawPt)
{
    switch(state)
    {
        default: break;
        case State::FigureWork:
        case State::GeologistGotoNextNode:
        case State::GoToFlag:
        {
            // normales Laufen zeichnen
            drawWalkingBobJobs(drawPt, Job::Geologist);
        }
        break;
        case State::GeologistDig:
        {
            // 1x grab, 1x "spring-grab", 2x grab, 1x "spring-grab", 2x grab, 1x "spring-grab", 4x grab
            unsigned short i = GAMECLIENT.interpolate(84, current_ev);
            unsigned sound = 0, sound_id;

            if(i < 6)
            {
                LOADER.getPlayerImage("rom_bobs", 324 + i)
                  ->drawFull(drawPt, COLOR_WHITE, world->getPlayer(player).color);
                if(i == 4)
                {
                    sound = 1;
                    sound_id = 0;
                }
            } else if(i < 16)
            {
                LOADER.getPlayerImage("rom_bobs", 314 + i - 6)
                  ->drawFull(drawPt, COLOR_WHITE, world->getPlayer(player).color);
                if(i == 14)
                {
                    sound = 2;
                    sound_id = 1;
                }
            } else if(i < 28)
            {
                LOADER.getPlayerImage("rom_bobs", 324 + (i - 16) % 6)
                  ->drawFull(drawPt, COLOR_WHITE, world->getPlayer(player).color);
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
                LOADER.getPlayerImage("rom_bobs", 314 + i - 28)
                  ->drawFull(drawPt, COLOR_WHITE, world->getPlayer(player).color);
                if(i == 36)
                {
                    sound = 2;
                    sound_id = 4;
                }
            } else if(i < 50)
            {
                LOADER.getPlayerImage("rom_bobs", 324 + (i - 38) % 6)
                  ->drawFull(drawPt, COLOR_WHITE, world->getPlayer(player).color);
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
                LOADER.getPlayerImage("rom_bobs", 314 + i - 50)
                  ->drawFull(drawPt, COLOR_WHITE, world->getPlayer(player).color);
                if(i == 58)
                {
                    sound = 2;
                    sound_id = 7;
                }
            } else
            {
                LOADER.getPlayerImage("rom_bobs", 324 + (i - 60) % 6)
                  ->drawFull(drawPt, COLOR_WHITE, world->getPlayer(player).color);
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
                world->getSoundMgr().playNOSound((sound == 1) ? 81 : 56, *this, sound_id);
        }
        break;
        case State::GeologistCheer:
        {
            unsigned short i = GAMECLIENT.interpolate(16, current_ev);

            if(i < 7)
            {
                LOADER.getPlayerImage("rom_bobs", 357 + i)
                  ->drawFull(drawPt, COLOR_WHITE, world->getPlayer(player).color);
            } else
            {
                std::array<unsigned char, 9> ids = {1, 0, 1, 2, 1, 0, 1, 2, 1};
                LOADER.getPlayerImage("rom_bobs", 361 + ids[i - 7])
                  ->drawFull(drawPt, COLOR_WHITE, world->getPlayer(player).color);
            }

            if(i == 4)
                world->getSoundMgr().playNOSound(107, *this, 12); // yippy
        }
        break;
    }
}

void nofGeologist::goalReached()
{
    // An der Flagge angekommen

    // Geologe muss nun 15 Schildchen aufstellen

    signs = 15;

    // Für jeden Ressourcentyp nur einmal eine Post-Nachricht schicken
    std::fill(resAlreadyFound.begin(), resAlreadyFound.end(), false);

    // Umgebung absuchen
    lookForNewNodes();

    // ersten Punkt suchen
    goToNextNode();
}

void nofGeologist::walked()
{
    if(state == State::GeologistGotoNextNode)
    {
        // Check if the flag still exists (not destroyed) and the goal node is still available (something could be build
        // there)
        if(!flag || !isNodeGood(node_goal))
        {
            // alten Punkt wieder freigeben
            world->setReserved(node_goal, false);
            // wenn nicht, dann zu einem neuen Punkt gehen
            goToNextNode();
            return;
        }

        // Bin ich am Zielpunkt?
        if(pos == node_goal)
        {
            // anfangen zu graben
            current_ev = getEvMgr().addEvent(this, 100, 1);
            state = State::GeologistDig;
        } else
        {
            // Weg zum nächsten Punkt suchen
            const auto dir = world->findHumanPath(pos, node_goal, 20);

            // Wenns keinen gibt
            if(!dir)
                goToNextNode();
            else
                startWalking(*dir);
        }
    } else if(state == State::GoToFlag)
    {
        goToFlag();
    }
}

void nofGeologist::handleDerivedEvent(const unsigned /*id*/)
{
    switch(state)
    {
        default: break;
        case State::GeologistDig:
        {
            // Check what is here
            Resource foundRes = world->getNode(pos).resources;
            // We don't care for fish (and most likely never find it)
            if(foundRes.getType() == ResourceType::Fish)
                foundRes.setType(ResourceType::Nothing);

            if(foundRes.getAmount() > 0u)
            {
                // Es wurde was gefunden, erstmal Jubeln
                state = State::GeologistCheer;
                current_ev = getEvMgr().addEvent(this, 15, 1);
            } else
            {
                // leeres Schild hinstecken und ohne Jubel weiterziehen
                setSign(foundRes);
                /// Punkt wieder freigeben
                world->setReserved(pos, false);
                goToNextNode();
            }
        }
        break;
        case State::GeologistCheer:
        {
            // Schild reinstecken
            setSign(world->getNode(pos).resources);
            /// Punkt wieder freigeben
            world->setReserved(pos, false);
            /// Sounds evtl löschen
            world->getSoundMgr().stopSounds(*this);
            // Und weiterlaufen
            goToNextNode();
        }
        break;
    }
}

bool nofGeologist::isNodeGood(const MapPoint pt) const
{
    // Es dürfen auch keine bestimmten Objekte darauf stehen und auch keine Schilder !!
    const noBase& obj = *world->getNO(pt);
    return PathConditionHuman(*world).isNodeOk(pt) && obj.getGOT() != GO_Type::Sign
           && obj.getType() != NodalObjectType::Flag && obj.getType() != NodalObjectType::Tree;
}

void nofGeologist::lookForNewNodes()
{
    const auto pts = world->getPointsInRadius(flag->getPos(), 15, ReturnMapPointWithRadius{});
    unsigned curMaxRadius = 15;
    bool found = false;
    for(const auto& it : pts)
    {
        if(it.second > curMaxRadius)
            break;
        if(isValidTargetNode(it.first))
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

bool nofGeologist::isValidTargetNode(const MapPoint pt) const
{
    return (isNodeGood(pt) && !world->getNode(pt).reserved && (pos == pt || world->findHumanPath(pos, pt, 20)));
}

helpers::OptionalEnum<Direction> nofGeologist::getNextNode()
{
    // Überhaupt noch Schilder zum Aufstellen
    if(!signs)
    {
        node_goal = MapPoint::invalid();
        return std::nullopt;
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
            if(!isNodeGood(node_goal) || world->getNode(node_goal).reserved)
                continue;

            helpers::OptionalEnum<Direction> ret_dir;
            if(pos != node_goal)
            {
                ret_dir = world->findHumanPath(pos, node_goal, 20);
                if(!ret_dir)
                    continue;
            }
            // Reservieren
            world->setReserved(node_goal, true);
            return ret_dir;
        }

        // Nach neuen Punkten sucehn
        lookForNewNodes();
    } while(!available_nodes.empty());

    node_goal = MapPoint::invalid();
    return std::nullopt;
}

void nofGeologist::goToNextNode()
{
    // Wenn es keine Flagge mehr gibt, dann gleich rumirren
    if(!flag)
    {
        startWandering();
        wander();
        state = State::FigureWork;
        return;
    }

    // ersten Punkt suchen
    const auto dir = getNextNode();

    if(dir)
    {
        // Wenn es einen Punkt gibt, dann hingehen
        state = State::GeologistGotoNextNode;
        startWalking(*dir);
        --signs;
    } else if(node_goal == pos)
    {
        // Already there
        state = State::GeologistGotoNextNode;
        --signs;
        walked();
    } else
    {
        // ansonsten zur Flagge zurückgehen
        state = State::GoToFlag;
        walked();
    }
}

void nofGeologist::setSign(Resource resources)
{
    RTTR_Assert(resources.getType() != ResourceType::Fish); // Shall never happen

    // Bestimmte Objekte können gelöscht werden
    NodalObjectType noType = world->getNO(pos)->getType();
    if(noType != NodalObjectType::Nothing && noType != NodalObjectType::Environment)
        return;
    world->destroyNO(pos, false);

    // Schild setzen
    world->setNO(pos, new noSign(pos, resources));

    // If nothing found, there is nothing left to do
    if(resources.getAmount() == 0u)
        return;

    if(!resAlreadyFound[resources.getType()] && !isSignInArea(resources.getType()))
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

        if(resources.getType() != ResourceType::Water || world->getGGS().getSelection(AddonId::EXHAUSTIBLE_WATER) != 1)
        {
            sendPostMessage(player,
                            std::make_unique<PostMsg>(getEvMgr().getCurrentGF(), msg, PostCategory::Geologist, pos));
        }

        world->getNotifications().publish(ResourceNote(player, pos, resources));
        if(world->hasLua())
            world->getLua().eventResourceFound(this->player, pos, resources.getType(), resources.getAmount());
    }
    resAlreadyFound[resources.getType()] = true;
}

void nofGeologist::lostWork()
{
    flag = nullptr;

    switch(state)
    {
        default: break;
        // Wenn wir noch hingehen, dann zurückgehen
        case State::FigureWork: goHome(); break;
        case State::GoToFlag:
            // dann sofort rumirren, wenn wir zur Flagge gehen
            startWandering();
            state = State::FigureWork;
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
        const auto* sign = gwb.getSpecObj<noSign>(pt);
        return sign && sign->getResource().getType() == type;
    }
};

bool nofGeologist::isSignInArea(ResourceType type) const
{
    return world->checkPointsInRadius(pos, 7, IsSignOfType(type, *world), false);
}
