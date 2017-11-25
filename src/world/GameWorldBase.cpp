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
#include "world/GameWorldBase.h"
#include "BQCalculator.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "addons/const_addons.h"
#include "buildings/nobHarborBuilding.h"
#include "buildings/nobMilitary.h"
#include "figures/nofPassiveSoldier.h"
#include "helpers/containerUtils.h"
#include "lua/LuaInterfaceGame.h"
#include "network/GameClient.h"
#include "notifications/NodeNote.h"
#include "notifications/PlayerNodeNote.h"
#include "pathfinding/FreePathFinder.h"
#include "pathfinding/RoadPathFinder.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noMovable.h"
#include "gameData/BuildingProperties.h"
#include "gameData/GameConsts.h"
#include "gameData/MapConsts.h"
#include "gameData/TerrainData.h"
#include <boost/bind.hpp>
#include <boost/foreach.hpp>

GameWorldBase::GameWorldBase(const std::vector<GamePlayer>& players, const GlobalGameSettings& gameSettings, EventManager& em)
    : roadPathFinder(new RoadPathFinder(*this)), freePathFinder(new FreePathFinder(*this)), players(players), gameSettings(gameSettings),
      em(em), gi(NULL)
{
    BuildingProperties::Init();
}

GameWorldBase::~GameWorldBase() {}

void GameWorldBase::Init(const MapExtent& mapSize, LandscapeType lt)
{
    World::Init(mapSize, lt);
    freePathFinder->Init(mapSize);
}

void GameWorldBase::InitAfterLoad()
{
    RTTR_FOREACH_PT(MapPoint, GetSize())
        RecalcBQ(pt);
}

GamePlayer& GameWorldBase::GetPlayer(const unsigned id)
{
    RTTR_Assert(id < GetNumPlayers());
    return players[id];
}

const GamePlayer& GameWorldBase::GetPlayer(const unsigned id) const
{
    RTTR_Assert(id < GetNumPlayers());
    return players[id];
}

unsigned GameWorldBase::GetNumPlayers() const
{
    return players.size();
}

bool GameWorldBase::IsSinglePlayer() const
{
    bool foundPlayer = false;
    BOOST_FOREACH(const PlayerInfo& player, players)
    {
        if(player.ps == PS_OCCUPIED)
        {
            if(foundPlayer)
                return false;
            else
                foundPlayer = true;
        }
    }
    return true;
}

bool GameWorldBase::IsRoadAvailable(const bool boat_road, const MapPoint pt) const
{
    // Hindernisse
    if(GetNode(pt).obj)
    {
        BlockingManner bm = GetNode(pt).obj->GetBM();
        if(bm != BlockingManner::None)
            return false;
    }

    // dont build on the border
    if(GetNode(pt).boundary_stones[0])
        return false;

    for(unsigned z = 0; z < 6; ++z)
    {
        // Roads around charburner piles are not possible
        if(GetNO(GetNeighbour(pt, Direction::fromInt(z)))->GetBM() == BlockingManner::NothingAround)
            return false;

        // Other roads at this point?
        if(GetPointRoad(pt, Direction::fromInt(z)))
            return false;
    }

    // Terrain (unterscheiden, ob Wasser und Landweg)
    if(!boat_road)
    {
        bool flagPossible = false;

        for(unsigned char i = 0; i < 6; ++i)
        {
            TerrainBQ bq = TerrainData::GetBuildingQuality(GetRightTerrain(pt, Direction::fromInt(i)));
            if(bq == TerrainBQ::DANGER)
                return false;
            else if(bq != TerrainBQ::NOTHING)
                flagPossible = true;
        }

        return flagPossible;
    } else
    {
        // Beim Wasserweg muss um den Punkt herum Wasser sein
        if(!IsWaterPoint(pt))
            return false;
    }

    return true;
}

