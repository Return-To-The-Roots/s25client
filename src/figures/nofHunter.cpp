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
#include "nofHunter.h"

#include "EventManager.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "SoundManager.h"
#include "buildings/nobUsual.h"
#include "network/GameClient.h"
#include "notifications/BuildingNote.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "pathfinding/PathConditionHuman.h"
#include "random/Random.h"
#include "world/GameWorldGame.h"
#include "nodeObjs/noAnimal.h"
#include "gameData/GameConsts.h"
#include "gameData/JobConsts.h"
#include <stdexcept>

/// Maximale Distanz, die ein Jäger läuft, um ein Tier zu jagen
const MapCoord MAX_HUNTING_DISTANCE = 50;

nofHunter::nofHunter(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofBuildingWorker(JOB_HUNTER, pos, player, workplace), animal(NULL), shootingPos(0, 0)
{}

void nofHunter::Serialize_nofHunter(SerializedGameData& sgd) const
{
    Serialize_nofBuildingWorker(sgd);

    if(state != STATE_FIGUREWORK && state != STATE_WAITING1)
    {
        sgd.PushObject(animal, true);
        sgd.PushMapPoint(shootingPos);
        sgd.PushUnsignedChar(shooting_dir.toUInt());
    }
}

nofHunter::nofHunter(SerializedGameData& sgd, const unsigned obj_id) : nofBuildingWorker(sgd, obj_id)
{
    if(state != STATE_FIGUREWORK && state != STATE_WAITING1)
    {
        animal = sgd.PopObject<noAnimal>(GOT_ANIMAL);
        shootingPos = sgd.PopMapPoint();
        shooting_dir = Direction::fromInt(sgd.PopUnsignedChar());
    } else
    {
        animal = NULL;
        shootingPos = MapPoint::Invalid();
    }
}

void nofHunter::DrawWorking(DrawPoint drawPt)
{
    switch(state)
    {
        default: break;
        case STATE_HUNTER_SHOOTING:
        {
            if(shooting_dir == Direction::EAST)
            {
                // die Animation in dieser Richtung ist etwas anders als die in den restlichen
                unsigned short id = GAMECLIENT.Interpolate(13, current_ev);
                LOADER.GetPlayerImage("rom_bobs", 219 + id)->DrawFull(drawPt, COLOR_WHITE, gwg->GetPlayer(player).color);

                if(id == 12)
                {
                    SOUNDMANAGER.PlayNOSound(74, this, 0);
                    was_sounding = true;
                }
            } else
            {
                unsigned short id = GAMECLIENT.Interpolate(8, current_ev);
                LOADER.GetPlayerImage("rom_bobs", 1686 + (shooting_dir + 2u).toUInt() * 8 + id)
                  ->DrawFull(drawPt, COLOR_WHITE, gwg->GetPlayer(player).color);

                if(id == 7)
                {
                    SOUNDMANAGER.PlayNOSound(74, this, 0);
                    was_sounding = true;
                }
            }

            DrawShadow(drawPt, 0, shooting_dir);
        }
        break;
        case STATE_HUNTER_EVISCERATING:
        {
            unsigned short id = GAMECLIENT.Interpolate(45, current_ev);
            unsigned short draw_id;

            if(id < 4)
                draw_id = 232 + id;
            else if(id < 36)
                draw_id = 236 + (id - 4) % 8;
            else
                draw_id = 244 + id - 36;

            LOADER.GetPlayerImage("rom_bobs", draw_id)->DrawFull(drawPt, COLOR_WHITE, gwg->GetPlayer(player).color);
        }
        break;
    }
}

void nofHunter::HandleDerivedEvent(const unsigned id)
{
    switch(state)
    {
        default: break;
        case STATE_WAITING1:
        {
            // Fertig mit warten --> anfangen zu arbeiten
            TryStartHunting();
        }
        break;
        case STATE_HUNTER_FINDINGSHOOTINGPOINT:
            if(id == 2)
                HandleStateFindingShootingPoint();
            break;
        case STATE_HUNTER_SHOOTING: { HandleStateShooting();
        }
        break;
        case STATE_HUNTER_EVISCERATING:
        {
            HandleStateEviscerating();

            // Evtl. Sounds löschen
            if(was_sounding)
            {
                SOUNDMANAGER.WorkingFinished(this);
                was_sounding = false;
            }
        }
        break;
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
            MapPoint curMapPos = gwg->MakeMapPoint(curPos);
            const std::list<noBase*>& figures = gwg->GetFigures(curMapPos);

            // nach Tieren suchen
            for(std::list<noBase*>::const_iterator it = figures.begin(); it != figures.end(); ++it)
            {
                if((*it)->GetType() != NOP_ANIMAL)
                    continue;
                // Ist das Tier überhaupt zum Jagen geeignet?
                if(!static_cast<noAnimal*>(*it)->CanHunted())
                    continue;

                // Und komme ich hin?
                if(gwg->FindHumanPath(pos, static_cast<noAnimal*>(*it)->GetPos(), MAX_HUNTING_DISTANCE) != 0xFF)
                {
                    // Dann nehmen wir es
                    available_animals.push_back(static_cast<noAnimal*>(*it));
                }
            }
        }
    }

    // Gibt es überhaupt ein Tier, das ich jagen kann?
    if(!available_animals.empty())
    {
        // Ein Tier zufällig heraussuchen
        animal = available_animals[RANDOM.Rand(__FILE__, __LINE__, GetObjId(), available_animals.size())];

        // Wir jagen es jetzt
        state = STATE_HUNTER_CHASING;

        // Wir arbeiten jetzt
        workplace->is_working = true;

        // Tier Bescheid sagen
        animal->BeginHunting(this);

        // Anfangen zu laufen (erstmal aus dem Haus raus!)
        StartWalking(Direction::SOUTHEAST);

        workplace->StopNotWorking();
    } else
    {
        // Weiter warten, vielleicht gibts ja später wieder mal was
        current_ev = GetEvMgr().AddEvent(this, JOB_CONSTS[job_].wait1_length, 1);
        gwg->GetNotifications().publish(
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
        case STATE_HUNTER_CHASING: { HandleStateChasing();
        }
        break;
        case STATE_HUNTER_FINDINGSHOOTINGPOINT: { HandleStateFindingShootingPoint();
        }
        break;
        case STATE_HUNTER_WALKINGTOCADAVER: { HandleStateWalkingToCadaver();
        }
        break;
        case STATE_WALKINGHOME: { WalkHome();
        }
        break;
    }
}

bool nofHunter::IsShootingPointGood(const MapPoint pt)
{
    // Punkt muss betretbar sein und man muss ihn erreichen können
    return (PathConditionHuman(*gwg).IsNodeOk(pt) && gwg->FindHumanPath(this->pos, pt, 6) != INVALID_DIR);
}

void nofHunter::HandleStateChasing()
{
    // Sind wir in der Nähe des Tieres?
    if(gwg->CalcDistance(pos, animal->GetPos()) < 7)
    {
        // Dann bitten wir es mal, schonmal anzuhalten und bekommen seine Koordinaten, wo es dann steht
        Position animalPos(animal->HunterIsNear());

        // Nun müssen wir drumherum einen Punkt suchen, von dem wir schießen, der natürlich direkt dem Standort
        // des Tieres gegenüberliegen muss (mit zufälliger Richtung beginnen)
        unsigned char doffset = RANDOM.Rand(__FILE__, __LINE__, GetObjId(), 6);
        shootingPos = MapPoint::Invalid();
        unsigned char d;
        for(d = 0; d < 6; ++d)
        {
            Position delta;
            switch((d + doffset) % 6)
            {
                case 0:
                    delta.x = -4;
                    delta.y = 0;
                    break;
                case 1:
                    delta.x = -2;
                    delta.y = -4;
                    break;
                case 2:
                    delta.x = 2;
                    delta.y = -4;
                    break;
                case 3:
                    delta.x = 4;
                    delta.y = 0;
                    break;
                case 4:
                    delta.x = 2;
                    delta.y = 4;
                    break;
                case 5:
                    delta.x = -2;
                    delta.y = 4;
                    break;
                default: throw std::logic_error("Wrong value?");
            }

            MapPoint curShootingPos = gwg->MakeMapPoint(animalPos + delta);
            if(curShootingPos == pos || gwg->FindHumanPath(pos, curShootingPos, 6) != 0xFF)
            {
                shootingPos = curShootingPos;
                break;
            }
        }

        // Wurde ein Punkt gefunden?
        if(shootingPos.isValid())
        {
            // Richtung, in die geschossen wird, bestimmen (natürlich die entgegengesetzte nehmen)
            shooting_dir = Direction(d + doffset + 3);
            // dorthingehen
            state = STATE_HUNTER_FINDINGSHOOTINGPOINT;
            HandleStateFindingShootingPoint();
        } else
        {
            // kein Punkt gefunden --> nach Hause gehen
            StartWalkingHome();
            WalkHome();
        }

    } else
    {
        // Weg dorthin suchen
        unsigned char dir = gwg->FindHumanPath(pos, animal->GetPos(), MAX_HUNTING_DISTANCE);
        if(dir != INVALID_DIR)
        {
            // Weg gefunden, dann hinlaufen
            StartWalking(Direction::fromInt(dir));
        } else
        {
            // kein Weg gefunden --> nach Hause laufen
            StartWalkingHome();
            WalkHome();
        }
    }
}

void nofHunter::HandleStateFindingShootingPoint()
{
    // Are we there yet?
    if(shootingPos == pos)
    {
        // Is the animal ready?
        if(animal->IsReadyForShooting())
        {
            // fire!
            state = STATE_HUNTER_SHOOTING;
            current_ev = GetEvMgr().AddEvent(this, 16, 1);
        } else if(animal->IsGettingReadyForShooting())
            current_ev = GetEvMgr().AddEvent(this, 15, 2); // Give the animal some time for getting ready
        else
        {
            // Something went wrong, animal is doing something else?
            StartWalkingHome();
            WalkHome();
        }
    } else
    {
        // Weg dorthin suchen
        unsigned char dir = gwg->FindHumanPath(pos, shootingPos, 6);
        if(dir != INVALID_DIR)
        {
            // Weg gefunden, dann hinlaufen
            StartWalking(Direction::fromInt(dir));
        } else
        {
            // kein Weg gefunden --> nach Hause laufen
            StartWalkingHome();
            WalkHome();
        }
    }
}

void nofHunter::HandleStateShooting()
{
    // Tier muss sterben
    animal->Die();
    // zum Kadaver laufen, um ihn auszunehmen
    state = STATE_HUNTER_WALKINGTOCADAVER;
    HandleStateWalkingToCadaver();
}

void nofHunter::HandleStateWalkingToCadaver()
{
    // Sind wir schon da?
    if(animal->GetPos() == pos)
    {
        // dann ausnehmen
        state = STATE_HUNTER_EVISCERATING;
        current_ev = GetEvMgr().AddEvent(this, 80, 1);
    } else
    {
        // Weg dorthin suchen
        unsigned char dir = gwg->FindHumanPath(pos, animal->GetPos(), 6);
        if(dir != INVALID_DIR)
        {
            // Weg gefunden, dann hinlaufen
            StartWalking(Direction::fromInt(dir));
        } else
        {
            // kein Weg gefunden --> nach Hause laufen
            StartWalkingHome();
            WalkHome();
        }
    }
}

void nofHunter::HandleStateEviscerating()
{
    // Tier verschwinden lassen
    gwg->RemoveFigure(pos, animal);
    // Tier vernichten
    animal->Eviscerated();
    animal->Destroy();
    deletePtr(animal);
    // Fleisch in die Hand nehmen
    ware = GD_MEAT;
    // und zurück zur Hütte
    StartWalkingHome();
    WalkHome();
}

void nofHunter::StartWalkingHome()
{
    // Lebt das Tier noch, müssen wir ihm Bescheid sagen
    if(animal)
    {
        animal->StopHunting();
        animal = NULL;
    }

    state = STATE_WALKINGHOME;
}

void nofHunter::WalkHome()
{
    // Sind wir zu Hause angekommen? (genauer an der Flagge !!)
    MapPoint homeFlagPos = gwg->GetNeighbour(workplace->GetPos(), Direction::SOUTHEAST);
    if(pos == homeFlagPos)
    {
        // Weiteres übernimmt nofBuildingWorker
        WorkingReady();
        return;
    }

    // Weg suchen und ob wir überhaupt noch nach Hause kommen (Toleranz bei dem Weg mit einberechnen,
    // damit er nicht einfach rumirrt und wegstirbt, wenn er einmal ein paar Felder zu weit gelaufen ist)
    unsigned char dir = gwg->FindHumanPath(pos, homeFlagPos, MAX_HUNTING_DISTANCE + MAX_HUNTING_DISTANCE / 4);
    if(dir == INVALID_DIR)
    {
        // Kein Weg führt mehr nach Hause--> Rumirren
        AbrogateWorkplace();
        StartWandering();
        Wander();
    } else
    {
        // Alles ok, wir können hinlaufen
        StartWalking(Direction::fromInt(dir));
    }
}

void nofHunter::AnimalLost()
{
    animal = NULL;

    switch(state)
    {
        default: return;
        case STATE_HUNTER_CHASING:
        case STATE_HUNTER_FINDINGSHOOTINGPOINT:
        case STATE_HUNTER_WALKINGTOCADAVER:
        {
            // nach Haue laufen
            StartWalkingHome();
        }
        break;
        case STATE_HUNTER_SHOOTING:
        case STATE_HUNTER_EVISCERATING:
        {
            // Arbeits-Event abmelden
            GetEvMgr().RemoveEvent(current_ev);
            // Nach Hause laufen
            StartWalkingHome();
            WalkHome();
        }
        break;
    }
}

void nofHunter::WorkAborted()
{
    // Tier Bescheid sagen
    if(state == STATE_HUNTER_CHASING || state == STATE_HUNTER_FINDINGSHOOTINGPOINT || state == STATE_HUNTER_SHOOTING)
    {
        if(animal)
        {
            animal->StopHunting();
            animal = NULL;
        }
    }
}
