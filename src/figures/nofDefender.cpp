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
#include "defines.h" // IWYU pragma: keep
#include "nofDefender.h"

#include "nofAttacker.h"
#include "buildings/nobMilitary.h"
#include "GameClient.h"
#include "Random.h"
#include "nodeObjs/noFighting.h"
#include "nofPassiveSoldier.h"
#include "SerializedGameData.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep


nofDefender::nofDefender(const MapPoint pos, const unsigned char player,
                         nobBaseMilitary* const home, const unsigned char rank, nofAttacker* const attacker)
    : nofActiveSoldier(pos, player, home, rank, STATE_DEFENDING_WALKINGTO), attacker(attacker)
{
}

nofDefender::nofDefender(nofPassiveSoldier* other, nofAttacker* const attacker)
    : nofActiveSoldier(*other, STATE_DEFENDING_WALKINGTO), attacker(attacker)
{
}

void nofDefender::Serialize_nofDefender(SerializedGameData& sgd) const
{
    Serialize_nofActiveSoldier(sgd);

    if(state != STATE_FIGUREWORK)
        sgd.PushObject(attacker, true);
}

nofDefender::nofDefender(SerializedGameData& sgd, const unsigned obj_id) : nofActiveSoldier(sgd, obj_id)
{
    if(state != STATE_FIGUREWORK)
        attacker = sgd.PopObject<nofAttacker>(GOT_NOF_ATTACKER);
    else
        attacker = 0;
}

/// wenn man gelaufen ist
void nofDefender::Walked()
{
    // Was bestimmtes machen, je nachdem welchen Status wir gerade haben
    switch(state)
    {
        case STATE_DEFENDING_WALKINGTO:
        {
            // Mit Angreifer den Kampf beginnen
            gwg->AddFigure(new noFighting(attacker, this), pos);
            state = STATE_FIGHTING;
            attacker->FightVsDefenderStarted();

        } break;
        case STATE_DEFENDING_WALKINGFROM:
        {
            // Ist evtl. unser Heimatgebäude zerstört?
            if(!building)
            {
                // Rumirren
                attacker = 0;
                state = STATE_FIGUREWORK;
                StartWandering();
                Wander();
                return;
            }

            // Zu Hause angekommen
            // Ist evtl. wieder ein Angreifer in der Zwischenzeit an der Fahne angekommen?
            if(attacker)
            {
                // dann umdrehen und wieder rausgehen
                state = STATE_DEFENDING_WALKINGTO;
                StartWalking(4);
            }
            else
            {
                // mich von der Landkarte tilgen
                gwg->RemoveFigure(this, pos);
                nobBaseMilitary* bld = building;
                // mich zum Gebäude wieder hinzufügen
                RTTR_Assert(bld->GetDefender() == this); // I should be the defender
                bld->AddActiveSoldier(this);
                RTTR_Assert(!bld->GetDefender()); // No defender anymore
            }

        } break;
        default:
            break;
    }
}

/// Wenn ein Heimat-Militärgebäude bei Missionseinsätzen zerstört wurde
void nofDefender::HomeDestroyed()
{
    building = 0;

    switch(state)
    {
        case STATE_DEFENDING_WAITING:
        {
            // Hier muss sofort reagiert werden, da man steht
            attacker = NULL;
            // Rumirren
            state = STATE_FIGUREWORK;
            StartWandering();
            Wander();
        } break;
        case STATE_DEFENDING_WALKINGTO:
        case STATE_DEFENDING_WALKINGFROM:
        {
            attacker = NULL;
            // Rumirren
            StartWandering();
            state = STATE_FIGUREWORK;
        } break;
        case STATE_FIGHTING:
        {
            // Die normale Tätigkeit wird erstmal fortgesetzt (Laufen, Kämpfen, wenn er schon an der Fahne ist
            // wird er auch nicht mehr zurückgehen)
        } break;
        default:
            break;
    }
}

void nofDefender::HomeDestroyedAtBegin()
{
    building = NULL;

    state = STATE_FIGUREWORK;

    // Rumirren
    StartWandering();
    StartWalking(RANDOM.Rand(__FILE__, __LINE__, GetObjId(), 6));
}


/// Wenn ein Kampf gewonnen wurde
void nofDefender::WonFighting()
{
	
	//addon BattlefieldPromotion active? -> increase rank!
	if(GAMECLIENT.GetGGS().isEnabled(AddonId::BATTLEFIELD_PROMOTION))
		IncreaseRank();
    // Angreifer tot
    attacker = NULL;

    // Ist evtl. unser Heimatgebäude zerstört?
    if(!building)
    {
        // Rumirren
        state = STATE_FIGUREWORK;
        StartWandering();
        Wander();

        return;
    }

    // Neuen Angreifer rufen
    attacker = building->FindAttackerNearBuilding();
    if(attacker)
    {
        // Ein Angreifer gefunden, dann warten wir auf ihn, bis er kommt
        state = STATE_DEFENDING_WAITING;
    }
    else
    {
        // Kein Angreifer gefunden, dann gehen wir wieder in unser Gebäude
        state = STATE_DEFENDING_WALKINGFROM;
        StartWalking(1);
    }
}

/// Wenn ein Kampf verloren wurde (Tod)
void nofDefender::LostFighting()
{
    attacker = NULL;

    // Gebäude Bescheid sagen, falls es noch existiert
    if(building)
    {
        building->NoDefender();
        // Ist das ein "normales" Militärgebäude?
        if(building->GetBuildingType() >= BLD_BARRACKS && building->GetBuildingType() <= BLD_FORTRESS)
        {
            // Wenn ich nicht der lezte Soldat da drinnen war, dann können noch neue kommen..
            RTTR_Assert(dynamic_cast<nobBaseMilitary*>(building));
            if(static_cast<nobMilitary*>(building)->GetTroopsCount())
                static_cast<nobMilitary*>(building)->RegulateTroops();
        }
        building = NULL;
    }
}


void nofDefender::AttackerArrested()
{
    // Neuen Angreifer suchen
    attacker = building->FindAttackerNearBuilding();
    if(!attacker)
    {
        // Kein Angreifer gefunden, dann gehen wir wieder in unser Gebäude
        state = STATE_DEFENDING_WALKINGFROM;
        StartWalking(1);
    }
}

/// The derived classes regain control after a fight of nofActiveSoldier
void nofDefender::FreeFightEnded()
{
    // This is not supposed to happen
    RTTR_Assert(false);
    nofActiveSoldier::FreeFightEnded();
}