bool GameWorldBase::RoadAlreadyBuilt(const bool boat_road, const MapPoint start, const std::vector<Direction>& route)
{
    MapPoint tmp(start);
    for(unsigned i = 0; i < route.size() - 1; ++i)
    {
        // Richtiger Weg auf diesem Punkt?
        if(!GetPointRoad(tmp, route[i]))
            return false;

        tmp = GetNeighbour(tmp, route[i]);
    }
    return true;
}

bool GameWorldBase::IsOnRoad(const MapPoint& pt) const
{
    for(unsigned roadDir = 0; roadDir < 3; ++roadDir)
        if(GetRoad(pt, roadDir))
            return true;
    for(unsigned roadDir = 0; roadDir < 3; ++roadDir)
        if(GetRoad(GetNeighbour(pt, Direction::fromInt(roadDir)), roadDir))
            return true;
    return false;
}

bool GameWorldBase::IsFlagAround(const MapPoint& pt) const
{
    for(unsigned i = 0; i < 6; ++i)
    {
        if(GetNO(GetNeighbour(pt, Direction::fromInt(i)))->GetBM() == BlockingManner::Flag)
            return true;
    }
    return false;
}

void GameWorldBase::RecalcBQForRoad(const MapPoint pt)
{
    RecalcBQ(pt);

    for(unsigned i = 3; i < 6; ++i)
        RecalcBQ(GetNeighbour(pt, Direction::fromInt(i)));
}

namespace {
bool IsMilBldOfOwner(const GameWorldBase& gwb, MapPoint pt, unsigned char owner)
{
    return gwb.IsMilitaryBuildingOnNode(pt, false) && (gwb.GetNode(pt).owner == owner);
}
} // namespace

bool GameWorldBase::IsMilitaryBuildingNearNode(const MapPoint nPt, const unsigned char player) const
{
    // Im Umkreis von 4 Punkten ein Militärgebäude suchen
    return CheckPointsInRadius(nPt, 4, boost::bind(IsMilBldOfOwner, boost::cref(*this), _1, player + 1), false);
}

bool GameWorldBase::IsMilitaryBuildingOnNode(const MapPoint pt, bool attackBldsOnly) const
{
    const noBase* obj = GetNO(pt);
    if(obj->GetType() == NOP_BUILDING || obj->GetType() == NOP_BUILDINGSITE)
    {
        BuildingType buildingType = static_cast<const noBaseBuilding*>(obj)->GetBuildingType();
        if(BuildingProperties::IsMilitary(buildingType))
            return true;
        if(!attackBldsOnly && (buildingType == BLD_HEADQUARTERS || buildingType == BLD_HARBORBUILDING))
            return true;
    }

    return false;
}

sortedMilitaryBlds GameWorldBase::LookForMilitaryBuildings(const MapPoint pt, unsigned short radius) const
{
    return militarySquares.GetBuildingsInRange(pt, radius);
}

noFlag* GameWorldBase::GetRoadFlag(MapPoint pt, Direction& dir, unsigned prevDir)
{
    // Getting a flag is const
    const noFlag* flag = const_cast<const GameWorldBase*>(this)->GetRoadFlag(pt, dir, prevDir);
    // However we self are not const, so we allow returning a non-const flag pointer
    return const_cast<noFlag*>(flag);
}

const noFlag* GameWorldBase::GetRoadFlag(MapPoint pt, Direction& dir, unsigned prevDir) const
{
    unsigned i = 0;

    while(true)
    {
        // suchen, wo der Weg weitergeht
        for(i = 0; i < Direction::COUNT; ++i)
        {
            if(i != prevDir && GetPointRoad(pt, Direction::fromInt(i)))
                break;
        }

        if(i == 6)
            return NULL;

        pt = GetNeighbour(pt, Direction::fromInt(i));

        // endlich am Ende des Weges und an einer Flagge angekommen?
        if(GetNO(pt)->GetType() == NOP_FLAG)
        {
            dir = Direction(i + 3);
            return GetSpecObj<noFlag>(pt);
        }
        prevDir = (i + 3) % 6;
    }
}

