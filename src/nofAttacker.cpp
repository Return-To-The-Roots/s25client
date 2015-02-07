// $Id: nofAttacker.cpp
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
#include "nofAttacker.h"
#include "nofDefender.h"
#include "nofAggressiveDefender.h"
#include "nofPassiveSoldier.h"
#include "nobMilitary.h"
#include "Loader.h"
#include "GameClient.h"
#include "GameConsts.h"
#include "Random.h"
#include "GameWorld.h"
#include "noFighting.h"
#include "SerializedGameData.h"
#include "nobBaseWarehouse.h"
#include "MapGeometry.h"
#include "PostMsg.h"
#include "nobHarborBuilding.h"
#include "noShip.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/// Nach einer bestimmten Zeit, in der der Angreifer an der Flagge des Gebäudes steht, blockt er den Weg
/// nur benutzt bei STATE_ATTACKING_WAITINGFORDEFENDER
/// Dieses Konstante gibt an, wie lange, nachdem er anfängt da zu stehen, er blockt
const unsigned BLOCK_OFFSET = 10;


nofAttacker::nofAttacker(nofPassiveSoldier* other, nobBaseMilitary* const attacked_goal)
    : nofActiveSoldier(*other, STATE_ATTACKING_WALKINGTOGOAL), attacked_goal(attacked_goal),
      should_haunted(GAMECLIENT.GetPlayer(attacked_goal->GetPlayer())->ShouldSendDefender()), blocking_event(0),
      harbor_x(0xffff), harbor_y(0xffff), ship_x(0xffff), ship_y(0xffff), ship_obj_id(0)
{
    // Dem Haus Bescheid sagen
    static_cast<nobMilitary*>(building)->SoldierOnMission(other, this);
    // Das Haus soll uns rausschicken
    building->AddLeavingFigure(this);
    // Dem Ziel Bescheid sagen
    attacked_goal->LinkAggressor(this);
}

nofAttacker::nofAttacker(nofPassiveSoldier* other, nobBaseMilitary* const attacked_goal, const nobHarborBuilding* const harbor)
    : nofActiveSoldier(*other, STATE_SEAATTACKING_GOTOHARBOR),

      attacked_goal(attacked_goal),
      should_haunted(GAMECLIENT.GetPlayer(attacked_goal->GetPlayer())->ShouldSendDefender()), blocking_event(0),
      harbor_x(harbor->GetX()), harbor_y(harbor->GetY()), ship_x(0xffff), ship_y(0xffff), ship_obj_id(0)
{
    // Dem Haus Bescheid sagen
    static_cast<nobMilitary*>(building)->SoldierOnMission(other, this);
    // Das Haus soll uns rausschicken
    building->AddLeavingFigure(this);
    // Dem Ziel Bescheid sagen
    attacked_goal->LinkAggressor(this);
}


nofAttacker::~nofAttacker()
{

    //unsigned char oplayer = (player == 0) ? 1 : 0;
    //assert(GameClient::inst().GetPlayer(oplayer)->GetFirstWH()->Test(this) == false);
}

void nofAttacker::Destroy_nofAttacker()
{
    Destroy_nofActiveSoldier();

    /*unsigned char oplayer = (player == 0) ? 1 : 0;
    assert(GameClient::inst().GetPlayer(oplayer)->GetFirstWH()->Test(this) == false);*/
}


void nofAttacker::Serialize_nofAttacker(SerializedGameData* sgd) const
{
    Serialize_nofActiveSoldier(sgd);

    if(state != STATE_WALKINGHOME && state != STATE_FIGUREWORK)
    {
        sgd->PushObject(attacked_goal, false);
        sgd->PushBool(should_haunted);
        sgd->PushUnsignedShort(radius);

        if(state == STATE_ATTACKING_WAITINGFORDEFENDER)
            sgd->PushObject(blocking_event, true);

        sgd->PushUnsignedShort(harbor_x);
        sgd->PushUnsignedShort(harbor_y);
        sgd->PushUnsignedShort(ship_x);
        sgd->PushUnsignedShort(ship_y);
        sgd->PushUnsignedInt(ship_obj_id);
    }
}

nofAttacker::nofAttacker(SerializedGameData* sgd, const unsigned obj_id) : nofActiveSoldier(sgd, obj_id)
{
    if(state != STATE_WALKINGHOME && state != STATE_FIGUREWORK)
    {
        attacked_goal = sgd->PopObject<nobBaseMilitary>(GOT_UNKNOWN);
        should_haunted = sgd->PopBool();
        radius = sgd->PopUnsignedShort();

        if(state == STATE_ATTACKING_WAITINGFORDEFENDER)
            blocking_event = sgd->PopObject<EventManager::Event>(GOT_EVENT);

        harbor_x = sgd->PopUnsignedShort();
        harbor_y = sgd->PopUnsignedShort();
        ship_x = sgd->PopUnsignedShort();
        ship_y = sgd->PopUnsignedShort();
        ship_obj_id = sgd->PopUnsignedInt();
    }
    else
    {
        attacked_goal = 0;
        should_haunted = 0;
        radius = 0;
    }

}


