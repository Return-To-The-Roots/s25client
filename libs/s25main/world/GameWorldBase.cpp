// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "world/GameWorldBase.h"
#include "BQCalculator.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "MapGeometry.h"
#include "RttrForeachPt.h"
#include "SoundManager.h"
#include "TradePathCache.h"
#include "addons/const_addons.h"
#include "buildings/nobHarborBuilding.h"
#include "buildings/nobMilitary.h"
#include "figures/nofPassiveSoldier.h"
#include "helpers/EnumRange.h"
#include "helpers/IdRange.h"
#include "helpers/containerUtils.h"
#include "lua/LuaInterfaceGame.h"
#include "notifications/NodeNote.h"
#include "notifications/PlayerNodeNote.h"
#include "pathfinding/FreePathFinder.h"
#include "pathfinding/RoadPathFinder.h"
#include "pathfinding/ShipPathData.h"
#include "nodeObjs/noFlag.h"
#include "gameData/BuildingProperties.h"
#include "gameData/TerrainDesc.h"
#include <limits>
#include <utility>

GameWorldBase::GameWorldBase(std::vector<GamePlayer> players, const GlobalGameSettings& gameSettings, EventManager& em)
    : roadPathFinder(new RoadPathFinder(*this)), freePathFinder(new FreePathFinder(*this)), players(std::move(players)),
      gameSettings(gameSettings), em(em), soundManager(std::make_unique<SoundManager>()), lua(nullptr), gi(nullptr)
{}

GameWorldBase::~GameWorldBase() = default;

void GameWorldBase::init(const MapExtent& mapSize, DescIdx<LandscapeDesc> lt)
{
    RTTR_Assert(getDescription().terrain.size() > 0); // Must have game data initialized
    World::init(mapSize, lt);
    freePathFinder->init(mapSize);
}

void GameWorldBase::initAfterLoad()
{
    RTTR_FOREACH_PT(MapPoint, getSize())
        recalcBQ(pt);
}

GamePlayer& GameWorldBase::getPlayer(const unsigned id)
{
    RTTR_Assert(id < getNumPlayers());
    return players[id];
}

const GamePlayer& GameWorldBase::getPlayer(const unsigned id) const
{
    RTTR_Assert(id < getNumPlayers());
    return players[id];
}

unsigned GameWorldBase::getNumPlayers() const
{
    return players.size();
}

s25util::span<GamePlayer> GameWorldBase::getPlayers()
{
    return players;
}

s25util::span<const GamePlayer> GameWorldBase::getPlayers() const
{
    return players;
}

bool GameWorldBase::isSinglePlayer() const
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

ShipPathData& GameWorldBase::getShipPathData() const
{
    if(!shipPathData)
        shipPathData = std::make_unique<ShipPathData>(*this);
    return *shipPathData;
}

bool GameWorldBase::isRoadAvailable(const bool boat_road, const MapPoint pt) const
{
    // Hindernisse
    if(getNode(pt).obj)
    {
        BlockingManner bm = getNode(pt).obj->getBM();
        if(bm != BlockingManner::None)
            return false;
    }

    // dont build on the border
    if(getNode(pt).boundary_stones[BorderStonePos::OnPoint])
        return false;

    for(const auto dir : helpers::EnumRange<Direction>{})
    {
        // Roads around charburner piles are not possible
        if(getNO(getNeighbour(pt, dir))->getBM() == BlockingManner::NothingAround)
            return false;

        // Other roads at this point?
        if(getPointRoad(pt, dir) != PointRoad::None)
            return false;
    }

    // Terrain (unterscheiden, ob Wasser und Landweg)
    if(!boat_road)
    {
        bool flagPossible = false;

        for(const DescIdx<TerrainDesc> tIdx : getTerrainsAround(pt))
        {
            const TerrainBQ bq = getDescription().get(tIdx).getBQ();
            if(bq == TerrainBQ::Danger)
                return false;
            else if(bq != TerrainBQ::Nothing)
                flagPossible = true;
        }

        return flagPossible;
    } else
    {
        // Beim Wasserweg muss um den Punkt herum Wasser sein
        if(!isWaterPoint(pt))
            return false;
    }

    return true;
}