Position GameWorldBase::GetNodePos(const MapPoint pt) const
{
    Position result;
    result.x = pt.x * TR_W;
    if(pt.y & 1)
        result.x += TR_W / 2;
    result.y = pt.y * TR_H - HEIGHT_FACTOR * (GetNode(pt).altitude + 0x0A);
    return result;
}

void GameWorldBase::VisibilityChanged(const MapPoint pt, unsigned player, Visibility oldVis, Visibility newVis)
{
    GetNotifications().publish(PlayerNodeNote(PlayerNodeNote::Visibility, pt, player));
}

/// Verändert die Höhe eines Punktes und die damit verbundenen Schatten
void GameWorldBase::AltitudeChanged(const MapPoint pt)
{
    RecalcBQAroundPointBig(pt);
    GetNotifications().publish(NodeNote(NodeNote::Altitude, pt));
}

void GameWorldBase::RecalcBQAroundPoint(const MapPoint pt)
{
    RecalcBQ(pt);
    for(unsigned char i = 0; i < 6; ++i)
        RecalcBQ(GetNeighbour(pt, Direction::fromInt(i)));
}

void GameWorldBase::RecalcBQAroundPointBig(const MapPoint pt)
{
    // Point and radius 1
    RecalcBQAroundPoint(pt);
    // And radius 2
    for(unsigned i = 0; i < 12; ++i)
        RecalcBQ(GetNeighbour2(pt, i));
}

Visibility GameWorldBase::CalcVisiblityWithAllies(const MapPoint pt, const unsigned char player) const
{
    const MapNode& node = GetNode(pt);
    Visibility best_visibility = node.fow[player].visibility;

    if(best_visibility == VIS_VISIBLE)
        return best_visibility;

    /// Teamsicht aktiviert?
    if(GetGGS().teamView)
    {
        const GamePlayer& curPlayer = GetPlayer(player);
        // Dann prüfen, ob Teammitglieder evtl. eine bessere Sicht auf diesen Punkt haben
        for(unsigned i = 0; i < GetNumPlayers(); ++i)
        {
            if(i != player && curPlayer.IsAlly(i))
            {
                if(node.fow[i].visibility > best_visibility)
                    best_visibility = node.fow[i].visibility;
            }
        }
    }

    return best_visibility;
}

bool GameWorldBase::IsCoastalPointToSeaWithHarbor(const MapPoint pt) const
{
    unsigned short sea = GetSeaFromCoastalPoint(pt);
    if(sea)
    {
        const unsigned numHarborPts = GetNumHarborPoints();
        for(unsigned i = 1; i <= numHarborPts; i++)
        {
            if(IsHarborAtSea(i, sea))
                return true;
        }
    }
    return false;
}

template<typename T_IsHarborOk>
unsigned GameWorldBase::GetHarborInDir(const MapPoint pt, const unsigned origin_harborId, const ShipDirection& dir,
                                       const unsigned char player, T_IsHarborOk isHarborOk) const
{
    RTTR_Assert(origin_harborId);

    // Herausfinden, in welcher Richtung sich dieser Punkt vom Ausgangspunkt unterscheidet
    unsigned char coastal_point_dir = 0xFF;
    const MapPoint hbPt = GetHarborPoint(origin_harborId);

    for(unsigned char i = 0; i < 6; ++i)
    {
        if(GetNeighbour(hbPt, Direction::fromInt(i)) == pt)
        {
            coastal_point_dir = i;
            break;
        }
    }

    RTTR_Assert(coastal_point_dir != 0xff);

    unsigned short seaId = GetSeaId(origin_harborId, Direction::fromInt(coastal_point_dir));
    const std::vector<HarborPos::Neighbor>& neighbors = GetHarborNeighbors(origin_harborId, dir);

    for(unsigned i = 0; i < neighbors.size(); ++i)
    {
        if(IsHarborAtSea(neighbors[i].id, seaId) && isHarborOk(neighbors[i].id))
            return neighbors[i].id;
    }

    // Nichts gefunden
    return 0;
}