void nofAttacker::Walked()
{
    ExpelEnemies();

    // Was bestimmtes machen, je nachdem welchen Status wir gerade haben
    switch(state)
    {
        default:
            nofActiveSoldier::Walked();
            break;
        case STATE_ATTACKING_WALKINGTOGOAL:
        {
            MissAttackingWalk();
        } break;
        case STATE_ATTACKING_ATTACKINGFLAG:
        {
            // Ist evtl. das Zielgebäude zerstört?
            if(!attacked_goal)
            {
                // Nach Hause gehen
                ReturnHomeMissionAttacking();

                return;
            }

            unsigned short flag_x = attacked_goal->GetFlag()->GetX(),
                           flag_y = attacked_goal->GetFlag()->GetY();
            //assert(enemy->GetGOT() == GOT_NOF_DEFENDER);
            // Are we at the flag?

            nofDefender* defender = NULL;
            // Look for defenders at this position
            for(list<noBase*>::iterator it = gwg->GetFigures(flag_x, flag_y).begin();
                    it != gwg->GetFigures(flag_x, flag_y).end(); ++it)
            {
                if((*it)->GetGOT() == GOT_NOF_DEFENDER)
                {
                    // Is the defender waiting at the flag?
                    // (could be wandering around or something)
                    if(static_cast<nofDefender*>(*it)->IsWaitingAtFlag())
                    {
                        defender = static_cast<nofDefender*>(*it);
                    }
                }
            }


            if(x == flag_x && y == flag_y)
            {


                if(defender)
                {
                    // Start fight with the defender
                    gwg->AddFigure(new noFighting(this, defender), x, y);

                    // Set the appropriate states
                    state = STATE_ATTACKING_FIGHTINGVSDEFENDER;
                    defender->FightStarted();
                }
                else
                    // No defender at the flag?
                    // -> Order new defenders or capture the building
                    ContinueAtFlag();
            }
            else
            {
                if( (dir = gwg->FindHumanPath(x, y, flag_x, flag_y, 5, true)) == 0xFF)
                {
                    // es wurde kein Weg mehr gefunden --> neues Plätzchen suchen und warten
                    state = STATE_ATTACKING_WALKINGTOGOAL;
                    MissAttackingWalk();
                    // der Verteidiger muss darüber informiert werden
                    if(defender)
                        defender->AttackerArrested();
                }
                else
                {
                    // Hinlaufen
                    StartWalking(dir);
                }
            }
        } break;
        case STATE_ATTACKING_CAPTURINGFIRST:
        {
            // Ist evtl. das Zielgebäude zerstört?
            if(!attacked_goal)
            {
                // Nach Hause gehen
                ReturnHomeMissionAttacking();

                return;
            }


            // Wenn schon welche drin sind, ist wieder ein feindlicher reingegangen
            if(attacked_goal->DefendersAvailable())
            {
                // Wieder rausgehen, Platz reservieren
                if(attacked_goal->GetGOT() == GOT_NOB_MILITARY)
                    static_cast<nobMilitary*>(attacked_goal)->StopCapturing();

                state = STATE_ATTACKING_WALKINGTOGOAL;
                StartWalking(4);
                return;
            }
            else
            {
                // Ist das Gebäude ein "normales Militärgebäude", das wir da erobert haben?
                if(attacked_goal->GetBuildingType() >= BLD_BARRACKS &&
                        attacked_goal->GetBuildingType() <= BLD_FORTRESS)
                {
                    // Meinem Heimatgebäude Bescheid sagen, dass ich nicht mehr komme (falls es noch eins gibt)
                    if(building)
                        building->SoldierLost(this);
                    // Ggf. Schiff Bescheid sagen (Schiffs-Angreifer)
                    if(ship_obj_id)
                        CancelAtShip();
                    // Gebäude einnehmen
                    static_cast<nobMilitary*>(attacked_goal)->Capture(player);
                    // Bin nun kein Angreifer mehr
                    attacked_goal->UnlinkAggressor(this);
                    // Das ist nun mein neues zu Hause
                    building = attacked_goal;
                    // mich zum Gebäude hinzufügen und von der Karte entfernen
                    attacked_goal->AddActiveSoldier(this);
                    gwg->RemoveFigure(this, x, y);
                    // ggf. weitere Soldaten rufen, damit das Gebäude voll wird
                    static_cast<nobMilitary*>(attacked_goal)->NeedOccupyingTroops(player);
                }
                // oder ein Hauptquartier oder Hafen?
                else
                {
                    // Inform the owner of the building
                    if(GameClient::inst().GetPlayerID() == attacked_goal->GetPlayer())
                    {
                        if(attacked_goal->GetGOT() == GOT_NOB_HQ)
                            GameClient::inst().SendPostMessage(new
                                                               ImagePostMsgWithLocation(_("Our headquarters was destroyed!"), PMC_MILITARY, x, y, attacked_goal->GetBuildingType(), attacked_goal->GetNation()));
                        else
                            GameClient::inst().SendPostMessage(new
                                                               ImagePostMsgWithLocation(_("This harbor building was destroyed"), PMC_MILITARY, x, y, attacked_goal->GetBuildingType(), attacked_goal->GetNation()));
                    }

                    // abreißen
                    nobBaseMilitary* tmp_goal = attacked_goal;  // attacked_goal wird evtl auf 0 gesetzt!
                    tmp_goal->Destroy();
                    delete tmp_goal;
                    attacked_goal = NULL;
                    ReturnHomeMissionAttacking();
                }
            }
        } break;
        case STATE_ATTACKING_CAPTURINGNEXT:
        {
            CapturingWalking();
        } break;

        case STATE_SEAATTACKING_GOTOHARBOR: // geht von seinem Heimatmilitärgebäude zum Starthafen
        {

            // Gucken, ob der Abflughafen auch noch steht und sich in unserer Hand befindet
            bool valid_harbor = true;
            noBase* hb = gwg->GetNO(harbor_x, harbor_y);
            if(hb->GetGOT() != GOT_NOB_HARBORBUILDING)
                valid_harbor = false;
            else if(static_cast<nobHarborBuilding*>(hb)->GetPlayer() != player)
                valid_harbor = false;

            // Nicht mehr oder das angegriffene Gebäude kaputt? Dann müssen wir die ganze Aktion abbrechen
            if(!valid_harbor || !attacked_goal)
            {
                // Dann gehen wir halt wieder nach Hause
                ReturnHomeMissionAttacking();
                return;
            }

            // Sind wir schon da?
            if(x == harbor_x && y == harbor_y)
            {
                // Uns zum Hafen hinzufügen
                state = STATE_SEAATTACKING_WAITINHARBOR;
                gwg->RemoveFigure(this, x, y);
                gwg->GetSpecObj<nobHarborBuilding>(x, y)->AddSeaAttacker(this);

                return;
            }

            // Erstmal Flagge ansteuern
            MapCoord harbor_flag_x = gwg->GetXA(harbor_x, harbor_y, 4);
            MapCoord harbor_flag_y = gwg->GetYA(harbor_x, harbor_y, 4);

            // Wenn wir an der Flagge bereits sind, in den Hafen eintreten
            if(x == harbor_flag_x && y == harbor_flag_y)
                StartWalking(1);
            else
            {
                // Weg zum Hafen suchen
                unsigned char dir = gwg->FindHumanPath(x, y, harbor_flag_x, harbor_flag_y, MAX_ATTACKING_RUN_DISTANCE, false, NULL);
                if(dir == 0xff)
                {
                    // Kein Weg gefunden? Dann auch abbrechen!
                    ReturnHomeMissionAttacking();
                    return;
                }

                // Und schön weiterlaufen
                StartWalking(dir);
            }

        } break;
        case STATE_SEAATTACKING_WAITINHARBOR: // wartet im Hafen auf das ankommende Schiff
        {
        } break;
        case STATE_SEAATTACKING_ONSHIP: // befindet sich auf dem Schiff auf dem Weg zum Zielpunkt
        {
            // Auweia, das darf nicht passieren
            assert(false);
        } break;
        case STATE_SEAATTACKING_RETURNTOSHIP: // befindet sich an der Zielposition auf dem Weg zurück zum Schiff
        {
            HandleState_SeaAttack_ReturnToShip();
        } break;
    }
}