bool GameWorldBase::roadAlreadyBuilt(const bool /*boat_road*/, const MapPoint start,
                                     const std::vector<Direction>& route)
{
    MapPoint tmp(start);
    for(unsigned i = 0; i < route.size() - 1; ++i)
    {
        // Richtiger Weg auf diesem Punkt?
        if(getPointRoad(tmp, route[i]) == PointRoad::None)
            return false;

        tmp = getNeighbour(tmp, route[i]);
    }
    return true;
}

bool GameWorldBase::isOnRoad(const MapPoint& pt) const
{
    // This must be fast for BQ calculation so don't use GetVisiblePointRoad
    for(const auto roadDir : helpers::EnumRange<RoadDir>{})
        if(getRoad(pt, roadDir) != PointRoad::None)
            return true;
    for(const auto roadDir : helpers::EnumRange<RoadDir>{})
    {
        const Direction oppositeDir = getOppositeDir(roadDir);
        if(getRoad(getNeighbour(pt, oppositeDir), roadDir) != PointRoad::None)
            return true;
    }
    return false;
}

bool GameWorldBase::isFlagAround(const MapPoint& pt) const
{
    for(const MapPoint nb : getNeighbours(pt))
    {
        if(getNO(nb)->getBM() == BlockingManner::Flag)
            return true;
    }
    return false;
}

bool GameWorldBase::isAnyNeighborOwned(const MapPoint& pt) const
{
    for(const MapPoint& nb : getNeighbours(pt))
        if(getNode(nb).owner)
            return true;
    return false;
}

void GameWorldBase::recalcBQForRoad(const MapPoint pt)
{
    recalcBQ(pt);

    for(const Direction dir : {Direction::East, Direction::SouthEast, Direction::SouthWest})
        recalcBQ(getNeighbour(pt, dir));
}

namespace {
bool isMilBldOfOwner(const GameWorldBase& gwb, MapPoint pt, unsigned char owner)
{
    return gwb.isMilitaryBuildingOnNode(pt, false) && (gwb.getNode(pt).owner == owner);
}
} // namespace

bool GameWorldBase::isMilitaryBuildingNearNode(const MapPoint nPt, const unsigned char player) const
{
    // Im Umkreis von 4 Punkten ein Militärgebäude suchen
    return checkPointsInRadius(
      nPt, 4, [this, player](auto pt, auto) { return isMilBldOfOwner(*this, pt, player + 1); }, false);
}

bool GameWorldBase::isMilitaryBuildingOnNode(const MapPoint pt, bool attackBldsOnly) const
{
    const noBase* obj = getNO(pt);
    if(obj->getType() == NodalObjectType::Building || obj->getType() == NodalObjectType::Buildingsite)
    {
        BuildingType buildingType = static_cast<const noBaseBuilding*>(obj)->getBuildingType();
        if(BuildingProperties::isMilitary(buildingType))
            return true;
        if(!attackBldsOnly
           && (buildingType == BuildingType::Headquarters || buildingType == BuildingType::HarborBuilding))
            return true;
    }

    return false;
}

sortedMilitaryBlds GameWorldBase::lookForMilitaryBuildings(const MapPoint pt, unsigned short radius) const
{
    return militarySquares.getBuildingsInRange(pt, radius);
}

noFlag* GameWorldBase::getRoadFlag(MapPoint pt, Direction& dir, const helpers::OptionalEnum<Direction> prevDir)
{
    // Getting a flag is const
    const noFlag* flag = const_cast<const GameWorldBase*>(this)->getRoadFlag(pt, dir, prevDir);
    // However we self are not const, so we allow returning a non-const flag pointer
    return const_cast<noFlag*>(flag);
}

const noFlag* GameWorldBase::getRoadFlag(MapPoint pt, Direction& dir, helpers::OptionalEnum<Direction> prevDir) const
{
    while(true)
    {
        // suchen, wo der Weg weitergeht
        helpers::OptionalEnum<Direction> nextDir;
        for(const auto i : helpers::EnumRange<Direction>{})
        {
            if(i != prevDir && getPointRoad(pt, i) != PointRoad::None)
            {
                nextDir = i;
                break;
            }
        }

        if(!nextDir)
            return nullptr;

        pt = getNeighbour(pt, *nextDir);

        // endlich am Ende des Weges und an einer Flagge angekommen?
        if(getNO(pt)->getType() == NodalObjectType::Flag)
        {
            dir = *nextDir + 3u;
            return getSpecObj<noFlag>(pt);
        }
        prevDir = *nextDir + 3u;
    }
}