/// Functor that returns true, when the owner of a point is set and different than the player
struct IsPointOwnerDifferent
{
    typedef unsigned char result_type;
    const GameWorldBase& gwb;
    // Owner to compare. Note that owner=0 --> No owner => owner=player+1
    const unsigned char cmpOwner;

    IsPointOwnerDifferent(const GameWorldBase& gwb, const unsigned char player) : gwb(gwb), cmpOwner(player + 1) {}

    bool operator()(const MapPoint pt, unsigned /*distance*/) const
    {
        const unsigned char owner = gwb.GetNode(pt).owner;
        return owner != 0 && owner != cmpOwner;
    }
};

/// Ist es an dieser Stelle für einen Spieler möglich einen Hafen zu bauen
bool GameWorldBase::IsHarborPointFree(const unsigned harborId, const unsigned char player) const
{
    MapPoint hbPos(GetHarborPoint(harborId));

    // Überprüfen, ob das Gebiet in einem bestimmten Radius entweder vom Spieler oder gar nicht besetzt ist außer wenn der Hafen und die
    // Flagge im Spielergebiet liegen
    MapPoint flagPos = GetNeighbour(hbPos, Direction::SOUTHEAST);
    if(GetNode(hbPos).owner != player + 1 || GetNode(flagPos).owner != player + 1)
    {
        if(CheckPointsInRadius(hbPos, 4, IsPointOwnerDifferent(*this, player), false))
            return false;
    }

    return GetNode(hbPos).bq == BQ_HARBOR;
}

/// Sucht freie Hafenpunkte, also wo noch ein Hafen gebaut werden kann
unsigned GameWorldBase::GetNextFreeHarborPoint(const MapPoint pt, const unsigned origin_harborId, const ShipDirection& dir,
                                               const unsigned char player) const
{
    return GetHarborInDir(pt, origin_harborId, dir, player, boost::bind(&GameWorldBase::IsHarborPointFree, this, _1, player));
}

/// Bestimmt für einen beliebigen Punkt auf der Karte die Entfernung zum nächsten Hafenpunkt
unsigned GameWorldBase::CalcDistanceToNearestHarbor(const MapPoint pos) const
{
    unsigned min_distance = 0xffffffff;
    for(unsigned i = 1; i <= GetNumHarborPoints(); ++i)
        min_distance = std::min(min_distance, this->CalcDistance(pos, GetHarborPoint(i)));

    return min_distance;
}

/// returns true when a harborpoint is in SEAATTACK_DISTANCE for figures!
bool GameWorldBase::IsAHarborInSeaAttackDistance(const MapPoint pos) const
{
    for(unsigned i = 1; i <= GetNumHarborPoints(); ++i)
    {
        if(CalcDistance(pos, GetHarborPoint(i)) < SEAATTACK_DISTANCE)
        {
            if(FindHumanPath(pos, GetHarborPoint(i), SEAATTACK_DISTANCE) != 0xff)
                return true;
        }
    }
    return false;
}