/// Wenn ein Heimat-Militärgebäude bei Missionseinsätzen zerstört wurde
void nofAttacker::HomeDestroyed()
{
    switch(state)
    {
        case STATE_ATTACKING_WAITINGAROUNDBUILDING:
        {
            // Hier muss sofort reagiert werden, da man steht

            // Angreifer muss zusätzlich seinem Ziel Bescheid sagen
            attacked_goal->UnlinkAggressor(this);

            // Ggf. Schiff Bescheid sagen (Schiffs-Angreifer)
            if(ship_obj_id)
                CancelAtShip();

            // Rumirren
            building = 0;
            state = STATE_FIGUREWORK;
            StartWandering();
            Wander();

            // und evtl einen Nachrücker für diesen Platz suchen
            attacked_goal->SendSuccessor(x, y, radius, dir);



        } break;

        default:
        {
            //  Die normale Tätigkeit wird erstmal fortgesetzt (Laufen, Kämpfen, wenn er schon an der Fahne ist
            // wird er auch nicht mehr zurückgehen)
            building = 0;
        } break;
    }

}

void nofAttacker::HomeDestroyedAtBegin()
{
    building = 0;

    // angegriffenem Gebäude Bescheid sagen, dass wir doch nicht mehr kommen
    if(attacked_goal)
    {
        attacked_goal->UnlinkAggressor(this);
        attacked_goal = 0;
    }

    state = STATE_FIGUREWORK;

    // Rumirren
    StartWandering();
    StartWalking(RANDOM.Rand(__FILE__, __LINE__, obj_id, 6));
}

/// Sagt dem Heimatgebäude Bescheid, dass er nicht mehr nach Hause kommen wird
void nofAttacker::CancelAtHomeMilitaryBuilding()
{
    if(building)
        building->SoldierLost(this);
}

