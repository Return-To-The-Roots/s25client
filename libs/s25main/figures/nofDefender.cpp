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

#include "nofDefender.h"

#include "GlobalGameSettings.h"
#include "SerializedGameData.h"
#include "addons/const_addons.h"
#include "buildings/nobMilitary.h"
#include "nofAttacker.h"
#include "nofPassiveSoldier.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include "nodeObjs/noFighting.h"
#include "gameData/BuildingProperties.h"

nofDefender::nofDefender(const MapPoint pos, const unsigned char player, nobBaseMilitary& home,
                         const unsigned char rank, nofAttacker& attacker)
    : nofActiveSoldier(pos, player, home, rank, SoldierState::DefendingWalkingTo), attacker(&attacker)
{}

nofDefender::nofDefender(const nofPassiveSoldier& other, nofAttacker& attacker)
    : nofActiveSoldier(other, SoldierState::DefendingWalkingTo), attacker(&attacker)
{}

void nofDefender::Serialize(SerializedGameData& sgd) const
{
    nofActiveSoldier::Serialize(sgd);

    if(state != SoldierState::FigureWork)
        sgd.PushObject(attacker, true);
}

nofDefender::nofDefender(SerializedGameData& sgd, const unsigned obj_id) : nofActiveSoldier(sgd, obj_id)
{
    if(state != SoldierState::FigureWork)
        attacker = sgd.PopObject<nofAttacker>(GO_Type::NofAttacker);
    else
        attacker = nullptr;
}

/// wenn man gelaufen ist
void nofDefender::Walked()
{
    // Was bestimmtes machen, je nachdem welchen Status wir gerade haben
    switch(state)
    {
        case SoldierState::DefendingWalkingTo:
        {
            // Mit Angreifer den Kampf beginnen
            world->AddFigure(pos, std::make_unique<noFighting>(*attacker, *this));
            state = SoldierState::Fighting;
            attacker->FightVsDefenderStarted();
        }
        break;
        case SoldierState::DefendingWalkingFrom:
        {
            // Ist evtl. unser Heimatgebäude zerstört?
            if(!building)
            {
                // Rumirren
                attacker = nullptr;
                state = SoldierState::FigureWork;
                StartWandering();
                Wander();
                return;
            }

            // Zu Hause angekommen
            // Ist evtl. wieder ein Angreifer in der Zwischenzeit an der Fahne angekommen?
            if(attacker)
            {
                // dann umdrehen und wieder rausgehen
                state = SoldierState::DefendingWalkingTo;
                StartWalking(Direction::SouthEast);
            } else
            {
                nobBaseMilitary* bld = building;
                RTTR_Assert(bld->GetDefender() == this); // I should be the defender
                bld->AddActiveSoldier(world->RemoveFigure(pos, *this));
                RTTR_Assert(!bld->GetDefender()); // No defender anymore
            }
        }
        break;
        default: break;
    }
}

/// Wenn ein Heimat-Militärgebäude bei Missionseinsätzen zerstört wurde
void nofDefender::HomeDestroyed()
{
    building = nullptr;

    switch(state)
    {
        case SoldierState::DefendingWaiting:
        {
            // Hier muss sofort reagiert werden, da man steht
            attacker = nullptr;
            // Rumirren
            state = SoldierState::FigureWork;
            StartWandering();
            Wander();
        }
        break;
        case SoldierState::DefendingWalkingTo:
        case SoldierState::DefendingWalkingFrom:
        {
            attacker = nullptr;
            // Rumirren
            StartWandering();
            state = SoldierState::FigureWork;
        }
        break;
        case SoldierState::Fighting:
        {
            // Die normale Tätigkeit wird erstmal fortgesetzt (Laufen, Kämpfen, wenn er schon an der Fahne ist
            // wird er auch nicht mehr zurückgehen)
        }
        break;
        default: break;
    }
}

void nofDefender::HomeDestroyedAtBegin()
{
    building = nullptr;

    state = SoldierState::FigureWork;

    // Rumirren
    StartWandering();
    StartWalking(RANDOM_ENUM(Direction));
}

/// Wenn ein Kampf gewonnen wurde
void nofDefender::WonFighting()
{
    // addon BattlefieldPromotion active? -> increase rank!
    if(world->GetGGS().isEnabled(AddonId::BATTLEFIELD_PROMOTION))
        IncreaseRank();
    // Angreifer tot
    attacker = nullptr;

    // Ist evtl. unser Heimatgebäude zerstört?
    if(!building)
    {
        // Rumirren
        state = SoldierState::FigureWork;
        StartWandering();
        Wander();

        return;
    }

    // Neuen Angreifer rufen
    attacker = building->FindAttackerNearBuilding();
    if(attacker)
    {
        // Ein Angreifer gefunden, dann warten wir auf ihn, bis er kommt
        state = SoldierState::DefendingWaiting;
    } else
    {
        // Kein Angreifer gefunden, dann gehen wir wieder in unser Gebäude
        state = SoldierState::DefendingWalkingFrom;
        StartWalking(Direction::NorthWest);
    }
}

/// Wenn ein Kampf verloren wurde (Tod)
void nofDefender::LostFighting()
{
    attacker = nullptr;

    // Gebäude Bescheid sagen, falls es noch existiert
    if(building)
    {
        building->NoDefender();
        // Ist das ein "normales" Militärgebäude?
        if(BuildingProperties::IsMilitary(building->GetBuildingType()))
        {
            // Wenn ich nicht der lezte Soldat da drinnen war, dann können noch neue kommen..
            RTTR_Assert(dynamic_cast<nobBaseMilitary*>(building));
            if(static_cast<nobMilitary*>(building)->GetNumTroops())
                static_cast<nobMilitary*>(building)->RegulateTroops();
        }
        building = nullptr;
    }
}

void nofDefender::AttackerArrested()
{
    // Neuen Angreifer suchen
    attacker = building->FindAttackerNearBuilding();
    if(!attacker)
    {
        // Kein Angreifer gefunden, dann gehen wir wieder in unser Gebäude
        state = SoldierState::DefendingWalkingFrom;
        StartWalking(Direction::NorthWest);
    }
}

/// The derived classes regain control after a fight of nofActiveSoldier
void nofDefender::FreeFightEnded()
{
    throw std::logic_error("Should not participate inf ree fights");
}