std::vector<unsigned> GameWorldBase::GetUsableTargetHarborsForAttack(const MapPoint targetPt, std::vector<bool>& use_seas,
                                                                     const unsigned char player_attacker) const
{
    // Walk to the flag of the bld/harbor. Important to check because in some locations where the coast is north of the harbor this might be
    // blocked
    const MapPoint flagPt = GetNeighbour(targetPt, Direction::SOUTHEAST);
    std::vector<unsigned> harbor_points;
    // Check each possible harbor
    for(unsigned curHbId = 1; curHbId <= GetNumHarborPoints(); ++curHbId)
    {
        const MapPoint harborPt = GetHarborPoint(curHbId);

        if(CalcDistance(harborPt, targetPt) > SEAATTACK_DISTANCE)
            continue;

        // Not attacking this harbor and harbors block?
        if(targetPt != harborPt && GetGGS().getSelection(AddonId::SEA_ATTACK) == 1)
        {
            // Does an enemy harbor exist at current harbor spot? -> Can't attack through this harbor spot
            const nobHarborBuilding* hb = GetSpecObj<nobHarborBuilding>(harborPt);
            if(hb && GetPlayer(player_attacker).IsAttackable(hb->GetPlayer()))
                continue;
        }

        // add seaIds from which we can actually attack the harbor
        bool harborinlist = false;
        for(unsigned z = 0; z < 6; ++z)
        {
            const unsigned short seaId = GetSeaId(curHbId, Direction::fromInt(z));
            if(!seaId)
                continue;
            // checks previously tested sea ids to skip pathfinding
            bool previouslytested = false;
            for(unsigned k = 0; k < z; k++)
            {
                if(seaId == GetSeaId(curHbId, Direction::fromInt(k)))
                {
                    previouslytested = true;
                    break;
                }
            }
            if(previouslytested)
                continue;

            // Can figures reach flag from coast
            const MapPoint coastalPt = GetCoastalPoint(curHbId, seaId);
            if((flagPt == coastalPt) || FindHumanPath(flagPt, coastalPt, SEAATTACK_DISTANCE) != INVALID_DIR)
            {
                use_seas.at(seaId - 1) = true;
                if(!harborinlist)
                {
                    harbor_points.push_back(curHbId);
                    harborinlist = true;
                }
            }
        }
    }
    return harbor_points;
}

std::vector<unsigned short> GameWorldBase::GetFilteredSeaIDsForAttack(const MapPoint targetPt,
                                                                      const std::vector<unsigned short>& usableSeas,
                                                                      const unsigned char player_attacker) const
{
    // Walk to the flag of the bld/harbor. Important to check because in some locations where the coast is north of the harbor this might be
    // blocked
    const MapPoint flagPt = GetNeighbour(targetPt, Direction::SOUTHEAST);
    std::vector<unsigned short> confirmedSeaIds;
    // Check each possible harbor
    for(unsigned curHbId = 1; curHbId <= GetNumHarborPoints(); ++curHbId)
    {
        const MapPoint harborPt = GetHarborPoint(curHbId);

        if(CalcDistance(harborPt, targetPt) > SEAATTACK_DISTANCE)
            continue;

        // Not attacking this harbor and harbors block?
        if(targetPt != harborPt && GetGGS().getSelection(AddonId::SEA_ATTACK) == 1)
        {
            // Does an enemy harbor exist at current harbor spot? -> Can't attack through this harbor spot
            const nobHarborBuilding* hb = GetSpecObj<nobHarborBuilding>(harborPt);
            if(hb && GetPlayer(player_attacker).IsAttackable(hb->GetPlayer()))
                continue;
        }

        for(unsigned z = 0; z < 6; ++z)
        {
            const unsigned short seaId = GetSeaId(curHbId, Direction::fromInt(z));
            if(!seaId)
                continue;
            // sea id is not in compare list or already confirmed? -> skip rest
            if(!helpers::contains(usableSeas, seaId) || helpers::contains(confirmedSeaIds, seaId))
                continue;

            // checks previously tested sea ids to skip pathfinding
            bool previouslytested = false;
            for(unsigned k = 0; k < z; k++)
            {
                if(seaId == GetSeaId(curHbId, Direction::fromInt(k)))
                {
                    previouslytested = true;
                    break;
                }
            }
            if(previouslytested)
                continue;

            // Can figures reach flag from coast
            MapPoint coastalPt = GetCoastalPoint(curHbId, seaId);
            if((flagPt == coastalPt) || FindHumanPath(flagPt, coastalPt, SEAATTACK_DISTANCE) != INVALID_DIR)
            {
                confirmedSeaIds.push_back(seaId);
                // all sea ids confirmed? return without changes
                if(confirmedSeaIds.size() == usableSeas.size())
                    return confirmedSeaIds;
            }
        }
    }
    return confirmedSeaIds;
}