Position GameWorldBase::getNodePos(const MapPoint pt) const
{
    return ::getNodePos(pt, getNode(pt).altitude);
}

void GameWorldBase::visibilityChanged(const MapPoint pt, unsigned player, Visibility /*oldVis*/, Visibility /*newVis*/)
{
    getNotifications().publish(PlayerNodeNote(PlayerNodeNote::Visibility, pt, player));
}

/// Verändert die Höhe eines Punktes und die damit verbundenen Schatten
void GameWorldBase::altitudeChanged(const MapPoint pt)
{
    recalcBQAroundPointBig(pt);
    getNotifications().publish(NodeNote(NodeNote::Altitude, pt));
}

void GameWorldBase::recalcBQAroundPoint(const MapPoint pt)
{
    recalcBQ(pt);
    for(const MapPoint nb : getNeighbours(pt))
        recalcBQ(nb);
}

void GameWorldBase::recalcBQAroundPointBig(const MapPoint pt)
{
    // Point and radius 1
    recalcBQAroundPoint(pt);
    // And radius 2
    for(unsigned i = 0; i < 12; ++i)
        recalcBQ(getNeighbour2(pt, i));
}

Visibility GameWorldBase::calcVisiblityWithAllies(const MapPoint pt, const unsigned char player) const
{
    const MapNode& node = getNode(pt);
    Visibility best_visibility = node.fow[player].visibility;

    if(best_visibility == Visibility::Visible)
        return best_visibility;

    /// Teamsicht aktiviert?
    if(getGGS().teamView)
    {
        const GamePlayer& curPlayer = getPlayer(player);
        // Dann prüfen, ob Teammitglieder evtl. eine bessere Sicht auf diesen Punkt haben
        for(unsigned i = 0; i < getNumPlayers(); ++i)
        {
            if(i != player && curPlayer.isAlly(i))
            {
                if(node.fow[i].visibility > best_visibility)
                    best_visibility = node.fow[i].visibility;
            }
        }
    }

    return best_visibility;
}

bool GameWorldBase::isCoastalPointToSeaWithHarbor(const MapPoint pt) const
{
    SeaId sea = getSeaFromCoastalPoint(pt);
    if(sea)
    {
        for(const auto i : helpers::idRange<HarborId>(getNumHarborPoints()))
        {
            if(isHarborAtSea(HarborId(i), sea))
                return true;
        }
    }
    return false;
}

template<typename T_IsHarborOk>
HarborId GameWorldBase::getHarborInDir(const MapPoint pt, const HarborId originHarborId, const ShipDirection& dir,
                                       T_IsHarborOk isHarborOk) const
{
    RTTR_Assert(originHarborId);

    const MapPoint hbPt = getHarborPoint(originHarborId);

    // Find sea of harbor point
    SeaId seaId;
    for(const auto d : helpers::EnumRange<Direction>{})
    {
        if(getNeighbour(hbPt, d) == pt)
        {
            seaId = getSeaId(originHarborId, d);
            break;
        }
    }
    RTTR_Assert(seaId);

    const std::vector<HarborPos::Neighbor>& neighbors = getHarborNeighbors(originHarborId, dir);

    for(const auto& neighbor : neighbors)
    {
        if(isHarborAtSea(neighbor.id, seaId) && isHarborOk(neighbor.id))
            return neighbor.id;
    }

    // Nichts gefunden
    return HarborId::invalidValue();
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
        const unsigned char owner = gwb.getNode(pt).owner;
        return owner != 0 && owner != cmpOwner;
    }
};

/// Ist es an dieser Stelle für einen Spieler möglich einen Hafen zu bauen
bool GameWorldBase::isHarborPointFree(const HarborId harborId, const unsigned char player) const
{
    MapPoint hbPos(getHarborPoint(harborId));

    // Überprüfen, ob das Gebiet in einem bestimmten Radius entweder vom Spieler oder gar nicht besetzt ist außer wenn
    // der Hafen und die Flagge im Spielergebiet liegen
    MapPoint flagPos = getNeighbour(hbPos, Direction::SouthEast);
    if(getNode(hbPos).owner != player + 1 || getNode(flagPos).owner != player + 1)
    {
        if(checkPointsInRadius(hbPos, 4, IsPointOwnerDifferent(*this, player), false))
            return false;
    }

    return getNode(hbPos).bq == BuildingQuality::Harbor;
}

