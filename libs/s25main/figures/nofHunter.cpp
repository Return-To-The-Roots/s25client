// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofHunter.h"

#include "EventManager.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "RTTR_Assert.h"
#include "SerializedGameData.h"
#include "SoundManager.h"
#include "buildings/nobUsual.h"
#include "enum_cast.hpp"
#include "network/GameClient.h"
#include "notifications/BuildingNote.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "pathfinding/PathConditionHuman.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include "nodeObjs/noAnimal.h"
#include "gameData/GameConsts.h"
#include "gameData/JobConsts.h"
#include <stdexcept>

/// Maximale Distanz, die ein Jäger läuft, um ein Tier zu jagen
const MapCoord MAX_HUNTING_DISTANCE = 50;

nofHunter::nofHunter(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofBuildingWorker(Job::Hunter, pos, player, workplace), animal(nullptr), shootingPos(MapPoint::Invalid()),
      shooting_dir(Direction::West)
{}

void nofHunter::Serialize(SerializedGameData& sgd) const
{
    nofBuildingWorker::Serialize(sgd);

    if(state != State::FigureWork && state != State::Waiting1)
    {
        sgd.PushObject(animal, true);
        helpers::pushPoint(sgd, shootingPos);
        sgd.PushEnum<uint8_t>(shooting_dir);
    }
}

nofHunter::nofHunter(SerializedGameData& sgd, const unsigned obj_id) : nofBuildingWorker(sgd, obj_id)
{
    if(state != State::FigureWork && state != State::Waiting1)
    {
        animal = sgd.PopObject<noAnimal>(GO_Type::Animal);
        shootingPos = sgd.PopMapPoint();
        shooting_dir = sgd.Pop<Direction>();
        // https://github.com/Return-To-The-Roots/s25client/issues/1126
        if(sgd.GetGameDataVersion() < 4 && state == State::HunterFindingShootingpoint && pos == shootingPos)
            state = State::HunterWaitingForAnimalReady;
    } else
    {
        animal = nullptr;
        shootingPos = MapPoint::Invalid();
        shooting_dir = Direction::West;
    }
}

void nofHunter::DrawWorking(DrawPoint drawPt)
{
    const GamePlayer& owner = world->GetPlayer(player);
    switch(state)
    {
        default: break;
        case State::HunterWaitingForAnimalReady:
            LOADER.getBobSprite(owner.nation, Job::Hunter, shooting_dir, 0).drawForPlayer(drawPt, owner.color);
            break;
        case State::HunterShooting:
        {
            if(shooting_dir == Direction::East)
            {
                // die Animation in dieser Richtung ist etwas anders als die in den restlichen
                unsigned short id = GAMECLIENT.Interpolate(13, current_ev);
                LOADER.GetPlayerImage("rom_bobs", 219 + id)->drawForPlayer(drawPt, owner.color);

                if(id == 12)
                {
                    world->GetSoundMgr().playNOSound(74, *this, 0);
                    was_sounding = true;
                }
            } else
            {
                unsigned short id = GAMECLIENT.Interpolate(8, current_ev);
                LOADER.GetPlayerImage("rom_bobs", 1686 + rttr::enum_cast(shooting_dir + 2u) * 8 + id)
                  ->drawForPlayer(drawPt, owner.color);

                if(id == 7)
                {
                    world->GetSoundMgr().playNOSound(74, *this, 0);
                    was_sounding = true;
                }
            }

            DrawShadow(drawPt, 0, shooting_dir);
        }
        break;
        case State::HunterEviscerating:
        {
            unsigned short id = GAMECLIENT.Interpolate(45, current_ev);
            unsigned short draw_id;

            if(id < 4)
                draw_id = 232 + id;
            else if(id < 36)
                draw_id = 236 + (id - 4) % 8;
            else
                draw_id = 244 + id - 36;

            LOADER.GetPlayerImage("rom_bobs", draw_id)->drawForPlayer(drawPt, owner.color);
        }
        break;
    }
}

void nofHunter::HandleDerivedEvent(unsigned /*id*/)
{
    switch(state)
    {
        default: RTTR_Assert(false); break;
        case State::Waiting1: TryStartHunting(); break;
        case State::HunterWaitingForAnimalReady: HandleStateWaitingForAnimalReady(); break;
        case State::HunterShooting: HandleStateShooting(); break;
        case State::HunterEviscerating: HandleStateEviscerating(); break;
    }
}

void nofHunter::TryStartHunting()
{
    // Find animals in a square around building (actually should be circle, but animals are moving anyway)
    const int SQUARE_SIZE = 19;

    // Liste mit den gefundenen Tieren
    std::vector<noAnimal*> available_animals;

    // Durchgehen und nach Tieren suchen
    Position curPos;
    for(curPos.y = pos.y - SQUARE_SIZE; curPos.y <= pos.y + SQUARE_SIZE; ++curPos.y)
    {
        for(curPos.x = pos.x - SQUARE_SIZE; curPos.x <= pos.x + SQUARE_SIZE; ++curPos.x)
        {
            MapPoint curMapPos = world->MakeMapPoint(curPos);

            // nach Tieren suchen
            for(auto& figure : world->GetFigures(curMapPos))
            {
                if(figure.GetType() != NodalObjectType::Animal)
                    continue;
                // Ist das Tier überhaupt zum Jagen geeignet?
                auto& animal = static_cast<noAnimal&>(figure);
                if(!animal.CanHunted())
                    continue;

                // Und komme ich hin?
                if(pos == animal.GetPos() || world->FindHumanPath(pos, animal.GetPos(), MAX_HUNTING_DISTANCE))
                {
                    // Dann nehmen wir es
                    available_animals.push_back(&animal);
                }
            }
        }
    }

    // Gibt es überhaupt ein Tier, das ich jagen kann?
    if(!available_animals.empty())
    {
        // Ein Tier zufällig heraussuchen
        animal = RANDOM_ELEMENT(available_animals);

        // Wir jagen es jetzt
        state = State::HunterChasing;

        // Wir arbeiten jetzt
        workplace->is_working = true;

        // Tier Bescheid sagen
        animal->BeginHunting(this);

        // Anfangen zu laufen (erstmal aus dem Haus raus!)
        StartWalking(Direction::SouthEast);

        workplace->StopNotWorking();
    } else
    {
        // Weiter warten, vielleicht gibts ja später wieder mal was
        current_ev = GetEvMgr().AddEvent(this, JOB_CONSTS[job_].wait1_length, 1);
        world->GetNotifications().publish(
          BuildingNote(BuildingNote::NoRessources, player, workplace->GetPos(), workplace->GetBuildingType()));
        workplace->StartNotWorking();
    }

    was_sounding = false;
}

void nofHunter::WalkedDerived()
{
    switch(state)
    {
        default: break;
        case State::HunterChasing: HandleStateChasing(); break;
        case State::HunterFindingShootingpoint: HandleStateFindingShootingPoint(); break;
        case State::HunterWalkingToCadaver: HandleStateWalkingToCadaver(); break;
        case State::WalkingHome: HandleStateWalkingHome(); break;
    }
}

bool nofHunter::IsShootingPointGood(const MapPoint pt)
{
    // Punkt muss betretbar sein und man muss ihn erreichen können
    return PathConditionHuman(*world).IsNodeOk(pt) && world->FindHumanPath(this->pos, pt, 6) != boost::none;
}

void nofHunter::HandleStateChasing()
{
    // Sind wir in der Nähe des Tieres?
    if(world->CalcDistance(pos, animal->GetPos()) < 7)
    {
        // Dann bitten wir es mal, schonmal anzuhalten und bekommen seine Koordinaten, wo es dann steht
        Position animalPos(animal->HunterIsNear());

        // Nun müssen wir drumherum einen Punkt suchen, von dem wir schießen, der natürlich direkt dem Standort
        // des Tieres gegenüberliegen muss (mit zufälliger Richtung beginnen)
        shootingPos = MapPoint::Invalid();
        for(const Direction d : helpers::enumRange(RANDOM_ENUM(Direction)))
        {
            Position delta;
            switch(d)
            {
                case Direction::West:
                    delta.x = -4;
                    delta.y = 0;
                    break;
                case Direction::NorthWest:
                    delta.x = -2;
                    delta.y = -4;
                    break;
                case Direction::NorthEast:
                    delta.x = 2;
                    delta.y = -4;
                    break;
                case Direction::East:
                    delta.x = 4;
                    delta.y = 0;
                    break;
                case Direction::SouthEast:
                    delta.x = 2;
                    delta.y = 4;
                    break;
                case Direction::SouthWest:
                    delta.x = -2;
                    delta.y = 4;
                    break;
                default: throw std::logic_error("Wrong value?");
            }

            MapPoint curShootingPos = world->MakeMapPoint(animalPos + delta);
            if(curShootingPos == pos || world->FindHumanPath(pos, curShootingPos, 6))
            {
                shootingPos = curShootingPos;
                // Richtung, in die geschossen wird, bestimmen (natürlich die entgegengesetzte nehmen)
                shooting_dir = d + 3u;
                break;
            }
        }

        // Wurde ein Punkt gefunden?
        if(shootingPos.isValid())
        {
            // dorthingehen
            state = State::HunterFindingShootingpoint;
            HandleStateFindingShootingPoint();
        } else
        {
            // kein Punkt gefunden --> nach Hause gehen
            StartWalkingHome();
        }

    } else
    {
        // Weg dorthin suchen
        const auto dir = world->FindHumanPath(pos, animal->GetPos(), MAX_HUNTING_DISTANCE);
        if(dir)
        {
            // Weg gefunden, dann hinlaufen
            StartWalking(*dir);
        } else
        {
            // kein Weg gefunden --> nach Hause laufen
            StartWalkingHome();
        }
    }
}

void nofHunter::HandleStateFindingShootingPoint()
{
    // Are we there yet?
    if(shootingPos == pos)
    {
        state = State::HunterWaitingForAnimalReady;
        HandleStateWaitingForAnimalReady();
    } else
    {
        // Weg dorthin suchen
        const auto dir = world->FindHumanPath(pos, shootingPos, 6);
        if(dir)
        {
            // Weg gefunden, dann hinlaufen
            StartWalking(*dir);
        } else
        {
            // kein Weg gefunden --> nach Hause laufen
            StartWalkingHome();
        }
    }
}

void nofHunter::HandleStateWaitingForAnimalReady()
{
    // Is the animal ready?
    if(animal->IsReadyForShooting())
    {
        // fire!
        state = State::HunterShooting;
        current_ev = GetEvMgr().AddEvent(this, 16, 1);
    } else if(animal->IsGettingReadyForShooting())
        current_ev = GetEvMgr().AddEvent(this, 15, 2); // Give the animal some time for getting ready
    else
    {
        // Something went wrong, animal is doing something else?
        StartWalkingHome();
    }
}

void nofHunter::HandleStateShooting()
{
    // Tier muss sterben
    animal->Die();
    // zum Kadaver laufen, um ihn auszunehmen
    state = State::HunterWalkingToCadaver;
    HandleStateWalkingToCadaver();
}

void nofHunter::HandleStateWalkingToCadaver()
{
    // Sind wir schon da?
    if(animal->GetPos() == pos)
    {
        // dann ausnehmen
        state = State::HunterEviscerating;
        current_ev = GetEvMgr().AddEvent(this, 80, 1);
    } else
    {
        // Weg dorthin suchen
        const auto dir = world->FindHumanPath(pos, animal->GetPos(), 6);
        if(dir)
        {
            // Weg gefunden, dann hinlaufen
            StartWalking(*dir);
        } else
        {
            // kein Weg gefunden --> nach Hause laufen
            StartWalkingHome();
        }
    }
}

void nofHunter::HandleStateEviscerating()
{
    // Evtl. Sounds löschen
    if(was_sounding)
    {
        world->GetSoundMgr().stopSounds(*this);
        was_sounding = false;
    }
    // Tier verschwinden lassen
    auto ownedAnimal = world->RemoveFigure(pos, *animal);
    animal = nullptr;
    // Tier vernichten
    ownedAnimal->Eviscerated();
    ownedAnimal->Destroy();
    // Fleisch in die Hand nehmen
    ware = GoodType::Meat;
    // und zurück zur Hütte
    StartWalkingHome();
}

void nofHunter::StartWalkingHome()
{
    WorkAborted();
    state = State::WalkingHome;
    // We may be still walking in which case we delay finding a path home until we reached the next node
    if(!IsMoving())
        HandleStateWalkingHome();
}

void nofHunter::HandleStateWalkingHome()
{
    // Sind wir zu Hause angekommen? (genauer an der Flagge !!)
    MapPoint homeFlagPos = world->GetNeighbour(workplace->GetPos(), Direction::SouthEast);
    if(pos == homeFlagPos)
    {
        // Weiteres übernimmt nofBuildingWorker
        WorkingReady();
        return;
    }

    // Weg suchen und ob wir überhaupt noch nach Hause kommen (Toleranz bei dem Weg mit einberechnen,
    // damit er nicht einfach rumirrt und wegstirbt, wenn er einmal ein paar Felder zu weit gelaufen ist)
    const auto dir = world->FindHumanPath(pos, homeFlagPos, MAX_HUNTING_DISTANCE + MAX_HUNTING_DISTANCE / 4);
    if(dir)
    {
        // All good, let's start walking there
        StartWalking(*dir);
    } else
    {
        // Kein Weg führt mehr nach Hause--> Rumirren
        AbrogateWorkplace();
        StartWandering();
        Wander();
    }
}

void nofHunter::AnimalLost()
{
    animal = nullptr;

    switch(state)
    {
        default: return;
        case State::HunterChasing:
        case State::HunterFindingShootingpoint:
        case State::HunterWalkingToCadaver: StartWalkingHome(); break;
        case State::HunterShooting:
        case State::HunterEviscerating:
        case State::HunterWaitingForAnimalReady:
            // Arbeits-Event abmelden
            GetEvMgr().RemoveEvent(current_ev);
            // Nach Hause laufen
            StartWalkingHome();
            break;
    }
}

void nofHunter::WorkAborted()
{
    // Tier Bescheid sagen
    if(animal)
    {
        animal->StopHunting();
        animal = nullptr;
    }
}
