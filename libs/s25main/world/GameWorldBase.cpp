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

#include "world/GameWorldBase.h"
#include "BQCalculator.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "MapGeometry.h"
#include "RttrForeachPt.h"
#include "TradePathCache.h"
#include "addons/const_addons.h"
#include "buildings/nobHarborBuilding.h"
#include "buildings/nobMilitary.h"
#include "figures/nofPassiveSoldier.h"
#include "helpers/EnumRange.h"
#include "helpers/containerUtils.h"
#include "lua/LuaInterfaceGame.h"
#include "notifications/NodeNote.h"
#include "notifications/PlayerNodeNote.h"
#include "pathfinding/FreePathFinder.h"
#include "pathfinding/RoadPathFinder.h"
#include "nodeObjs/noFlag.h"
#include "gameData/BuildingProperties.h"
#include "gameData/GameConsts.h"
#include "gameData/TerrainDesc.h"
#include <utility>

GameWorldBase::GameWorldBase(std::vector<GamePlayer> players, const GlobalGameSettings& gameSettings, EventManager& em)
    : roadPathFinder(new RoadPathFinder(*this)), freePathFinder(new FreePathFinder(*this)), players(std::move(players)),
      gameSettings(gameSettings), em(em), gi(nullptr)
{}

GameWorldBase::~GameWorldBase() = default;