/// Wenn ein Kampf gewonnen wurde
void nofAttacker::WonFighting()
{
	//addon BattlefieldPromotion active? -> increase rank!
	if(GameClient::inst().GetGGS().isEnabled(ADDON_BATTLEFIELD_PROMOTION))
		IncreaseRank();
    // Ist evtl. unser Heimatgebäude zerstört?
    if(!building && state != STATE_ATTACKING_FIGHTINGVSDEFENDER)
    {
        // Dann dem Ziel Bescheid sagen, falls es existiert (evtl. wurdes zufällig zur selben Zeit zerstört)
        if(attacked_goal)
        {
            attacked_goal->UnlinkAggressor(this);
            attacked_goal = 0;
        }

        // Ggf. Schiff Bescheid sagen (Schiffs-Angreifer)
        if(ship_obj_id)
            CancelAtShip();

        // Rumirren
        state = STATE_FIGUREWORK;
        StartWandering();
        Wander();

        return;
    }


    // Ist evtl. unser Ziel-Gebäude zerstört?
    if(!attacked_goal)
    {
        // Nach Hause gehen
        ReturnHomeMissionAttacking();

        return;
    }


    ContinueAtFlag();


}

/// Doesn't find a defender at the flag -> Send defenders or capture it
void nofAttacker::ContinueAtFlag()
{
    // Greifen wir grad ein Gebäude an?
	if(state == STATE_ATTACKING_FIGHTINGVSDEFENDER || (attacked_goal && state == STATE_FIGHTING && attacked_goal->GetFlag()->GetX()==x && attacked_goal->GetFlag()->GetY()==y))
    {
        // Dann neuen Verteidiger rufen
        if(attacked_goal->CallDefender(this))
        {
            // Verteidiger gefunden --> hinstellen und auf ihn warten
            SwitchStateAttackingWaitingForDefender();
        }
        else
        {
			//check for soldiers of other non-friendly non-owner players to fight
			if(FindEnemiesNearby(attacked_goal->GetPlayer()))
				return;
            // kein Verteidiger gefunden --> ins Gebäude laufen und es erobern
            state = STATE_ATTACKING_CAPTURINGFIRST;
            StartWalking(1);

            // Normalen Militärgebäuden schonmal Bescheid sagen
            if(attacked_goal->GetGOT() == GOT_NOB_MILITARY)
                static_cast<nobMilitary*>(attacked_goal)->PrepareCapturing();
        }
    }
    else
    {
        // weiterlaufen
        state = STATE_ATTACKING_WALKINGTOGOAL;
        MissAttackingWalk();
    }
}

/// Wenn ein Kampf verloren wurde (Tod)
void nofAttacker::LostFighting()
{
    // Meinem zu Hause Bescheid sagen, dass ich nicht mehr lebe (damit neue Truppen reinkönnen)
    // falls das Gebäude noch existiert
    if(building)
        building->SoldierLost(this);

    // Angreifer müssen zusätzlich ihrem Ziel Bescheid sagen
    if(attacked_goal)
    {
        attacked_goal->UnlinkAggressor(this);
        attacked_goal = 0;
    }

    // Ggf. Schiff Bescheid sagen
    if(ship_obj_id)
        this->CancelAtShip();
}


void nofAttacker::ReturnHomeMissionAttacking()
{
    // Zielen Bescheid sagen
    InformTargetsAboutCancelling();
    // Schiffsangreifer?
    if(ship_obj_id)
    {
        state = STATE_SEAATTACKING_RETURNTOSHIP;
        HandleState_SeaAttack_ReturnToShip();
    }
    else
        // Und nach Hause gehen
        ReturnHome();
}

void nofAttacker::MissAttackingWalk()
{
    // Ist evtl. unser Heimatgebäude zerstört?
    if(!building)
    {
        // Dann dem Ziel Bescheid sagen, falls es existiert (evtl. wurdes zufällig zur selben Zeit zerstört)
        if(attacked_goal)
        {
            attacked_goal->UnlinkAggressor(this);
            attacked_goal = 0;
        }

        // Ggf. Schiff Bescheid sagen (Schiffs-Angreifer)
        if(ship_obj_id)
            CancelAtShip();

        // Rumirren
        state = STATE_FIGUREWORK;
        StartWandering();
        Wander();


        return;
    }

    // Gibts das Ziel überhaupt noch?
    if(!attacked_goal)
    {
        ReturnHomeMissionAttacking();
        return;
    }

    /*// Is it still a hostile destination?
    // (Could be captured in the meantime)
    if(!players->getElement(player)->IsPlayerAttackable(attacked_goal->GetPlayer()))
    {
        ReturnHomeMissionAttacking();
        return;
    }*/


    // Eine Position rund um das Militärgebäude suchen
    unsigned short goal_x, goal_y;
    attacked_goal->FindAnAttackerPlace(goal_x, goal_y, radius, this);

    // Keinen Platz mehr gefunden?
    if(goal_x == 0xFFFF)
    {
        // Dann nach Haus gehen
        ReturnHomeMissionAttacking();
        return;
    }

    // Sind wir evtl schon da?
    if(x == goal_x && y == goal_y)
    {
        ReachedDestination();
        return;
    }

    // Find all sorts of enemies (attackers, aggressive defenders..) nearby
    if(FindEnemiesNearby())
        // Enemy found -> abort, because nofActiveSoldier handles all things now
        return;

    // Haben wir noch keinen Feind?
    // Könnte mir noch ein neuer Verteidiger entgegenlaufen?
    TryToOrderAggressiveDefender();



    // Ansonsten Weg zum Ziel suchen
    dir = gwg->FindHumanPath(x, y, goal_x, goal_y, MAX_ATTACKING_RUN_DISTANCE, true);
    // Keiner gefunden? Nach Hause gehen
    if(dir == 0xff)
    {
        ReturnHomeMissionAttacking();
        return;
    }

    // Start walking
    StartWalking(dir);
}