/// Sucht freie Hafenpunkte, also wo noch ein Hafen gebaut werden kann
HarborId GameWorldBase::getNextFreeHarborPoint(const MapPoint pt, const HarborId originHarborId,
                                               const ShipDirection& dir, const unsigned char player) const
{
    return getHarborInDir(pt, originHarborId, dir,
                          [this, player](auto harborId) { return this->isHarborPointFree(harborId, player); });
}

/// Bestimmt für einen beliebigen Punkt auf der Karte die Entfernung zum nächsten Hafenpunkt
unsigned GameWorldBase::calcDistanceToNearestHarbor(const MapPoint pos) const
{
    unsigned min_distance = std::numeric_limits<unsigned>::max();
    for(const auto i : helpers::idRange<HarborId>(getNumHarborPoints()))
        min_distance = std::min(min_distance, this->calcDistance(pos, getHarborPoint(i)));

    return min_distance;
}

/// returns true when a harborpoint is in SEAATTACK_DISTANCE for figures!
bool GameWorldBase::isAHarborInSeaAttackDistance(const MapPoint pos) const
{
    for(const auto i : helpers::idRange<HarborId>(getNumHarborPoints()))
    {
        if(calcDistance(pos, getHarborPoint(i)) < SEAATTACK_DISTANCE)
        {
            if(findHumanPath(pos, getHarborPoint(i), SEAATTACK_DISTANCE))
                return true;
        }
    }
    return false;
}