void GameWorldBase::Init(const MapExtent& mapSize, DescIdx<LandscapeDesc> lt)
{
    RTTR_Assert(GetDescription().terrain.size() > 0); // Must have game data initialized
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
    for(const PlayerInfo& player : players)
    {
        if(player.ps == PlayerState::Occupied)
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
    if(GetNode(pt).boundary_stones[BorderStonePos::OnPoint])
        return false;

    for(const auto dir : helpers::EnumRange<Direction>{})
    {
        // Roads around charburner piles are not possible
        if(GetNO(GetNeighbour(pt, dir))->GetBM() == BlockingManner::NothingAround)
            return false;

        // Other roads at this point?
        if(GetPointRoad(pt, dir) != PointRoad::None)
            return false;
    }

    // Terrain (unterscheiden, ob Wasser und Landweg)
    if(!boat_road)
    {
        bool flagPossible = false;

        for(const auto dir : helpers::EnumRange<Direction>{})
        {
            TerrainBQ bq = GetDescription().get(GetRightTerrain(pt, dir)).GetBQ();
            if(bq == TerrainBQ::Danger)
                return false;
            else if(bq != TerrainBQ::Nothing)
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

bool GameWorldBase::RoadAlreadyBuilt(const bool /*boat_road*/, const MapPoint start,
                                     const std::vector<Direction>& route)
{
    MapPoint tmp(start);
    for(unsigned i = 0; i < route.size() - 1; ++i)
    {
        // Richtiger Weg auf diesem Punkt?
        if(GetPointRoad(tmp, route[i]) == PointRoad::None)
            return false;

        tmp = GetNeighbour(tmp, route[i]);
    }
    return true;
}

bool GameWorldBase::IsOnRoad(const MapPoint& pt) const
{
    // This must be fast for BQ calculation so don't use GetVisiblePointRoad
    for(const auto roadDir : helpers::EnumRange<RoadDir>{})
        if(GetRoad(pt, roadDir) != PointRoad::None)
            return true;
    for(const auto roadDir : helpers::EnumRange<RoadDir>{})
    {
        const Direction oppositeDir = getOppositeDir(roadDir);
        if(GetRoad(GetNeighbour(pt, oppositeDir), roadDir) != PointRoad::None)
            return true;
    }
    return false;
}

bool GameWorldBase::IsFlagAround(const MapPoint& pt) const
{
    for(const auto dir : helpers::EnumRange<Direction>{})
    {
        if(GetNO(GetNeighbour(pt, dir))->GetBM() == BlockingManner::Flag)
            return true;
    }
    return false;
}

void GameWorldBase::RecalcBQForRoad(const MapPoint pt)
{
    RecalcBQ(pt);

    for(const Direction dir : {Direction::East, Direction::SouthEast, Direction::SouthWest})
        RecalcBQ(GetNeighbour(pt, dir));
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
    return CheckPointsInRadius(
      nPt, 4, [this, player](auto pt, auto) { return IsMilBldOfOwner(*this, pt, player + 1); }, false);
}

bool GameWorldBase::IsMilitaryBuildingOnNode(const MapPoint pt, bool attackBldsOnly) const
{
    const noBase* obj = GetNO(pt);
    if(obj->GetType() == NodalObjectType::Building || obj->GetType() == NodalObjectType::Buildingsite)
    {
        BuildingType buildingType = static_cast<const noBaseBuilding*>(obj)->GetBuildingType();
        if(BuildingProperties::IsMilitary(buildingType))
            return true;
        if(!attackBldsOnly
           && (buildingType == BuildingType::Headquarters || buildingType == BuildingType::HarborBuilding))
            return true;
    }

    return false;
}

sortedMilitaryBlds GameWorldBase::LookForMilitaryBuildings(const MapPoint pt, unsigned short radius) const
{
    return militarySquares.GetBuildingsInRange(pt, radius);
}

noFlag* GameWorldBase::GetRoadFlag(MapPoint pt, Direction& dir, const helpers::OptionalEnum<Direction> prevDir)
{
    // Getting a flag is const
    const noFlag* flag = const_cast<const GameWorldBase*>(this)->GetRoadFlag(pt, dir, prevDir);
    // However we self are not const, so we allow returning a non-const flag pointer
    return const_cast<noFlag*>(flag);
}

const noFlag* GameWorldBase::GetRoadFlag(MapPoint pt, Direction& dir, helpers::OptionalEnum<Direction> prevDir) const
{
    while(true)
    {
        // suchen, wo der Weg weitergeht
        helpers::OptionalEnum<Direction> nextDir;
        for(const auto i : helpers::EnumRange<Direction>{})
        {
            if(i != prevDir && GetPointRoad(pt, i) != PointRoad::None)
            {
                nextDir = i;
                break;
            }
        }

        if(!nextDir)
            return nullptr;

        pt = GetNeighbour(pt, *nextDir);

        // endlich am Ende des Weges und an einer Flagge angekommen?
        if(GetNO(pt)->GetType() == NodalObjectType::Flag)
        {
            dir = *nextDir + 3u;
            return GetSpecObj<noFlag>(pt);
        }
        prevDir = *nextDir + 3u;
    }
}

Position GameWorldBase::GetNodePos(const MapPoint pt) const
{
    return ::GetNodePos(pt, GetNode(pt).altitude);
}

void GameWorldBase::VisibilityChanged(const MapPoint pt, unsigned player, Visibility /*oldVis*/, Visibility /*newVis*/)
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
    for(const auto dir : helpers::EnumRange<Direction>{})
        RecalcBQ(GetNeighbour(pt, dir));
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

    if(best_visibility == Visibility::Visible)
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
                                       T_IsHarborOk isHarborOk) const
{
    RTTR_Assert(origin_harborId);

    // Herausfinden, in welcher Richtung sich dieser Punkt vom Ausgangspunkt unterscheidet
    helpers::OptionalEnum<Direction> coastal_point_dir;
    const MapPoint hbPt = GetHarborPoint(origin_harborId);

    for(const auto dir : helpers::EnumRange<Direction>{})
    {
        if(GetNeighbour(hbPt, dir) == pt)
        {
            coastal_point_dir = dir;
            break;
        }
    }

    RTTR_Assert(coastal_point_dir);

    unsigned short seaId = GetSeaId(origin_harborId, *coastal_point_dir);
    const std::vector<HarborPos::Neighbor>& neighbors = GetHarborNeighbors(origin_harborId, dir);

    for(auto neighbor : neighbors)
    {
        if(IsHarborAtSea(neighbor.id, seaId) && isHarborOk(neighbor.id))
            return neighbor.id;
    }

    // Nichts gefunden
    return 0;
}

/// Functor that returns true, when the owner of a point is set and different than the player
struct IsPointOwnerDifferent
{
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

    // Überprüfen, ob das Gebiet in einem bestimmten Radius entweder vom Spieler oder gar nicht besetzt ist außer wenn
    // der Hafen und die Flagge im Spielergebiet liegen
    MapPoint flagPos = GetNeighbour(hbPos, Direction::SouthEast);
    if(GetNode(hbPos).owner != player + 1 || GetNode(flagPos).owner != player + 1)
    {
        if(CheckPointsInRadius(hbPos, 4, IsPointOwnerDifferent(*this, player), false))
            return false;
    }

    return GetNode(hbPos).bq == BuildingQuality::Harbor;
}

/// Sucht freie Hafenpunkte, also wo noch ein Hafen gebaut werden kann
unsigned GameWorldBase::GetNextFreeHarborPoint(const MapPoint pt, const unsigned origin_harborId,
                                               const ShipDirection& dir, const unsigned char player) const
{
    return GetHarborInDir(pt, origin_harborId, dir,
                          [this, player](auto harborId) { return this->IsHarborPointFree(harborId, player); });
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
            if(FindHumanPath(pos, GetHarborPoint(i), SEAATTACK_DISTANCE))
                return true;
        }
    }
    return false;
}

std::vector<unsigned> GameWorldBase::GetUsableTargetHarborsForAttack(const MapPoint targetPt,
                                                                     std::vector<bool>& use_seas,
                                                                     const unsigned char player_attacker) const
{
    // Walk to the flag of the bld/harbor. Important to check because in some locations where the coast is north of the
    // harbor this might be blocked
    const MapPoint flagPt = GetNeighbour(targetPt, Direction::SouthEast);
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
            const auto* hb = GetSpecObj<nobHarborBuilding>(harborPt);
            if(hb && GetPlayer(player_attacker).IsAttackable(hb->GetPlayer()))
                continue;
        }

        // add seaIds from which we can actually attack the harbor
        bool harborinlist = false;
        for(const auto dir : helpers::enumRange<Direction>())
        {
            const unsigned short seaId = GetSeaId(curHbId, dir);
            if(!seaId)
                continue;
            // checks previously tested sea ids to skip pathfinding
            bool previouslytested = false;
            for(unsigned k = 0; k < rttr::enum_cast(dir); k++)
            {
                if(seaId == GetSeaId(curHbId, Direction(k)))
                {
                    previouslytested = true;
                    break;
                }
            }
            if(previouslytested)
                continue;

            // Can figures reach flag from coast
            const MapPoint coastalPt = GetCoastalPoint(curHbId, seaId);
            if((flagPt == coastalPt) || FindHumanPath(flagPt, coastalPt, SEAATTACK_DISTANCE))
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
    // Walk to the flag of the bld/harbor. Important to check because in some locations where the coast is north of the
    // harbor this might be blocked
    const MapPoint flagPt = GetNeighbour(targetPt, Direction::SouthEast);
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
            const auto* hb = GetSpecObj<nobHarborBuilding>(harborPt);
            if(hb && GetPlayer(player_attacker).IsAttackable(hb->GetPlayer()))
                continue;
        }

        for(const auto dir : helpers::enumRange<Direction>())
        {
            const unsigned short seaId = GetSeaId(curHbId, dir);
            if(!seaId)
                continue;
            // sea id is not in compare list or already confirmed? -> skip rest
            if(!helpers::contains(usableSeas, seaId) || helpers::contains(confirmedSeaIds, seaId))
                continue;

            // checks previously tested sea ids to skip pathfinding
            bool previouslytested = false;
            for(unsigned k = 0; k < rttr::enum_cast(dir); k++)
            {
                if(seaId == GetSeaId(curHbId, Direction(k)))
                {
                    previouslytested = true;
                    break;
                }
            }
            if(previouslytested)
                continue;

            // Can figures reach flag from coast
            MapPoint coastalPt = GetCoastalPoint(curHbId, seaId);
            if((flagPt == coastalPt) || FindHumanPath(flagPt, coastalPt, SEAATTACK_DISTANCE))
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
            if(pt == harborPt || FindHumanPath(pt, harborPt, SEAATTACK_DISTANCE))
                harbor_points.push_back(i);
        }
    }
    return harbor_points;
}