/// Ist am Militärgebäude angekommen
void nofAttacker::ReachedDestination()
{
    // Sind wir direkt an der Flagge?
    if(x == attacked_goal->GetFlag()->GetX() && y == attacked_goal->GetFlag()->GetY())
    {
        // Building already captured? Continue capturing
        // This can only be a far away attacker
        if(attacked_goal->GetPlayer() == player)
        {
            state = STATE_ATTACKING_CAPTURINGNEXT;
            CapturingWalking();
            static_cast<nobMilitary*>(attacked_goal)->FarAwayAttackerReachedGoal(this);
            return;
        }

        // Post schicken "Wir werden angegriffen" TODO evtl. unschön, da jeder Attacker das dann aufruft
        if(attacked_goal->GetPlayer() == GameClient::inst().GetPlayerID())
            GAMECLIENT.SendPostMessage(
                new ImagePostMsgWithLocation(_("We are under attack!"), PMC_MILITARY, x, y,
                                             attacked_goal->GetBuildingType(), attacked_goal->GetNation()));

        // Dann Verteidiger rufen
        if(attacked_goal->CallDefender(this))
        {
            // Verteidiger gefunden --> hinstellen und auf ihn warten
            SwitchStateAttackingWaitingForDefender();


        }
        else
        {
            // kein Verteidiger gefunden --> ins Gebäude laufen und es erobern
            state = STATE_ATTACKING_CAPTURINGFIRST;
            StartWalking(1);
			// Normalen Militärgebäuden schonmal Bescheid sagen
            if(attacked_goal->GetGOT() == GOT_NOB_MILITARY)
                static_cast<nobMilitary*>(attacked_goal)->PrepareCapturing();
        }
    }
    else
    {
        // dann hinstellen und warten, bis wir an die Reihe kommmen mit Kämpfen und außerdem diesen Platz
        // reservieren, damit sich kein anderer noch hier hinstellt
        state = STATE_ATTACKING_WAITINGAROUNDBUILDING;
        // zur Flagge hin ausrichten
        MapCoord flag_x = attacked_goal->GetFlag()->GetX();
        MapCoord flag_y = attacked_goal->GetFlag()->GetY();
        if(y == flag_y && x <= flag_x) dir = 3;
        else if(y == flag_y && x > flag_x) dir = 0;
        else if(y < flag_y && x < flag_x) dir = 4;
        else if(y < flag_y && x >  flag_x) dir = 5;
        else if(y > flag_y && x < flag_x) dir = 2;
        else if(y > flag_y && x >  flag_x) dir = 1;
        else if(x ==  flag_x)
        {
            if(y < flag_y && !(SafeDiff(y, flag_y) & 1)) dir = 4;
            else if(y < flag_y && (SafeDiff(y, flag_y) & 1))
            {
                if(y & 1) dir = 5; else dir = 4;
            }
            else if(y > flag_y && !(SafeDiff(y, flag_y) & 1)) dir = 2;
            else if(y > flag_y && (SafeDiff(y, flag_y) & 1))
            {
                if(y & 1) dir = 1; else dir = 2;
            }
        }
    }
}


/// Versucht, eine aggressiven Verteidiger für uns zu bestellen
void nofAttacker::TryToOrderAggressiveDefender()
{
    // Haben wir noch keinen Gegner?
    // Könnte mir noch ein neuer Verteidiger entgegenlaufen?
    if(should_haunted && state >= STATE_ATTACKING_WALKINGTOGOAL)
    {
        // 20%ige Chance, dass wirklich jemand angreift
        if(RANDOM.Rand(__FILE__, __LINE__, obj_id, 10) < 2)
        {
            // Militärgebäude in der Nähe abgrasen
            std::list<nobBaseMilitary*> buildings;
            gwg->LookForMilitaryBuildings(buildings, x, y, 2);

            for(std::list<nobBaseMilitary*>::iterator it = buildings.begin(); it != buildings.end(); ++it)
            {
                // darf kein HQ sein, außer, das HQ wird selbst angegriffen, darf nicht weiter weg als 15 sein
                // und es darf natürlich auch der entsprechende Feind sein, aber es darf auch nicht derselbe Spieler
                // wie man selbst sein, da das Gebäude ja z.B. schon erobert worden sein kann
                if(((*it)->GetBuildingType() != BLD_HEADQUARTERS || (*it) == attacked_goal)
                        && gwg->CalcDistance(x, y, (*it)->GetX(), (*it)->GetY()) < 15
                        && GameClient::inst().GetPlayer(attacked_goal->GetPlayer())->IsAlly((*it)->GetPlayer())  &&
                        GameClient::inst().GetPlayer(player)->IsPlayerAttackable((*it)->GetPlayer()))
                {
                    // ggf. Verteidiger rufen
                    if( ((*it)->SendDefender(this)))
                    {
                        // nun brauchen wir keinen Verteidiger mehr
                        should_haunted = false;
                        break;
                    }
                }
            }
        }
    }
}