/// Liefert Hafenpunkte im Umkreis von einem bestimmten Militärgebäude
std::vector<unsigned> GameWorldBase::GetHarborPointsAroundMilitaryBuilding(const MapPoint pt) const
{
    std::vector<unsigned> harbor_points;
    // Nach Hafenpunkten in der Nähe des angegriffenen Gebäudes suchen
    // Alle unsere Häfen durchgehen
    for(unsigned i = 1; i <= GetNumHarborPoints(); ++i)
    {
        const MapPoint harborPt = GetHarborPoint(i);

        if(CalcDistance(harborPt, pt) <= SEAATTACK_DISTANCE)
        {
            // Wird ein Weg vom Militärgebäude zum Hafen gefunden bzw. Ziel = Hafen?
            if(pt == harborPt)
                harbor_points.push_back(i);
            else if(FindHumanPath(pt, harborPt, SEAATTACK_DISTANCE) != 0xff)
                harbor_points.push_back(i);
        }
    }
    return harbor_points;
}

/// Gibt Anzahl oder geschätzte Stärke(rang summe + anzahl) der verfügbaren Soldaten die zu einem Schiffsangriff starten können von einer
/// bestimmten sea id aus
unsigned GameWorldBase::GetNumSoldiersForSeaAttackAtSea(const unsigned char player_attacker, unsigned short seaid, bool returnCount) const
{
    // Liste alle Militärgebäude des Angreifers, die Soldaten liefern
    std::vector<nobHarborBuilding::SeaAttackerBuilding> buildings;
    unsigned attackercount = 0;
    // Angrenzende Häfen des Angreifers an den entsprechenden Meeren herausfinden
    const std::list<nobHarborBuilding*>& harbors = GetPlayer(player_attacker).GetBuildingRegister().GetHarbors();
    for(std::list<nobHarborBuilding*>::const_iterator it = harbors.begin(); it != harbors.end(); ++it)
    {
        // Bestimmen, ob Hafen an einem der Meere liegt, über die sich auch die gegnerischen
        // Hafenpunkte erreichen lassen
        if(!IsHarborAtSea((*it)->GetHarborPosID(), seaid))
            continue;

        std::vector<nobHarborBuilding::SeaAttackerBuilding> tmp = (*it)->GetAttackerBuildingsForSeaIdAttack();
        buildings.insert(buildings.begin(), tmp.begin(), tmp.end());
    }

    // Die Soldaten aus allen Militärgebäuden sammeln
    for(unsigned i = 0; i < buildings.size(); ++i)
    {
        // Soldaten holen
        std::vector<nofPassiveSoldier*> tmp_soldiers = buildings[i].building->GetSoldiersForAttack(buildings[i].harbor->GetPos());

        // Überhaupt welche gefunden?
        if(tmp_soldiers.empty())
            continue;

        // Soldaten hinzufügen
        for(unsigned j = 0; j < tmp_soldiers.size(); ++j)
        {
            if(returnCount)
                attackercount++;
            else
                attackercount += (tmp_soldiers[j]->GetRank() + 1); // private is rank 0 -> increase by 1-5
        }
    }
    return attackercount;
}