/// Gibt Anzahl oder geschätzte Stärke(rang summe + anzahl) der verfügbaren Soldaten die zu einem Schiffsangriff starten
/// können von einer bestimmten sea id aus
unsigned GameWorldBase::GetNumSoldiersForSeaAttackAtSea(const unsigned char player_attacker, unsigned short seaid,
                                                        bool returnCount) const
{
    // Liste alle Militärgebäude des Angreifers, die Soldaten liefern
    std::vector<nobHarborBuilding::SeaAttackerBuilding> buildings;
    unsigned attackercount = 0;
    // Angrenzende Häfen des Angreifers an den entsprechenden Meeren herausfinden
    const std::list<nobHarborBuilding*>& harbors = GetPlayer(player_attacker).GetBuildingRegister().GetHarbors();
    for(auto* harbor : harbors)
    {
        // Bestimmen, ob Hafen an einem der Meere liegt, über die sich auch die gegnerischen
        // Hafenpunkte erreichen lassen
        if(!IsHarborAtSea(harbor->GetHarborPosID(), seaid))
            continue;

        std::vector<nobHarborBuilding::SeaAttackerBuilding> tmp = harbor->GetAttackerBuildingsForSeaIdAttack();
        buildings.insert(buildings.begin(), tmp.begin(), tmp.end());
    }

    // Die Soldaten aus allen Militärgebäuden sammeln
    for(auto& building : buildings)
    {
        // Soldaten holen
        std::vector<nofPassiveSoldier*> tmp_soldiers =
          building.building->GetSoldiersForAttack(building.harbor->GetPos());

        // Überhaupt welche gefunden?
        if(tmp_soldiers.empty())
            continue;

        // Soldaten hinzufügen
        for(auto& tmp_soldier : tmp_soldiers)
        {
            if(returnCount)
                attackercount++;
            else
                attackercount += (tmp_soldier->GetRank() + 1); // private is rank 0 -> increase by 1-5
        }
    }
    return attackercount;
}