void nofAttacker::AttackedGoalDestroyed()
{
    attacked_goal = 0;

    bool was_waiting_for_defender = (state == STATE_ATTACKING_WAITINGFORDEFENDER);

    // Wenn man gerade rumsteht, muss man sich bewegen
    if(state == STATE_ATTACKING_WAITINGFORDEFENDER ||
            state == STATE_ATTACKING_WAITINGAROUNDBUILDING ||
            state == STATE_WAITINGFORFIGHT)
        ReturnHomeMissionAttacking();

    if(was_waiting_for_defender)
    {
        // Block-Event ggf abmelden
        em->RemoveEvent(blocking_event);
        blocking_event = 0;
        gwg->RoadNodeAvailable(x, y);
    }
}

bool nofAttacker::AttackFlag(nofDefender* defender)
{
    // Zur Flagge laufen, findet er einen Weg?
    unsigned char tmp_dir = gwg->FindHumanPath(x, y, attacked_goal->GetFlag()->GetX(),
                            attacked_goal->GetFlag()->GetY(), 3, true);

    if(tmp_dir != 0xFF)
    {
        // alte Richtung für Nachrücker merken
        unsigned char old_dir = dir;

        // Hat er drumrum gewartet?
        bool waiting_around_building = (state == STATE_ATTACKING_WAITINGAROUNDBUILDING);

        // Ja er hat einen Weg gefunden, also hinlaufen

        // Wenn er steht, muss er loslaufen
        if(waiting_around_building)
            StartWalking(tmp_dir);

        state = STATE_ATTACKING_ATTACKINGFLAG;

        // Hatte er ums Gebäude gewartet?
        if(waiting_around_building)
        {
            // evtl. Nachrücker senden
            attacked_goal->SendSuccessor(x, y, radius, old_dir);
        }

        return true;
    }

    return false;
}
void nofAttacker::AttackFlag()
{
    // "Normal" zur Flagge laufen
    state = STATE_ATTACKING_WALKINGTOGOAL;
    MissAttackingWalk();
}


void nofAttacker::CaptureBuilding()
{
    // mit ins Militärgebäude gehen
    state = STATE_ATTACKING_CAPTURINGNEXT;
    // und hinlaufen
    CapturingWalking();
}

void nofAttacker::CapturingWalking()
{
    // Ist evtl. das Zielgebäude zerstört?
    if(!attacked_goal)
    {
        // Nach Hause gehen
        ReturnHomeMissionAttacking();

        return;
    }

    unsigned short flag_x = attacked_goal->GetFlag()->GetX(),
                   flag_y = attacked_goal->GetFlag()->GetY();

    // Sind wir schon im Gebäude?
    if(x == attacked_goal->GetX() && y == attacked_goal->GetY())
    {
        // Meinem alten Heimatgebäude Bescheid sagen (falls es noch existiert)
        if(building)
            building->SoldierLost(this);
        if(ship_obj_id)
            CancelAtShip();
        // mich von der Karte tilgen-
        gwg->RemoveFigure(this, x, y);
        // Das ist nun mein neues zu Hause
        building = attacked_goal;
        // und zum Gebäude hinzufügen
        attacked_goal->AddActiveSoldier(this);

        // Ein erobernder Soldat weniger
        if(attacked_goal->GetBuildingType() >= BLD_BARRACKS &&
                attacked_goal->GetBuildingType() <= BLD_FORTRESS)
            static_cast<nobMilitary*>(attacked_goal)->CapturingSoldierArrived();

        // außerdem aus der Angreiferliste entfernen
        attacked_goal->UnlinkAggressor(this);
        attacked_goal = 0;

    }
    // oder zumindest schonmal an der Flagge?
    else if(x == flag_x && y == flag_y)
    {
        // ins Gebäude laufen
        StartWalking(1);
        // nächsten Angreifer ggf. rufen, der auch reingehen soll
        static_cast<nobMilitary*>(attacked_goal)->NeedOccupyingTroops(player);
    }
    else
    {
        // Ist evtl. unser Heimatgebäude zerstört?
        if(!building)
        {
            // Wenn noch das Ziel existiert (könnte ja zeitgleich abgebrannt worden sein)
            if(attacked_goal)
            {
                // Ziel Bescheid sagen
                attacked_goal->UnlinkAggressor(this);
                // Evtl. neue Besatzer rufen
                static_cast<nobMilitary*>(attacked_goal)->NeedOccupyingTroops(0xFF);

                attacked_goal = 0;
            }

            // Ggf. Schiff Bescheid sagen (Schiffs-Angreifer)
            if(ship_obj_id)
                CancelAtShip();

            // Rumirren
            state = STATE_FIGUREWORK;
            StartWandering();
            Wander();

            return;
        }

        // weiter zur Flagge laufen
        if((dir = gwg->FindHumanPath(x, y, flag_x, flag_y, 10, true)) == 0xFF)
        {
            // auweia, es wurde kein Weg mehr gefunden

            // Evtl. neue Besatzer rufen
            static_cast<nobMilitary*>(attacked_goal)->NeedOccupyingTroops(0xFF);
            // Nach Hause gehen
            ReturnHomeMissionAttacking();
        }
        else
            StartWalking(dir);
    }
}