/// Sucht verfügbare Soldaten, um dieses Militärgebäude mit einem Seeangriff anzugreifen
std::vector<GameWorldBase::PotentialSeaAttacker> GameWorldBase::GetSoldiersForSeaAttack(const unsigned char player_attacker,
                                                                                        const MapPoint pt) const
{
    std::vector<GameWorldBase::PotentialSeaAttacker> attackers;
    // sea attack abgeschaltet per addon?
    if(GetGGS().getSelection(AddonId::SEA_ATTACK) == 2)
        return attackers;
    // Do we have an attackble military building?
    const nobBaseMilitary* milBld = GetSpecObj<nobBaseMilitary>(pt);
    if(!milBld || !milBld->IsAttackable(player_attacker))
        return attackers;
    std::vector<bool> use_seas(GetNumSeas());

    // Mögliche Hafenpunkte in der Nähe des Gebäudes
    std::vector<unsigned> defender_harbors = GetUsableTargetHarborsForAttack(pt, use_seas, player_attacker);

    // Liste alle Militärgebäude des Angreifers, die Soldaten liefern
    std::vector<nobHarborBuilding::SeaAttackerBuilding> buildings;

    // Angrenzende Häfen des Angreifers an den entsprechenden Meeren herausfinden
    const std::list<nobHarborBuilding*>& harbors = GetPlayer(player_attacker).GetBuildingRegister().GetHarbors();
    for(std::list<nobHarborBuilding*>::const_iterator it = harbors.begin(); it != harbors.end(); ++it)
    {
        // Bestimmen, ob Hafen an einem der Meere liegt, über die sich auch die gegnerischen
        // Hafenpunkte erreichen lassen
        bool is_at_sea = false;
        for(unsigned i = 0; i < 6; ++i)
        {
            const unsigned short seaId = GetSeaId((*it)->GetHarborPosID(), Direction::fromInt(i));
            if(seaId && use_seas[seaId - 1])
            {
                is_at_sea = true;
                break;
            }
        }

        if(!is_at_sea)
            continue;

        std::vector<nobHarborBuilding::SeaAttackerBuilding> tmp = (*it)->GetAttackerBuildingsForSeaAttack(defender_harbors);
        for(std::vector<nobHarborBuilding::SeaAttackerBuilding>::iterator itBld = tmp.begin(); itBld != tmp.end(); ++itBld)
        {
            // Check if the building was already inserted
            std::vector<nobHarborBuilding::SeaAttackerBuilding>::iterator oldBldIt =
              std::find_if(buildings.begin(), buildings.end(), nobHarborBuilding::SeaAttackerBuilding::CmpBuilding(itBld->building));
            if(oldBldIt == buildings.end())
            {
                // Not found -> Add
                buildings.push_back(*itBld);
            } else if(oldBldIt->distance > itBld->distance
                      || (oldBldIt->distance == itBld->distance && oldBldIt->harbor->GetObjId() > itBld->harbor->GetObjId()))
            {
                // New distance is smaller (with tie breaker for async prevention) -> update
                *oldBldIt = *itBld;
            }
        }
    }

    // Die Soldaten aus allen Militärgebäuden sammeln
    for(std::vector<nobHarborBuilding::SeaAttackerBuilding>::const_iterator it = buildings.begin(); it != buildings.end(); ++it)
    {
        // Soldaten holen
        std::vector<nofPassiveSoldier*> tmp_soldiers = it->building->GetSoldiersForAttack(it->harbor->GetPos());

        // Soldaten hinzufügen
        for(std::vector<nofPassiveSoldier*>::const_iterator itSoldier = tmp_soldiers.begin(); itSoldier != tmp_soldiers.end(); ++itSoldier)
        {
            RTTR_Assert(std::find_if(attackers.begin(), attackers.end(), PotentialSeaAttacker::CmpSoldier(*itSoldier)) == attackers.end());
            PotentialSeaAttacker pa(*itSoldier, it->harbor, it->distance);
            attackers.push_back(pa);
        }
    }

    return attackers;
}

void GameWorldBase::RecalcBQ(const MapPoint pt)
{
    BQCalculator calcBQ(*this);
    if(SetBQ(pt, calcBQ(pt, boost::bind(&GameWorldBase::IsOnRoad, this, _1))))
        GetNotifications().publish(NodeNote(NodeNote::BQ, pt));
}