std::vector<HarborId> GameWorldBase::getUsableTargetHarborsForAttack(const MapPoint targetPt,
                                                                     std::vector<bool>& use_seas,
                                                                     const unsigned char player_attacker) const
{
    // Walk to the flag of the bld/harbor. Important to check because in some locations where the coast is north of the
    // harbor this might be blocked
    const MapPoint flagPt = getNeighbour(targetPt, Direction::SouthEast);
    std::vector<HarborId> harbor_points;
    // Check each possible harbor
    for(const auto curHbId : helpers::idRange<HarborId>(getNumHarborPoints()))
    {
        const MapPoint harborPt = getHarborPoint(curHbId);

        if(calcDistance(harborPt, targetPt) > SEAATTACK_DISTANCE)
            continue;

        // Not attacking this harbor and harbors block?
        if(targetPt != harborPt && getGGS().getSelection(AddonId::SEA_ATTACK) == 1)
        {
            // Does an enemy harbor exist at current harbor spot? -> Can't attack through this harbor spot
            const auto* hb = getSpecObj<nobHarborBuilding>(harborPt);
            if(hb && getPlayer(player_attacker).isAttackable(hb->getPlayer()))
                continue;
        }

        // add seaIds from which we can actually attack the harbor
        bool harborinlist = false;
        for(const auto dir : helpers::enumRange<Direction>())
        {
            const SeaId seaId = getSeaId(curHbId, dir);
            if(!seaId)
                continue;
            // checks previously tested sea ids to skip pathfinding
            bool previouslytested = false;
            for(unsigned k = 0; k < rttr::enumCast(dir); k++)
            {
                if(seaId == getSeaId(curHbId, Direction(k)))
                {
                    previouslytested = true;
                    break;
                }
            }
            if(previouslytested)
                continue;

            // Can figures reach flag from coast
            const MapPoint coastalPt = getCoastalPoint(curHbId, seaId);
            if((flagPt == coastalPt) || findHumanPath(flagPt, coastalPt, SEAATTACK_DISTANCE))
            {
                use_seas.at(seaId.value() - 1) = true;
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

std::vector<SeaId> GameWorldBase::getFilteredSeaIDsForAttack(const MapPoint targetPt,
                                                             const std::vector<SeaId>& usableSeas,
                                                             const unsigned char player_attacker) const
{
    // Walk to the flag of the bld/harbor. Important to check because in some locations where the coast is north of the
    // harbor this might be blocked
    const MapPoint flagPt = getNeighbour(targetPt, Direction::SouthEast);
    std::vector<SeaId> confirmedSeaIds;
    // Check each possible harbor
    for(const auto curHbId : helpers::idRange<HarborId>(getNumHarborPoints()))
    {
        const MapPoint harborPt = getHarborPoint(curHbId);

        if(calcDistance(harborPt, targetPt) > SEAATTACK_DISTANCE)
            continue;

        // Not attacking this harbor and harbors block?
        if(targetPt != harborPt && getGGS().getSelection(AddonId::SEA_ATTACK) == 1)
        {
            // Does an enemy harbor exist at current harbor spot? -> Can't attack through this harbor spot
            const auto* hb = getSpecObj<nobHarborBuilding>(harborPt);
            if(hb && getPlayer(player_attacker).isAttackable(hb->getPlayer()))
                continue;
        }

        for(const auto dir : helpers::enumRange<Direction>())
        {
            const SeaId seaId = getSeaId(curHbId, dir);
            if(!seaId)
                continue;
            // sea id is not in compare list or already confirmed? -> skip rest
            if(!helpers::contains(usableSeas, seaId) || helpers::contains(confirmedSeaIds, seaId))
                continue;

            // checks previously tested sea ids to skip pathfinding
            bool previouslytested = false;
            for(unsigned k = 0; k < rttr::enumCast(dir); k++)
            {
                if(seaId == getSeaId(curHbId, Direction(k)))
                {
                    previouslytested = true;
                    break;
                }
            }
            if(previouslytested)
                continue;

            // Can figures reach flag from coast
            MapPoint coastalPt = getCoastalPoint(curHbId, seaId);
            if((flagPt == coastalPt) || findHumanPath(flagPt, coastalPt, SEAATTACK_DISTANCE))
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
std::vector<HarborId> GameWorldBase::getHarborPointsAroundMilitaryBuilding(const MapPoint pt) const
{
    std::vector<HarborId> harbor_points;
    // Nach Hafenpunkten in der Nähe des angegriffenen Gebäudes suchen
    // Alle unsere Häfen durchgehen
    for(const auto i : helpers::idRange<HarborId>(getNumHarborPoints()))
    {
        const MapPoint harborPt = getHarborPoint(i);

        if(calcDistance(harborPt, pt) <= SEAATTACK_DISTANCE)
        {
            // Wird ein Weg vom Militärgebäude zum Hafen gefunden bzw. Ziel = Hafen?
            if(pt == harborPt || findHumanPath(pt, harborPt, SEAATTACK_DISTANCE))
                harbor_points.push_back(i);
        }
    }
    return harbor_points;
}

/// Gibt Anzahl oder geschätzte Stärke(rang summe + anzahl) der verfügbaren Soldaten die zu einem Schiffsangriff starten
/// können von einer bestimmten sea id aus
unsigned GameWorldBase::getNumSoldiersForSeaAttackAtSea(const unsigned char player_attacker, SeaId sea,
                                                        bool returnCount) const
{
    // Liste alle Militärgebäude des Angreifers, die Soldaten liefern
    std::vector<nobHarborBuilding::SeaAttackerBuilding> buildings;
    unsigned attackercount = 0;
    // Angrenzende Häfen des Angreifers an den entsprechenden Meeren herausfinden
    const std::list<nobHarborBuilding*>& harbors = getPlayer(player_attacker).getBuildingRegister().getHarbors();
    for(auto* harbor : harbors)
    {
        // Bestimmen, ob Hafen an einem der Meere liegt, über die sich auch die gegnerischen
        // Hafenpunkte erreichen lassen
        if(!isHarborAtSea(harbor->getHarborPosID(), sea))
            continue;

        std::vector<nobHarborBuilding::SeaAttackerBuilding> tmp = harbor->getAttackerBuildingsForSeaIdAttack();
        buildings.insert(buildings.begin(), tmp.begin(), tmp.end());
    }

    // Die Soldaten aus allen Militärgebäuden sammeln
    for(auto& building : buildings)
    {
        // Soldaten holen
        std::vector<nofPassiveSoldier*> tmp_soldiers =
          building.building->getSoldiersForAttack(building.harbor->getPos());

        // Überhaupt welche gefunden?
        if(tmp_soldiers.empty())
            continue;

        // Soldaten hinzufügen
        for(auto& tmp_soldier : tmp_soldiers)
        {
            if(returnCount)
                attackercount++;
            else
                attackercount += (tmp_soldier->getRank() + 1); // private is rank 0 -> increase by 1-5
        }
    }
    return attackercount;
}

/// Sucht verfügbare Soldaten, um dieses Militärgebäude mit einem Seeangriff anzugreifen
std::vector<GameWorldBase::PotentialSeaAttacker>
GameWorldBase::getSoldiersForSeaAttack(const unsigned char player_attacker, const MapPoint pt) const
{
    std::vector<GameWorldBase::PotentialSeaAttacker> attackers;
    // sea attack abgeschaltet per addon?
    if(!getGGS().isEnabled(AddonId::SEA_ATTACK))
        return attackers;
    // Do we have an attackble military building?
    const auto* milBld = getSpecObj<nobBaseMilitary>(pt);
    if(!milBld || !milBld->isAttackable(player_attacker))
        return attackers;
    std::vector<bool> use_seas(getNumSeas());

    // Mögliche Hafenpunkte in der Nähe des Gebäudes
    std::vector<HarborId> defender_harbors = getUsableTargetHarborsForAttack(pt, use_seas, player_attacker);

    // Liste alle Militärgebäude des Angreifers, die Soldaten liefern
    std::vector<nobHarborBuilding::SeaAttackerBuilding> buildings;

    // Angrenzende Häfen des Angreifers an den entsprechenden Meeren herausfinden
    const std::list<nobHarborBuilding*>& harbors = getPlayer(player_attacker).getBuildingRegister().getHarbors();
    for(auto* harbor : harbors)
    {
        // Bestimmen, ob Hafen an einem der Meere liegt, über die sich auch die gegnerischen
        // Hafenpunkte erreichen lassen
        bool is_at_sea = false;
        for(const auto dir : helpers::EnumRange<Direction>{})
        {
            const SeaId seaId = getSeaId(harbor->getHarborPosID(), dir);
            if(seaId && use_seas[seaId.value() - 1])
            {
                is_at_sea = true;
                break;
            }
        }

        if(!is_at_sea)
            continue;

        std::vector<nobHarborBuilding::SeaAttackerBuilding> tmp =
          harbor->getAttackerBuildingsForSeaAttack(defender_harbors);
        for(auto& itBld : tmp)
        {
            // Check if the building was already inserted
            auto oldBldIt =
              helpers::findIf(buildings, nobHarborBuilding::SeaAttackerBuilding::CmpBuilding(itBld.building));
            if(oldBldIt == buildings.end())
            {
                // Not found -> Add
                buildings.push_back(itBld);
            } else if(oldBldIt->distance > itBld.distance
                      || (oldBldIt->distance == itBld.distance
                          && oldBldIt->harbor->getObjId() > itBld.harbor->getObjId()))
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
        std::vector<nofPassiveSoldier*> tmp_soldiers = bld.building->getSoldiersForAttack(bld.harbor->getPos());

        // Soldaten hinzufügen
        for(nofPassiveSoldier* soldier : tmp_soldiers)
        {
            RTTR_Assert(!helpers::containsIf(attackers, PotentialSeaAttacker::CmpSoldier(soldier)));
            attackers.push_back(PotentialSeaAttacker(soldier, bld.harbor, bld.distance));
        }
    }

    return attackers;
}

void GameWorldBase::recalcBQ(const MapPoint pt)
{
    BQCalculator calcBQ(*this);
    if(setBQ(pt, calcBQ(pt, [this](auto pt) { return this->isOnRoad(pt); })))
    {
        getNotifications().publish(NodeNote(NodeNote::BQ, pt));
    }
}

void GameWorldBase::setComputerBarrier(const MapPoint& pt, unsigned radius)
{
    for(const auto& pt : getPointsInRadiusWithCenter(pt, radius))
        ptsInsideComputerBarriers.insert(pt);
}

bool GameWorldBase::isInsideComputerBarrier(const MapPoint& pt) const
{
    return helpers::contains(ptsInsideComputerBarriers, pt);
}