void nofAttacker::CapturedBuildingFull()
{
    switch(state)
    {
        default:
            break;

        case STATE_ATTACKING_WAITINGAROUNDBUILDING:
        {
            // nach Hause gehen
            ReturnHomeMissionAttacking();
        } break;
        case STATE_ATTACKING_WALKINGTOGOAL:
        case STATE_ATTACKING_WAITINGFORDEFENDER:
        case STATE_ATTACKING_ATTACKINGFLAG:
        case STATE_WAITINGFORFIGHT:
        case STATE_MEETENEMY:
        case STATE_FIGHTING:
        case STATE_SEAATTACKING_GOTOHARBOR: // geht von seinem Heimatmilitärgebäude zum Starthafen
        case STATE_SEAATTACKING_WAITINHARBOR: // wartet im Hafen auf das ankommende Schiff
        case STATE_SEAATTACKING_ONSHIP: // befindet sich auf dem Schiff auf dem Weg zum Zielpunkt
        {
            // Bei allem anderen läuft man oder kämpft --> auf 0 setzen und wenn man fertig
            // mit der jetzigen Aktion ist, entsprechend handeln (nicht die Einnehmer darüber benachrichten, sonst
            // gehen die nicht rein)
            attacked_goal = 0;
        } break;
    }
}

void nofAttacker::StartSucceeding(const unsigned short x, const unsigned short y, const unsigned short new_radius, const unsigned char dir)
{
    // Wir sollen auf diesen Punkt nachrücken
    state = STATE_ATTACKING_WALKINGTOGOAL;

    // Unsere alte Richtung merken für evtl. weitere Nachrücker
    unsigned char old_dir = this->dir;

    // unser alter Platz ist ja nun auch leer, da gibts vielleicht auch einen Nachrücker?
    attacked_goal->SendSuccessor(this->x, this->y, radius, old_dir);

    // Und schonmal loslaufen, da wir ja noch stehen
    MissAttackingWalk();


    // Neuen Radius speichern
    radius = new_radius;
}


void nofAttacker::LetsFight(nofAggressiveDefender* other)
{
    // wir werden jetzt "gejagt"
    should_haunted = false;

    // wenn ich stehe (z.B. vor der Hütte warte), hinlaufen
    if(state == STATE_ATTACKING_WAITINGAROUNDBUILDING)
    {
        state = STATE_ATTACKING_WALKINGTOGOAL;
        MissAttackingWalk();
    }
}

void nofAttacker::AggressiveDefenderLost()
{
    // Wenn wir auf die gewartet hatten, müssen wir uns nun bewegen
    if(state == STATE_WAITINGFORFIGHT)
    {
        state = STATE_ATTACKING_WALKINGTOGOAL;
        MissAttackingWalk();
    }
}

void nofAttacker::SwitchStateAttackingWaitingForDefender()
{
    state = STATE_ATTACKING_WAITINGFORDEFENDER;
    // Blockevent anmelden
    blocking_event = em->AddEvent(this, BLOCK_OFFSET, 5);
}

void nofAttacker::HandleDerivedEvent(const unsigned int id)
{
    // abfragen, nich dass er evtl schon losgelaufen ist wieder, weil das Gebäude abgebrannt wurde etc.
    if(state == STATE_ATTACKING_WAITINGFORDEFENDER)
    {
        // Figuren stoppen
        gwg->StopOnRoads(x, y);
        blocking_event = 0;
    }
}


bool nofAttacker::IsBlockingRoads() const
{
    if(state != STATE_ATTACKING_WAITINGFORDEFENDER)
        return false;

    // Wenn Block-Event schon abgelaufen ist --> blocking_event = 0, da dürfen sich nicht mehr durch
    // wenn es das noch gibt, ist es noch nicht abgelaufen und die Leute können noch durchgehen
    if(!blocking_event)
        return true;
    else
        return false;
}

/// Sagt den verschiedenen Zielen Bescheid, dass wir doch nicht mehr kommen können
void nofAttacker::InformTargetsAboutCancelling()
{
    // Ziel Bescheid sagen, falls es das noch gibt
    if(attacked_goal)
    {
        attacked_goal->UnlinkAggressor(this);
        attacked_goal = 0;
    }
}