/// Sucht verfügbare Soldaten, um dieses Militärgebäude mit einem Seeangriff anzugreifen
std::vector<GameWorldBase::PotentialSeaAttacker>
GameWorldBase::GetSoldiersForSeaAttack(const unsigned char player_attacker, const MapPoint pt) const
{
    std::vector<GameWorldBase::PotentialSeaAttacker> attackers;
    // sea attack abgeschaltet per addon?
    if(GetGGS().getSelection(AddonId::SEA_ATTACK) == 2)
        return attackers;
    // Do we have an attackble military building?
    const auto* milBld = GetSpecObj<nobBaseMilitary>(pt);
    if(!milBld || !milBld->IsAttackable(player_attacker))
        return attackers;
    std::vector<bool> use_seas(GetNumSeas());

    // Mögliche Hafenpunkte in der Nähe des Gebäudes
    std::vector<unsigned> defender_harbors = GetUsableTargetHarborsForAttack(pt, use_seas, player_attacker);

    // Liste alle Militärgebäude des Angreifers, die Soldaten liefern
    std::vector<nobHarborBuilding::SeaAttackerBuilding> buildings;

    // Angrenzende Häfen des Angreifers an den entsprechenden Meeren herausfinden
    const std::list<nobHarborBuilding*>& harbors = GetPlayer(player_attacker).GetBuildingRegister().GetHarbors();
    for(auto* harbor : harbors)
    {
        // Bestimmen, ob Hafen an einem der Meere liegt, über die sich auch die gegnerischen
        // Hafenpunkte erreichen lassen
        bool is_at_sea = false;
        for(const auto dir : helpers::EnumRange<Direction>{})
        {
            const unsigned short seaId = GetSeaId(harbor->GetHarborPosID(), dir);
            if(seaId && use_seas[seaId - 1])
            {
                is_at_sea = true;
                break;
            }
        }

        if(!is_at_sea)
            continue;

        std::vector<nobHarborBuilding::SeaAttackerBuilding> tmp =
          harbor->GetAttackerBuildingsForSeaAttack(defender_harbors);
        for(auto& itBld : tmp)
        {
            // Check if the building was already inserted
            auto oldBldIt = std::find_if(buildings.begin(), buildings.end(),
                                         nobHarborBuilding::SeaAttackerBuilding::CmpBuilding(itBld.building));
            if(oldBldIt == buildings.end())
            {
                // Not found -> Add
                buildings.push_back(itBld);
            } else if(oldBldIt->distance > itBld.distance
                      || (oldBldIt->distance == itBld.distance
                          && oldBldIt->harbor->GetObjId() > itBld.harbor->GetObjId()))
            {
                // New distance is smaller (with tie breaker for async prevention) -> update
                *oldBldIt = itBld;
            }
        }
    }

    // Die Soldaten aus allen Militärgebäuden sammeln
    for(const auto& bld : buildings)
    {
        // Soldaten holen
        std::vector<nofPassiveSoldier*> tmp_soldiers = bld.building->GetSoldiersForAttack(bld.harbor->GetPos());

        // Soldaten hinzufügen
        for(nofPassiveSoldier* soldier : tmp_soldiers)
        {
            RTTR_Assert(std::find_if(attackers.begin(), attackers.end(), PotentialSeaAttacker::CmpSoldier(soldier))
                        == attackers.end());
            attackers.push_back(PotentialSeaAttacker(soldier, bld.harbor, bld.distance));
        }
    }

    return attackers;
}

void GameWorldBase::RecalcBQ(const MapPoint pt)
{
    BQCalculator calcBQ(*this);
    if(SetBQ(pt, calcBQ(pt, [this](auto pt) { return this->IsOnRoad(pt); })))
    {
        GetNotifications().publish(NodeNote(NodeNote::BQ, pt));
    }
}