/// Startet den Angriff am Landungspunkt vom Schiff
void nofAttacker::StartAttackOnOtherIsland(const MapCoord ship_x, const MapCoord ship_y, const unsigned ship_id)
{
    x = this->ship_x = ship_x;
    y = this->ship_y = ship_y;
    this->ship_obj_id = ship_id;

    state = STATE_ATTACKING_WALKINGTOGOAL;
    on_ship = false;
    // Normal weiterlaufen
    MissAttackingWalk();
}

/// Sea attacker enters harbor and finds no shipping route or no longer has a valid target: set state,target,goal,building to 0 to avoid future problems (and add to harbor inventory)
void nofAttacker::SeaAttackFailedBeforeLaunch()
{
    attacked_goal = 0;
    goal = 0;
    building = 0;
    state = STATE_FIGUREWORK;
}

/// Sagt Schiffsangreifern, dass sie mit dem Schiff zurück fahren
void nofAttacker::StartReturnViaShip()
{
    // remove us from where we are, so nobody will ever draw us :)
    gwg->RemoveFigure(this, this->x, this->y);

    goal = building;
    state = STATE_FIGUREWORK;
    on_ship = true;
}

/// notify sea attackers that they wont return home
void nofAttacker::HomeHarborLost()
{
    goal = 0; //this in combination with telling the home building that the soldier is lost should work just fine
}

/// Für Schiffsangreifer: Sagt dem Schiff Bescheid, dass wir nicht mehr kommen
void nofAttacker::CancelAtShip()
{
    // Alle Figuren durchgehen
    list<noBase*> figures;
    gwg->GetDynamicObjectsFrom(ship_x, ship_y, figures);
    for(list<noBase*>::iterator it = figures.begin(); it.valid(); ++it)
    {
        if((*it)->GetObjId() == ship_obj_id)
        {
            noShip* ship = static_cast<noShip*>(*it);
            ship->SeaAttackerWishesNoReturn();
            return;
        }
    }

}

/// Behandelt das Laufen zurück zum Schiff
void nofAttacker::HandleState_SeaAttack_ReturnToShip()
{
    // Ist evtl. unser Heimatgebäude zerstört?
    if(!building)
    {
        // Rumirren
        state = STATE_FIGUREWORK;
        StartWandering();
        Wander();

        // Schiff Bescheid sagen
        CancelAtShip();

        return;
    }

    // Sind wir schon im Schiff?
    if(x == ship_x && y == ship_y)
    {
        // Alle Figuren durchgehen
        list<noBase*> figures;
        gwg->GetDynamicObjectsFrom(x, y, figures);
        for(list<noBase*>::iterator it = figures.begin(); it.valid(); ++it)
        {
            if((*it)->GetObjId() == ship_obj_id)
            {
                noShip* ship = static_cast<noShip*>(*it);
                // Und von der Landkarte tilgen
                gwg->RemoveFigure(this, x, y);
                // Uns zum Schiff hinzufügen
                ship->AddAttacker(this);


                state = STATE_FIGUREWORK;
                fs = FS_GOTOGOAL;
                StartReturnViaShip();
                return;
            }
        }

        // Kein Schiff gefunden? Das kann eigentlich nich sein!
        // Dann rumirren
        StartWandering();
        state = STATE_FIGUREWORK;
        Wander();
    }
    // oder finden wir gar keinen Weg mehr?
    else if((dir = gwg->FindHumanPath(x, y, ship_x, ship_y, MAX_ATTACKING_RUN_DISTANCE)) == 0xFF)
    {
        // Kein Weg gefunden --> Rumirren
        StartWandering();
        state = STATE_FIGUREWORK;
        Wander();

        // Dem Heimatgebäude Bescheid sagen
        building->SoldierLost(this);
        // Und dem Schiff
        CancelAtShip();
    }
    // oder ist alles ok? :)
    else
    {
        // weiterlaufen
        StartWalking(dir);
    }

}

/// Bricht einen Seeangriff ab
void nofAttacker::CancelSeaAttack()
{
    InformTargetsAboutCancelling();
}


/// The derived classes regain control after a fight of nofActiveSoldier
void nofAttacker::FreeFightEnded()
{
    // Continue with normal walking towards our goal
    state = STATE_ATTACKING_WALKINGTOGOAL;
}

/// Try to start capturing although he is still far away from the destination
/// Returns true if successful
bool nofAttacker::TryToStartFarAwayCapturing(nobMilitary* dest)
{
    // Are we already walking to the destination?
    if(state == STATE_ATTACKING_WALKINGTOGOAL || state == STATE_MEETENEMY || state == STATE_WAITINGFORFIGHT
            || state == STATE_FIGHTING)
    {
        // Not too far away?
        if(gwg->CalcDistance(x, y, dest->GetX(), dest->GetY()) < MAX_FAR_AWAY_CAPTURING_DISTANCE)
            return true;
    }

    return false;
}
