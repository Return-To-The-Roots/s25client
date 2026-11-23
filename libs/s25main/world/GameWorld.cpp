// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "world/GameWorld.h"
#include "EventManager.h"
#include "GameInterface.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "RttrForeachPt.h"
#include "TradePathCache.h"
#include "addons/const_addons.h"
#include "buildings/noBuildingSite.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobUsual.h"
#include "figures/nofAttacker.h"
#include "figures/nofPassiveSoldier.h"
#include "figures/nofScout_Free.h"
#include "helpers/IdRange.h"
#include "helpers/Range.h"
#include "helpers/containerUtils.h"
#include "helpers/mathFuncs.h"
#include "helpers/reverse.h"
#include "lua/LuaInterfaceGame.h"
#include "notifications/BuildingNote.h"
#include "notifications/ExpeditionNote.h"
#include "notifications/NodeNote.h"
#include "notifications/RoadNote.h"
#include "pathfinding/PathConditionHuman.h"
#include "pathfinding/PathConditionRoad.h"
#include "postSystem/PostMsgWithBuilding.h"
#include "world/MapGeometry.h"
#include "world/TerritoryRegion.h"
#include "nodeObjs/noFighting.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noShip.h"
#include "nodeObjs/noStaticObject.h"
#include "gameData/BuildingConsts.h"
#include "gameData/BuildingProperties.h"
#include "gameData/MilitaryConsts.h"
#include "gameData/TerrainDesc.h"
#include <algorithm>
#include <functional>
#include <set>
#include <stdexcept>

inline std::vector<GamePlayer> createPlayers(const std::vector<PlayerInfo>& playerInfos, GameWorld& world)
{
    std::vector<GamePlayer> players;
    players.reserve(playerInfos.size());
    for(unsigned i = 0; i < playerInfos.size(); ++i)
        players.push_back(GamePlayer(i, playerInfos[i], world));
    return players;
}

GameWorld::GameWorld(const std::vector<PlayerInfo>& players, const GlobalGameSettings& gameSettings, EventManager& em)
    : GameWorldBase(createPlayers(players, *this), gameSettings, em)
{
    GameObject::attachWorld(this);
}

GameWorld::~GameWorld()
{
    GameObject::detachWorld(this);
}

MilitarySquares& GameWorld::getMilitarySquares()
{
    return militarySquares;
}

bool GameWorld::SetFlag(const MapPoint pt, const unsigned char player)
{
    if(getBQ(pt, player) == BuildingQuality::Nothing)
        return false;
    // There must be no other flag around that point
    if(isFlagAround(pt))
        return false;

    // Gucken, nicht, dass schon eine Flagge dasteht
    if(getNO(pt)->getType() != NodalObjectType::Flag)
    {
        destroyNO(pt, false);
        setNO(pt, new noFlag(pt, player));

        recalcBQAroundPointBig(pt);
    }
    return true;
}

void GameWorld::DestroyFlag(const MapPoint pt, unsigned char playerId)
{
    // Let's see if there is a flag
    if(getNO(pt)->getType() == NodalObjectType::Flag)
    {
        auto* flag = getSpecObj<noFlag>(pt);
        if(flag->getPlayer() != playerId)
            return;

        // Get the attached building if existing
        noBase* building = getNO(getNeighbour(pt, Direction::NorthWest));

        // Is this a military building?
        if(building->getGOT() == GO_Type::NobMilitary)
        {
            // Maybe demolition of the building is not allowed?
            if(!static_cast<nobMilitary*>(building)->isDemolitionAllowed())
                return; // Abort the whole thing
        }

        // Demolish, also the building
        flag->destroyAttachedBuilding();

        destroyNO(pt, false);
        recalcBQAroundPointBig(pt);
    }

    if(gi)
        gi->GI_FlagDestroyed(pt);
}

void GameWorld::setPointRoad(MapPoint pt, const Direction dir, const PointRoad type)
{
    const RoadDir rDir = toRoadDir(pt, dir);
    setRoad(pt, rDir, type);

    if(gi)
        gi->GI_UpdateMinimap(pt);
}

void GameWorld::SetBuildingSite(const BuildingType type, const MapPoint pt, const unsigned char player)
{
    if(!getPlayer(player).isBuildingEnabled(type))
        return;

    // Gucken, ob das Gebäude hier überhaupt noch gebaut wrden kann
    if(!canUseBq(getBQ(pt, player), BUILDING_SIZE[type]))
        return;

    // Wenn das ein Militärgebäude ist und andere Militärgebäude bereits in der Nähe sind, darf dieses nicht gebaut
    // werden
    if(BuildingProperties::isMilitary(type))
    {
        if(isMilitaryBuildingNearNode(pt, player))
            return;
    }

    // Prüfen ob Katapult und ob Katapult erlaubt ist
    if(type == BuildingType::Catapult && !getPlayer(player).canBuildCatapult())
        return;

    if(type == BuildingType::Church && !canBuildChurchAt(pt, player))
        return;

    destroyNO(pt, false);

    // Baustelle setzen
    setNO(pt, new noBuildingSite(type, pt, player));
    if(gi)
        gi->GI_UpdateMinimap(pt);

    // Bauplätze drumrum neu berechnen
    recalcBQAroundPointBig(pt);
}

void GameWorld::DestroyBuilding(const MapPoint pt, const unsigned char player)
{
    // Steht da auch ein Gebäude oder eine Baustelle, nicht dass wir aus Verzögerung Feuer abreißen wollen, das geht
    // schief
    if(getNO(pt)->getType() == NodalObjectType::Building || getNO(pt)->getType() == NodalObjectType::Buildingsite)
    {
        auto* nbb = getSpecObj<noBaseBuilding>(pt);

        // Ist das Gebäude auch von dem Spieler, der es abreißen will?
        if(nbb->getPlayer() != player)
            return;

        // Militärgebäude?
        if(nbb->getGOT() == GO_Type::NobMilitary)
        {
            // Darf das Gebäude abgerissen werden?
            if(!static_cast<nobMilitary*>(nbb)->isDemolitionAllowed())
                return;
        }

        destroyNO(pt);
        // Bauplätze drumrum neu berechnen
        recalcBQAroundPointBig(pt);
    }
}

MapPoint GameWorld::BuildRoad(const unsigned char playerId, const bool boat_road, const MapPoint start,
                              const std::vector<Direction>& route)
{
    // No routes with less than 2 parts. Actually invalid!
    if(route.size() < 2)
    {
        RTTR_Assert(false);
        return {};
    }

    if(!getSpecObj<noFlag>(start) || getSpecObj<noFlag>(start)->getPlayer() != playerId)
    {
        getNotifications().publish(RoadNote(RoadNote::ConstructionFailed, playerId, start, route));
        return {};
    }

    // See if the road can still be built at all
    PathConditionRoad<GameWorldBase> roadChecker(*this, boat_road);
    MapPoint curPt(start);
    for(const auto i : helpers::range(route.size() - 1u))
    {
        bool roadOk = roadChecker.isEdgeOk(curPt, route[i]);
        curPt = getNeighbour(curPt, route[i]);
        roadOk &= roadChecker.isNodeOk(curPt);
        if(!roadOk)
        {
            // No? Then check whether the desired road is already there
            if(!roadAlreadyBuilt(boat_road, start, route))
                getNotifications().publish(RoadNote(RoadNote::ConstructionFailed, playerId, start, route));
            return {};
        }
    }

    const auto endPt = getNeighbour(curPt, route.back());

    // Check whether there is a flag at the end or whether one can be built
    if(getNO(endPt)->getGOT() == GO_Type::Flag)
    {
        // Wrong player?
        if(getSpecObj<noFlag>(endPt)->getPlayer() != playerId)
        {
            getNotifications().publish(RoadNote(RoadNote::ConstructionFailed, playerId, start, route));
            return {};
        }
    } else if(!SetFlag(endPt, playerId)) // Try placing a flag
    {
        getNotifications().publish(RoadNote(RoadNote::ConstructionFailed, playerId, start, route));
        return {};
    }

    // Destroy possible decorative objects at start
    if(hasRemovableObjForRoad(start))
        destroyNO(start);

    curPt = start;
    for(auto i : route)
    {
        setPointRoad(curPt, i, boat_road ? PointRoad::Boat : PointRoad::Normal);
        recalcBQForRoad(curPt);
        curPt = getNeighbour(curPt, i);

        // Destroy possible decorative objects at end
        if(hasRemovableObjForRoad(curPt))
            destroyNO(curPt);
    }

    auto* rs = new RoadSegment(boat_road ? RoadType::Water : RoadType::Normal, getSpecObj<noFlag>(start),
                               getSpecObj<noFlag>(endPt), route);

    getSpecObj<noFlag>(start)->setRoute(route.front(), rs);
    getSpecObj<noFlag>(endPt)->setRoute(route.back() + 3u, rs);

    // Tell the economy that a new road has been built
    getPlayer(playerId).newRoadConnection(rs);
    getNotifications().publish(RoadNote(RoadNote::Constructed, playerId, start, route));

    // Add flags on land roads for human players if addon is enabled
    if(getGGS().isEnabled(AddonId::AUTOFLAGS) && rs->getRoadType() != RoadType::Water && getPlayer(playerId).isHuman())
    {
        MapPoint roadPt = getSpecObj<noFlag>(start)->getPos();
        for(const Direction curDir : route)
        {
            roadPt = getNeighbour(roadPt, curDir);
            if(!isFlagAround(roadPt))
                SetFlag(roadPt, playerId);
        }
    }
    return endPt;
}

bool GameWorld::hasRemovableObjForRoad(const MapPoint pt) const
{
    const auto* obj = getSpecObj<noStaticObject>(pt);
    return obj && obj->getSize() == 0;
}

// When defined the game tries to remove "blocks" of border stones that look ugly (TODO: Example?)
// DISABLED: This currently leads to bugs. If you enable/fix this, please add tests and document the conditions this
// tries to fix
// #define PREVENT_BORDER_STONE_BLOCKING

void GameWorld::recalcBorderStones(Position startPt, Extent areaSize)
{
    // Add a bit extra space as this influences also border stones around the region
    // But not so much we wrap completely around the map (+1 to round up, /2 to have extra space centered)
    Position EXTRA_SPACE = elMin(Position::all(3), (getSize() - areaSize + Position::all(1)) / 2);
    startPt -= EXTRA_SPACE;
    areaSize += 2u * Extent(EXTRA_SPACE);
    // We might still be 1 node to big, make sure we have don't exceed the mapsize
    areaSize = elMin(areaSize, Extent(getSize()));

#ifdef PREVENT_BORDER_STONE_BLOCKING
    // Store how many neighbors a border stone has
    std::vector<uint8_t> neighbors(areaSize.x * areaSize.y, 0);
#endif

    RTTR_FOREACH_PT(Position, areaSize)
    {
        // Make map point
        const MapPoint curMapPt = makeMapPoint(pt + startPt);
        const unsigned char owner = getNode(curMapPt).owner;
        BoundaryStones& boundaryStones = getBoundaryStones(curMapPt);

        // Is this a border node?
        if(owner && isBorderNode(curMapPt, owner))
        {
            // Check which neighbors are also border nodes and place the half-way stones to them
            for(const auto bPos : helpers::EnumRange<BorderStonePos>{})
            {
                if(bPos == BorderStonePos::OnPoint || isBorderNode(getNeighbour(curMapPt, toDirection(bPos)), owner))
                    boundaryStones[bPos] = owner;
                else
                    boundaryStones[bPos] = 0;
            }

#ifdef PREVENT_BORDER_STONE_BLOCKING
            // Count number of border nodes with same owner
            int idx = pt.y * width + pt.x;
            for(const MapPoint nb : getNeighbours(curMapPt))
            {
                if(getNode(nb).boundary_stones[0] == owner)
                    ++neighbors[idx];
            }
#endif
        } else
        {
            // Not a border node -> Delete all border stones
            std::fill(boundaryStones.begin(), boundaryStones.end(), 0);
        }
    }

#ifdef PREVENT_BORDER_STONE_BLOCKING
    // Do a second pass and delete some stones with 3 or more neighbors to avoid blocks of stones
    RTTR_FOREACH_PT(Position, areaSize)
    {
        const MapPoint curMapPt = makeMapPoint(pt + startPt);

        // Do we have a stone here?
        const unsigned char owner = getNode(curMapPt).boundary_stones[BorderStonePos::OnPoint];
        if(!owner)
            continue;

        int idx = pt.y * width + pt.x;
        if(neighbors[idx] < 3)
            continue;

        for(unsigned dir = 0; dir < 3 && neighbors[idx] > 2; ++dir)
        {
            // Do we have a border stone of the same owner on the node in that direction?
            BoundaryStones& nbBoundStones = getBoundaryStones(getNeighbour(curMapPt, dir + 3));

            if(nbBoundStones[0] != owner)
                continue;

            Position pa = ::getNeighbour(pt, Direction(dir + 3));
            if(pa.x < 0 || pa.x >= areaSize.x || pa.y < 0 || pa.y >= areaSize.y)
                continue;
            // If that one has to many stones too, we delete the connection stone
            int idxNb = pa.y * width + pa.x;
            if(neighbors[idxNb] > 2)
            {
                nbBoundStones[dir + 1] = 0;
                --neighbors[idx];
                --neighbors[idxNb];
            }
        }
    }
#endif
}

void GameWorld::recalcTerritory(const noBaseBuilding& building, TerritoryChangeReason reason)
{
    // Additional radius to eliminate border stones or odd remaining territory parts
    static const int ADD_RADIUS = 2;
    // Get the military radius this building affects. Bld is either a military building or a harbor building site
    RTTR_Assert(
      (building.getBuildingType() == BuildingType::HarborBuilding && dynamic_cast<const noBuildingSite*>(&building))
      || dynamic_cast<const nobBaseMilitary*>(&building));
    const unsigned militaryRadius = building.getMilitaryRadius();
    RTTR_Assert(militaryRadius > 0u);

    const TerritoryRegion region = createTerritoryRegion(building, militaryRadius + ADD_RADIUS, reason);

    std::vector<MapPoint> ptsWithChangedOwners;
    std::vector<int> sizeChanges(getNumPlayers());

    // Copy owners from territory region to map and do the bookkeeping
    RTTR_FOREACH_PT(Position, region.size)
    {
        const MapPoint curMapPt = makeMapPoint(pt + region.startPt);
        const uint8_t oldOwner = getNode(curMapPt).owner;
        const uint8_t newOwner = region.getOwner(pt);

        // If nothing changed, there is nothing to do (ownerChanged was already initialized)
        if(oldOwner == newOwner)
            continue;

        setOwner(curMapPt, newOwner);
        ptsWithChangedOwners.push_back(curMapPt);
        if(newOwner != 0)
            sizeChanges[newOwner - 1]++;
        if(oldOwner != 0)
            sizeChanges[oldOwner - 1]--;
    }

    const std::vector<MapPoint> ptsToHandle = getAllNeighboursUnion(ptsWithChangedOwners);

    // Destroy everything from old player on all nodes where the owner has changed
    for(const MapPoint& curMapPt : ptsToHandle)
    {
        // Do not destroy the triggering building or its flag
        destroyPlayerRests(curMapPt, getNode(curMapPt).owner, &building);

        if(gi)
            gi->GI_UpdateMinimap(curMapPt);
    }

    // Destroy remaining roads going through non-owned and border territory
    for(const MapPoint& curMapPt : ptsToHandle)
    {
        // Skip if there is an object. We are looking only for roads going through, not ending here
        // (objects here are already destroyed and if the road ended there it would have been as well)
        if(getNode(curMapPt).obj)
            continue;
        Direction dir;
        noFlag* flag = getRoadFlag(curMapPt, dir);

        if(!flag)
            continue;

        const uint8_t owner = getNode(curMapPt).owner;
        if(flag->getPlayer() + 1 == owner && isPlayerTerritory(curMapPt, owner))
            continue;

        flag->DestroyRoad(dir);
    }

    // Notify
    for(const MapPoint& curMapPt : ptsWithChangedOwners)
        getNotifications().publish(NodeNote(NodeNote::Owner, curMapPt));

    for(const MapPoint& pt : ptsToHandle)
    {
        // BQ neu berechnen
        recalcBQ(pt);
        // ggf den noch darüber, falls es eine Flagge war (kann ja ein Gebäude entstehen)
        const MapPoint neighbourPt = getNeighbour(pt, Direction::NorthWest);
        if(getNode(neighbourPt).bq != BuildingQuality::Nothing)
            recalcBQ(neighbourPt);
    }

    recalcBorderStones(region.startPt, region.size);

    // Recalc visibilities if building was destroyed
    // Otherwise just set everything to visible
    const unsigned visualRadius = militaryRadius + VISUALRANGE_MILITARY;
    if(reason == TerritoryChangeReason::Destroyed)
        recalcVisibilitiesAroundPoint(building.getPos(), visualRadius, building.getPlayer(), &building);
    else
        makeVisibleAroundPoint(building.getPos(), visualRadius, building.getPlayer());

    // Notify players
    for(unsigned i = 0; i < getNumPlayers(); ++i)
    {
        getPlayer(i).changeStatisticValue(StatisticType::Country, sizeChanges[i]);

        // Negatives Wachstum per Post dem/der jeweiligen Landesherren/dame melden, nur bei neugebauten Gebäuden
        if(reason == TerritoryChangeReason::Build && sizeChanges[i] < 0)
        {
            getPostMgr().sendMsg(i, std::make_unique<PostMsgWithBuilding>(getEvMgr().getCurrentGF(),
                                                                          _("Lost land by this building"),
                                                                          PostCategory::Military, building));
            getNotifications().publish(
              BuildingNote(BuildingNote::LostLand, i, building.getPos(), building.getBuildingType()));
        }
    }

    // Notify script
    if(hasLua())
    {
        for(const MapPoint& pt : ptsWithChangedOwners)
        {
            const uint8_t newOwner = getNode(pt).owner;
            // Event for map scripting
            if(newOwner != 0)
                getLua().eventOccupied(newOwner - 1, pt);
        }
    }
}

bool GameWorld::doesDestructionChangeTerritory(const noBaseBuilding& building) const
{
    // Get the military radius this building affects. Bld is either a military building or a harbor building site
    RTTR_Assert(
      (building.getBuildingType() == BuildingType::HarborBuilding && dynamic_cast<const noBuildingSite*>(&building))
      || dynamic_cast<const nobBaseMilitary*>(&building));
    const unsigned militaryRadius = building.getMilitaryRadius();
    RTTR_Assert(militaryRadius > 0u);

    const TerritoryRegion region = createTerritoryRegion(building, militaryRadius, TerritoryChangeReason::Destroyed);

    // Look for a node that changed its owner and is important. If any -> return true
    RTTR_FOREACH_PT(Position, region.size)
    {
        MapPoint curMapPt = makeMapPoint(pt + region.startPt);
        const MapNode& node = getNode(curMapPt);
        if(node.owner == region.getOwner(pt))
            continue;
        // AI can ignore water/snow/lava/swamp terrain (because it wouldn't help win the game)
        // So we check if any terrain is usable and if it is -> Land is important
        if(getDescription().get(node.t1).is(ETerrain::Walkable) && getDescription().get(node.t2).is(ETerrain::Walkable))
            return true;
        // also check neighboring nodes since border will still count as player territory but not allow any buildings!
        for(const MapPoint neighbourPt : getNeighbours(curMapPt))
        {
            const MapNode& nNode = getNode(neighbourPt);
            if(getDescription().get(nNode.t1).is(ETerrain::Walkable)
               || getDescription().get(nNode.t2).is(ETerrain::Walkable))
                return true;
        }
    }
    return false;
}

TerritoryRegion GameWorld::createTerritoryRegion(const noBaseBuilding& building, unsigned radius,
                                                 TerritoryChangeReason reason) const
{
    const MapPoint bldPos = building.getPos();

    // Span at most half the map size (assert even sizes, given due to layout)
    RTTR_Assert(getWidth() % 2 == 0);
    RTTR_Assert(getHeight() % 2 == 0);
    Extent halfSize(getSize() / 2u);
    Extent radius2D = elMin(Extent::all(radius), halfSize);

    // Koordinaten erzeugen für TerritoryRegion
    const Position startPt = Position(bldPos) - radius2D;
    // If we want to check the same number of points right of bld as left we need a +1.
    // But we can't check more than the whole map.
    const Extent size = elMin(2u * radius2D + Extent(1, 1), Extent(getSize()));
    TerritoryRegion region(startPt, size, *this);

    // Alle Gebäude ihr Terrain in der Nähe neu berechnen
    sortedMilitaryBlds buildings = lookForMilitaryBuildings(bldPos, 3);
    for(const nobBaseMilitary* milBld : buildings)
    {
        if(reason != TerritoryChangeReason::Destroyed || milBld != &building)
            region.calcTerritoryOfBuilding(*milBld);
    }

    // Baustellen von Häfen mit einschließen
    for(const noBuildingSite* bldSite : harbor_building_sites_from_sea)
    {
        if(reason != TerritoryChangeReason::Destroyed || bldSite != &building)
            region.calcTerritoryOfBuilding(*bldSite);
    }
    cleanTerritoryRegion(region, reason, building);

    return region;
}

void GameWorld::cleanTerritoryRegion(TerritoryRegion& region, TerritoryChangeReason reason,
                                     const noBaseBuilding& triggerBld) const
{
    if(getGGS().isEnabled(AddonId::NO_ALLIED_PUSH))
    {
        const bool isHq = triggerBld.getBuildingType() == BuildingType::Headquarters;
        const auto newOwnerOfTriggerBld = region.getOwner(region.getPosFromMapPos(triggerBld.getPos()));
        // An HQ can be placed independently of the current owner.
        // So ensure the HQ position is always considered to belong to the HQ owner
        // if the TerritoryChangeReason is Build
        const auto ownerOfTriggerBld =
          isHq && reason == TerritoryChangeReason::Build ? newOwnerOfTriggerBld : getNode(triggerBld.getPos()).owner;

        RTTR_FOREACH_PT(Position, region.size)
        {
            const MapPoint curMapPt = makeMapPoint(pt + region.startPt);
            const auto oldOwner = getNode(curMapPt).owner;
            const auto newOwner = region.getOwner(pt);

            // If nothing changed, there is nothing to do (ownerChanged was already initialized)
            if(oldOwner == newOwner)
                continue;

            const bool ownersAllied = oldOwner > 0 && newOwner > 0 && getPlayer(oldOwner - 1).isAlly(newOwner - 1);
            if(
              // rule 1: only take territory from an ally if that ally loses a building
              // special case: headquarter can take territory
              (ownersAllied && (ownerOfTriggerBld != oldOwner || reason == TerritoryChangeReason::Build) && !isHq) ||
              // rule 2: do not gain territory when you lose a building (captured or destroyed)
              (ownerOfTriggerBld == newOwner && reason != TerritoryChangeReason::Build) ||
              // rule 3: do not lose territory when you gain a building (newBuilt or capture)
              ((ownerOfTriggerBld == oldOwner && oldOwner > 0 && reason == TerritoryChangeReason::Build)
               || (newOwnerOfTriggerBld == oldOwner && reason == TerritoryChangeReason::Captured)))
            {
                region.setOwner(pt, oldOwner);
            }
        }
    }

    // "Cosmetics": Remove points that do not border to territory to avoid edges of border stones
    RTTR_FOREACH_PT(Position, region.size)
    {
        uint8_t owner = region.getOwner(pt);
        if(!owner)
            continue;
        // Check if any neighbour is fully surrounded by player territory
        bool isPlayerTerritoryNear = false;
        for(const auto d : helpers::EnumRange<Direction>{})
        {
            Position neighbour = ::getNeighbour(pt + region.startPt, d);
            if(region.safeGetOwner(neighbour - region.startPt) != owner)
                continue;
            // Don't check this point as it is always true
            const Direction exceptDir = d + 3u;
            if(region.willBePlayerTerritory(neighbour, owner, exceptDir))
            {
                isPlayerTerritoryNear = true;
                break;
            }
        }

        // All good?
        if(isPlayerTerritoryNear)
            continue;
        // No neighbouring player territory found. Look for another
        uint8_t newOwner = 0;
        for(const auto d : helpers::EnumRange<Direction>{})
        {
            Position neighbour = ::getNeighbour(pt + region.startPt, d);
            uint8_t nbOwner = region.safeGetOwner(neighbour - region.startPt);
            // No or same player?
            if(!nbOwner || nbOwner == owner)
                continue;
            // Don't check this point as it would always fail
            const Direction exceptDir = d + 3u;
            // First one found gets it
            if(region.willBePlayerTerritory(neighbour, nbOwner, exceptDir))
            {
                newOwner = nbOwner;
                break;
            }
        }
        region.setOwner(pt, newOwner);
    }
}

void GameWorld::destroyPlayerRests(const MapPoint pt, unsigned char newOwner, const noBaseBuilding* exception)
{
    noBase* no = getNode(pt).obj;
    if(!no || no == exception)
        return;

    // Destroy only flags, buildings and building sites
    const NodalObjectType noType = no->getType();
    if(noType != NodalObjectType::Flag && noType != NodalObjectType::Building
       && noType != NodalObjectType::Buildingsite)
        return;

    // is the building on a node with a different owner? Border is allways destroyed.
    if(static_cast<noRoadNode*>(no)->getPlayer() + 1 == newOwner && !isBorderNode(pt, newOwner))
        return;

    // Do not destroy military buildings that hold territory on their own
    // Normally they will be on players territory but it can happen that they don't
    // Examples: Improved alliances or expedition building sites
    const noBase* noCheckMil = (noType == NodalObjectType::Flag) ? getNO(getNeighbour(pt, Direction::NorthWest)) : no;
    const GO_Type goType = noCheckMil->getGOT();
    if(goType == GO_Type::NobHq || goType == GO_Type::NobHarborbuilding
       || (goType == GO_Type::NobMilitary && !static_cast<const nobMilitary*>(noCheckMil)->isNewBuilt())
       || (noCheckMil->getType() == NodalObjectType::Buildingsite
           && static_cast<const noBuildingSite*>(noCheckMil)->isHarborBuildingSiteFromSea()))
    {
        // LOG.write(("destroyPlayerRests of hq, military, harbor or colony-harbor in construction stopped at x, %i y,
        // %i type, %i \n", x, y, noType);
        return;
    }

    // If it is a flag, destroy the building
    if(noType == NodalObjectType::Flag && (!exception || no != exception->getFlag()))
        static_cast<noFlag*>(no)->destroyAttachedBuilding();

    destroyNO(pt, false);
}

void GameWorld::roadNodeAvailable(const MapPoint pt)
{
    // Figuren direkt daneben
    for(const MapPoint nb : getNeighbours(pt))
    {
        // Nochmal prüfen, ob er nun wirklich verfügbar ist (evtl blocken noch mehr usw.)
        if(!isRoadNodeForFigures(pt))
            continue;

        // Figuren Bescheid sagen
        for(noBase& object : getFigures(nb))
        {
            if(object.getType() == NodalObjectType::Figure)
                static_cast<noFigure&>(object).nodeFreed(pt);
        }
    }
}

/// Kleine Klasse für Angriffsfunktion für einen potentielle angreifenden Soldaten
struct PotentialAttacker
{
    nofPassiveSoldier* soldier;
    /// Weglänge zum Angriffsziel
    unsigned distance;
};

void GameWorld::Attack(const unsigned char playerAttacker, const MapPoint pt, const unsigned short soldiers_count,
                       const bool strong_soldiers)
{
    auto* attacked_building = getSpecObj<nobBaseMilitary>(pt);
    if(!attacked_building || !attacked_building->isAttackable(playerAttacker))
        return;

    // Militärgebäude in der Nähe finden
    sortedMilitaryBlds buildings = lookForMilitaryBuildings(pt, 3);

    // Liste von verfügbaren Soldaten, geordnet einfügen, damit man dann starke oder schwache Soldaten nehmen kann
    std::list<PotentialAttacker> potentialAttackers;

    for(const auto* building : buildings)
    {
        // Muss ein Gebäude von uns sein und darf nur ein "normales Militärgebäude" sein (kein HQ etc.)
        if(building->getPlayer() != playerAttacker || !BuildingProperties::isMilitary(building->getBuildingType()))
            continue;

        const auto& milBld = static_cast<const nobMilitary&>(*building);
        unsigned numSoldiersForCurrentBld = milBld.getNumSoldiersForAttack(pt);
        if(!numSoldiersForCurrentBld)
            continue;

        // Take soldier(s)
        unsigned curNumSoldiers = 0;
        const unsigned distance = calcDistance(building->getPos(), pt);
        if(strong_soldiers)
        {
            // Strong soldiers first
            for(auto& curSoldier : helpers::reverse(milBld.getTroops()))
            {
                if(curNumSoldiers >= numSoldiersForCurrentBld)
                    break;
                ++curNumSoldiers;
                /* Insert new soldier before current one if:
                        new soldiers rank is greater
                        OR new soldiers rank is equal AND new soldiers distance is smaller */
                const auto itInsertPos = helpers::findIf(
                  potentialAttackers, [curRank = curSoldier.getRank(), distance](const PotentialAttacker& pa) {
                      return pa.soldier->getRank() < curRank
                             || (pa.soldier->getRank() == curRank && pa.distance > distance);
                  });
                potentialAttackers.emplace(itInsertPos, PotentialAttacker{&curSoldier, distance});
            }
        } else
        {
            // Weak soldiers first
            for(auto& curSoldier : milBld.getTroops())
            {
                if(curNumSoldiers >= numSoldiersForCurrentBld)
                    break;
                ++curNumSoldiers;
                /* Insert new soldier before current one if:
                           new soldiers rank is less
                           OR new soldiers rank is equal AND new soldiers distance is smaller */
                const auto itInsertPos = helpers::findIf(
                  potentialAttackers, [curRank = curSoldier.getRank(), distance](const PotentialAttacker& pa) {
                      return pa.soldier->getRank() > curRank
                             || (pa.soldier->getRank() == curRank && pa.distance > distance);
                  });
                potentialAttackers.emplace(itInsertPos, PotentialAttacker{&curSoldier, distance});
            }
        } // End weak/strong check
    }

    // Send the soldiers to attack
    unsigned curNumSoldiers = 0;

    for(PotentialAttacker& pa : potentialAttackers)
    {
        if(curNumSoldiers >= soldiers_count)
            break;
        pa.soldier->getHome()->sendAttacker(pa.soldier, *attacked_building);
        curNumSoldiers++;
    }

    if(curNumSoldiers > 0 && hasLua())
    {
        getLua().eventAttack(playerAttacker, attacked_building->getPlayer(), curNumSoldiers);
    }
}

/// Compare sea attackers by their rank, armor, then by their distance
template<class T_RankCmp>
struct CmpSeaAttacker : private T_RankCmp
{
    bool operator()(const GameWorldBase::PotentialSeaAttacker& lhs, const GameWorldBase::PotentialSeaAttacker& rhs)
    {
        // Sort after rank, then armor then distance then objId
        if(lhs.soldier->getRank() == rhs.soldier->getRank())
        {
            if(lhs.soldier->hasArmor() != rhs.soldier->hasArmor())
                return T_RankCmp::operator()(lhs.soldier->hasArmor() ? 1 : 0, rhs.soldier->hasArmor() ? 1 : 0);
            if(lhs.soldier->hasHealing() != rhs.soldier->hasHealing())
                return T_RankCmp::operator()(lhs.soldier->hasHealing() ? 1 : 0, rhs.soldier->hasHealing() ? 1 : 0);
            if(lhs.distance == rhs.distance)
                return (lhs.soldier->getObjId() < rhs.soldier->getObjId()); // tie breaker
            else
                return lhs.distance < rhs.distance;
        } else
            return T_RankCmp::operator()(lhs.soldier->getRank(), rhs.soldier->getRank());
    }
};

void GameWorld::attackViaSea(const unsigned char playerAttacker, const MapPoint pt, const unsigned short soldiers_count,
                             const bool strong_soldiers)
{
    // Verfügbare Soldaten herausfinden
    std::vector<GameWorldBase::PotentialSeaAttacker> attackers = getSoldiersForSeaAttack(playerAttacker, pt);
    if(attackers.empty())
        return;

    // Sort them
    if(strong_soldiers)
        helpers::sort(attackers, CmpSeaAttacker<std::greater<>>());
    else
        helpers::sort(attackers, CmpSeaAttacker<std::less<>>());

    auto& attacked_building = *getSpecObj<nobBaseMilitary>(pt);
    unsigned counter = 0;
    for(GameWorldBase::PotentialSeaAttacker& pa : attackers)
    {
        if(counter >= soldiers_count)
            break;
        pa.soldier->getHome()->sendAttacker(pa.soldier, attacked_building, pa.harbor);
        counter++;
    }

    if(counter > 0 && hasLua())
    {
        getLua().eventAttack(playerAttacker, attacked_building.getPlayer(), counter);
    }
}

TradePathCache& GameWorld::getTradePathCache()
{
    if(!tradePathCache)
        tradePathCache = std::make_unique<TradePathCache>(*this);
    return *tradePathCache;
}

void GameWorld::setEconHandler(std::unique_ptr<EconomyModeHandler> handler)
{
    RTTR_Assert_Msg(!econHandler, "Can't reset the economy mode handler ATM");
    econHandler = std::move(handler);
}

bool GameWorld::isRoadNodeForFigures(const MapPoint pt)
{
    // Figuren durchgehen, bei Kämpfen und wartenden Angreifern sowie anderen wartenden Figuren stoppen!
    for(const noBase& object : getFigures(pt))
    {
        // andere wartende Figuren
        /*
                ATTENTION! This leads to figures on the same node blocking each other. -> Ghost jams

                if((*it)->getType() == NodalObjectType::Figure)
                {
                    noFigure * fig = static_cast<noFigure*>(*it);
                    // Figuren dürfen sich nicht gegenüber stehen, sonst warten sie ja ewig aufeinander
                    // Außerdem muss auch die Position stimmen, sonst spinnt der ggf. rum, da
                    if(fig->isWaitingForFreeNode() && (fig->getDir()+3)%6 != dir)
                        return false;
                }*/

        // Kampf
        if(object.getGOT() == GO_Type::Fighting)
        {
            if(static_cast<const noFighting&>(object).isActive())
                return false;
        } else if(object.getGOT() == GO_Type::NofAttacker) // wartende Angreifer
        {
            if(static_cast<const nofAttacker&>(object).isBlockingRoads())
                return false;
        }
    }

    // alles ok
    return true;
}

/// Lässt alle Figuren, die auf diesen Punkt  auf Wegen zulaufen, anhalten auf dem Weg (wegen einem Kampf)
void GameWorld::stopOnRoads(const MapPoint pt, const helpers::OptionalEnum<Direction> dir)
{
    // Figuren drumherum sammeln (auch von dem Punkt hier aus)
    std::vector<noFigure*> figures;

    // Auch vom Ausgangspunkt aus, da sie im GameWorld wegem Zeichnen auch hier hängen können!
    for(auto& fieldFigure : getFigures(pt))
    {
        if(fieldFigure.getType() == NodalObjectType::Figure)
            figures.push_back(static_cast<noFigure*>(&fieldFigure));
    }

    // Und natürlich in unmittelbarer Umgebung suchen
    for(const MapPoint nb : getNeighbours(pt))
    {
        for(auto& fieldFigure : getFigures(nb))
        {
            if(fieldFigure.getType() == NodalObjectType::Figure)
                figures.push_back(static_cast<noFigure*>(&fieldFigure));
        }
    }

    for(auto& figure : figures)
    {
        if(dir && *dir + 3u == static_cast<noFigure*>(figure)->getCurMoveDir())
        {
            if(getNeighbour(pt, *dir) == static_cast<noFigure*>(figure)->getPos())
                continue;
        }

        // Derjenige muss ggf. stoppen, wenn alles auf ihn zutrifft
        static_cast<noFigure*>(figure)->stopIfNecessary(pt);
    }
}

void GameWorld::armageddon()
{
    RTTR_FOREACH_PT(MapPoint, getSize())
    {
        auto* flag = getSpecObj<noFlag>(pt);
        if(flag)
        {
            flag->destroyAttachedBuilding();
            destroyNO(pt, false);
        }
    }
}

void GameWorld::armageddon(const unsigned char player)
{
    RTTR_FOREACH_PT(MapPoint, getSize())
    {
        auto* flag = getSpecObj<noFlag>(pt);
        if(flag && flag->getPlayer() == player)
        {
            flag->destroyAttachedBuilding();
            destroyNO(pt, false);
        }
    }
}

bool GameWorld::validWaitingAroundBuildingPoint(const MapPoint pt, const MapPoint center)
{
    // Gültiger Punkt für Figuren?
    if(!PathConditionHuman(*this).isNodeOk(pt))
        return false;

    // Objekte, die sich hier befinden durchgehen
    for(const auto& figure : getFigures(pt))
    {
        // Ist hier ein anderer Soldat, der hier ebenfalls wartet?
        if(figure.getGOT() == GO_Type::NofAttacker || figure.getGOT() == GO_Type::NofAggressivedefender
           || figure.getGOT() == GO_Type::NofDefender)
        {
            const auto state = static_cast<const nofActiveSoldier&>(figure).getState();
            if(state == nofActiveSoldier::SoldierState::WaitingForFight
               || state == nofActiveSoldier::SoldierState::AttackingWaitingAroundBuilding)
                return false;
        }

        // Oder ein Kampf, der hier tobt?
        if(figure.getGOT() == GO_Type::Fighting)
            return false;
    }
    // object wall or impassable terrain increasing my path to target length to a higher value than the direct distance?
    return findHumanPath(pt, center, calcDistance(pt, center)).hasValue();
}

bool GameWorld::isValidPointForFighting(MapPoint pt, const nofActiveSoldier& soldier,
                                        bool avoid_military_building_flags)
{
    // Is this a flag of a military building?
    if(avoid_military_building_flags && getNO(pt)->getGOT() == GO_Type::Flag)
    {
        GO_Type got = getNO(getNeighbour(pt, Direction::NorthWest))->getGOT();
        if(got == GO_Type::NobMilitary || got == GO_Type::NobHarborbuilding || got == GO_Type::NobHq)
            return false;
    }

    // Objekte, die sich hier befinden durchgehen
    for(const auto& figure : getFigures(pt))
    {
        // Ist hier ein anderer Soldat, der hier ebenfalls wartet ?
        const GO_Type got = figure.getGOT();
        if(got == GO_Type::NofAttacker || got == GO_Type::NofAggressivedefender || got == GO_Type::NofDefender)
        {
            if(static_cast<const nofActiveSoldier*>(&figure) == &soldier)
                continue;
            switch(static_cast<const nofActiveSoldier&>(figure).getState())
            {
                default: break;
                case nofActiveSoldier::SoldierState::WaitingForFight:
                case nofActiveSoldier::SoldierState::AttackingWaitingAroundBuilding:
                case nofActiveSoldier::SoldierState::AttackingWaitingForDefender:
                case nofActiveSoldier::SoldierState::DefendingWaiting: return false;
            }
        } else if(got == GO_Type::Fighting) // Oder ein Kampf, der hier tobt?
        {
            const auto& fight = static_cast<const noFighting&>(figure);
            // A soldier currently fighting shouldn't look for a new fighting spot
            RTTR_Assert(!fight.isFighter(soldier));
            if(fight.isActive())
                return false;
        }
    }
    // Liegt hier was rum auf dem man nicht kämpfen sollte?
    const BlockingManner bm = getNO(pt)->getBM();
    return bm == BlockingManner::None || bm == BlockingManner::Tree || bm == BlockingManner::Flag;
}

bool GameWorld::isPointCompletelyVisible(const MapPoint& pt, unsigned char player,
                                         const noBaseBuilding* exception) const
{
    sortedMilitaryBlds buildings = lookForMilitaryBuildings(pt, 3);

    // Sichtbereich von Militärgebäuden
    for(const nobBaseMilitary* milBld : buildings)
    {
        if(milBld->getPlayer() == player && milBld != exception)
        {
            // Prüfen, obs auch unbesetzt ist
            if(milBld->getGOT() == GO_Type::NobMilitary)
            {
                if(static_cast<const nobMilitary*>(milBld)->isNewBuilt())
                    continue;
            }

            if(calcDistance(pt, milBld->getPos()) <= unsigned(milBld->getMilitaryRadius() + VISUALRANGE_MILITARY))
                return true;
        }
    }

    // Sichtbereich von Hafenbaustellen
    for(const noBuildingSite* bldSite : harbor_building_sites_from_sea)
    {
        if(bldSite->getPlayer() == player && bldSite != exception)
        {
            if(calcDistance(pt, bldSite->getPos()) <= unsigned(HARBOR_RADIUS + VISUALRANGE_MILITARY))
                return true;
        }
    }

    // Sichtbereich von Spähtürmen
    for(const nobUsual* bld : getPlayer(player).getBuildingRegister().getBuildings(BuildingType::LookoutTower)) //-V807
    {
        // Ist Späturm überhaupt besetzt?
        if(!bld->hasWorker())
            continue;

        // Nicht die Ausnahme wählen
        if(bld == exception)
            continue;

        // Liegt Spähturm innerhalb des Sichtradius?
        if(calcDistance(pt, bld->getPos()) <= VISUALRANGE_LOOKOUTTOWER)
            return true;
    }

    // Check scouts and soldiers
    const unsigned range = std::max(VISUALRANGE_SCOUT, VISUALRANGE_SOLDIER);
    if(checkPointsInRadius(
         pt, range,
         [this, player](auto pt, auto distance) { return this->isScoutingFigureOnNode(pt, player, distance); }, true))
    {
        return true;
    }
    return isPointScoutedByShip(pt, player);
}

bool GameWorld::isScoutingFigureOnNode(const MapPoint& pt, unsigned player, unsigned distance) const
{
    static_assert(VISUALRANGE_SCOUT >= VISUALRANGE_SOLDIER, "Visual range changed. Check loop below!");

    // Späher/Soldaten in der Nähe prüfen und direkt auf dem Punkt
    for(const noBase& obj : getFigures(pt))
    {
        const GO_Type got = obj.getGOT();
        // Check for scout. Note: no need to check for distance as scouts have higher distance than soldiers
        if(got == GO_Type::NofScoutFree)
        {
            // Prüfen, ob er auch am Erkunden ist und an der Position genau und ob es vom richtigen Spieler ist
            if(static_cast<const nofScout_Free&>(obj).getPlayer() == player)
                return true;
            else
                continue;
        } else if(distance <= VISUALRANGE_SOLDIER)
        {
            // Soldaten?
            if(got == GO_Type::NofAttacker || got == GO_Type::NofAggressivedefender)
            {
                if(static_cast<const nofActiveSoldier&>(obj).getPlayer() == player)
                    return true;
            }
            // Kämpfe (wo auch Soldaten drin sind)
            else if(got == GO_Type::Fighting)
            {
                // Prüfen, ob da ein Soldat vom angegebenen Spieler dabei ist
                if(static_cast<const noFighting&>(obj).isSoldierOfPlayer(player))
                    return true;
            }
        }
    }

    return false;
}

bool GameWorld::isPointScoutedByShip(const MapPoint& pt, unsigned player) const
{
    const std::vector<noShip*>& ships = getPlayer(player).getShips();
    for(const noShip* ship : ships)
    {
        unsigned shipDistance = calcDistance(pt, ship->getPos());
        if(shipDistance <= ship->getVisualRange())
            return true;
    }
    return false;
}

void GameWorld::recalcVisibility(const MapPoint pt, const unsigned char player, const noBaseBuilding* const exception)
{
    /// Zustand davor merken
    Visibility visibilityBefore = getNode(pt).fow[player].visibility;

    /// Herausfinden, ob vollständig sichtbar
    bool visible = isPointCompletelyVisible(pt, player, exception);

    // Vollständig sichtbar --> vollständig sichtbar logischerweise
    if(visible)
        makeVisible(pt, player);
    else
    {
        // nicht mehr sichtbar
        // Je nach vorherigen Zustand und Einstellung entscheiden
        switch(getGGS().exploration)
        {
            case Exploration::Disabled:
            case Exploration::Classic:
                // einmal sichtbare Bereiche bleiben erhalten
                // nichts zu tun
                break;
            case Exploration::FogOfWar:
            case Exploration::FogOfWarExplored:
                // wenn es mal sichtbar war, nun im Nebel des Krieges
                if(visibilityBefore == Visibility::Visible)
                {
                    setVisibility(pt, player, Visibility::FogOfWar, getEvMgr().getCurrentGF());
                }
                break;
            default: throw std::logic_error("Invalid exploration value");
        }
    }
}

void GameWorld::makeVisible(const MapPoint pt, const unsigned char player)
{
    setVisibility(pt, player, Visibility::Visible);
}

void GameWorld::recalcVisibilitiesAroundPoint(const MapPoint pt, const MapCoord radius, const unsigned char player,
                                              const noBaseBuilding* const exception)
{
    std::vector<MapPoint> pts = getPointsInRadiusWithCenter(pt, radius);
    for(const MapPoint& pt : pts)
        recalcVisibility(pt, player, exception);
}

/// Setzt die Sichtbarkeiten um einen Punkt auf sichtbar (aus Performancegründen Alternative zu oberem)
void GameWorld::makeVisibleAroundPoint(const MapPoint pt, const MapCoord radius, const unsigned char player)
{
    std::vector<MapPoint> pts = getPointsInRadiusWithCenter(pt, radius);
    for(const MapPoint& curPt : pts)
        makeVisible(curPt, player);
}

/// Bestimmt bei der Bewegung eines spähenden Objekts die Sichtbarkeiten an
/// den Rändern neu
void GameWorld::recalcMovingVisibilities(const MapPoint pt, const unsigned char player, const MapCoord radius,
                                         const Direction moving_dir, MapPoint* enemyTerritory)
{
    // Neue Sichtbarkeiten zuerst setzen
    // Zum Eckpunkt der beiden neuen sichtbaren Kanten gehen
    MapPoint t(pt);
    for(MapCoord i = 0; i < radius; ++i)
        t = getNeighbour(t, moving_dir);

    // Und zu beiden Abzweigungen weiter gehen und Punkte auf visible setzen
    makeVisible(t, player);
    MapPoint tt(t);
    Direction dir = moving_dir + 2u;
    for(MapCoord i = 0; i < radius; ++i)
    {
        tt = getNeighbour(tt, dir);
        // Sichtbarkeit und für FOW-Gebiet vorherigen Besitzer merken
        // (d.h. der dort  zuletzt war, als es für Spieler player sichtbar war)
        Visibility oldVis = calcVisiblityWithAllies(tt, player);
        unsigned char oldOwner = getNode(tt).fow[player].owner;
        makeVisible(tt, player);
        // Neues feindliches Gebiet entdeckt?
        // Muss vorher undaufgedeckt oder FOW gewesen sein, aber in dem Fall darf dort vorher noch kein
        // Territorium entdeckt worden sein
        unsigned char currentOwner = getNode(tt).owner;
        if(currentOwner
           && (oldVis == Visibility::Invisible || (oldVis == Visibility::FogOfWar && oldOwner != currentOwner)))
        {
            if(getPlayer(player).isAttackable(currentOwner - 1) && enemyTerritory)
            {
                *enemyTerritory = tt;
            }
        }
    }

    tt = t;
    dir = moving_dir - 2u;
    for(MapCoord i = 0; i < radius; ++i)
    {
        tt = getNeighbour(tt, dir);
        // Sichtbarkeit und für FOW-Gebiet vorherigen Besitzer merken
        // (d.h. der dort  zuletzt war, als es für Spieler player sichtbar war)
        Visibility oldVis = calcVisiblityWithAllies(tt, player);
        unsigned char oldOwner = getNode(tt).fow[player].owner;
        makeVisible(tt, player);
        // Neues feindliches Gebiet entdeckt?
        // Muss vorher undaufgedeckt oder FOW gewesen sein, aber in dem Fall darf dort vorher noch kein
        // Territorium entdeckt worden sein
        unsigned char currentOwner = getNode(tt).owner;
        if(currentOwner
           && (oldVis == Visibility::Invisible || (oldVis == Visibility::FogOfWar && oldOwner != currentOwner)))
        {
            if(getPlayer(player).isAttackable(currentOwner - 1) && enemyTerritory)
            {
                *enemyTerritory = tt;
            }
        }
    }

    // Dasselbe für die zurückgebliebenen Punkte
    // Diese müssen allerdings neu berechnet werden!
    t = pt;
    Direction antiMovingDir = moving_dir + 3u;
    for(MapCoord i = 0; i < radius + 1; ++i)
        t = getNeighbour(t, antiMovingDir);

    recalcVisibility(t, player, nullptr);
    tt = t;
    dir = antiMovingDir + 2u;
    for(MapCoord i = 0; i < radius; ++i)
    {
        tt = getNeighbour(tt, dir);
        recalcVisibility(tt, player, nullptr);
    }

    tt = t;
    dir = antiMovingDir - 2u;
    for(unsigned i = 0; i < radius; ++i)
    {
        tt = getNeighbour(tt, dir);
        recalcVisibility(tt, player, nullptr);
    }
}

bool GameWorld::isBorderNode(const MapPoint pt, const unsigned char owner) const
{
    return (getNode(pt).owner == owner && !isPlayerTerritory(pt, owner));
}

/// Gründet vom Schiff aus eine neue Kolonie
bool GameWorld::foundColony(const HarborId harbor, const unsigned char player, const SeaId seaId)
{
    // Ist es hier überhaupt noch möglich, eine Kolonie zu gründen?
    if(!isHarborAtSea(harbor, seaId) || !isHarborPointFree(harbor, player))
        return false;

    MapPoint pos(getHarborPoint(harbor));
    destroyNO(pos, false);

    // Hafenbaustelle errichten
    auto* bs = new noBuildingSite(pos, player);
    setNO(pos, bs);
    addHarborBuildingSiteFromSea(bs);

    if(gi)
        gi->GI_UpdateMinimap(pos);

    recalcTerritory(*bs, TerritoryChangeReason::Build);
    // BQ neu berechnen (evtl durch recalcTerritory noch nicht geschehen)
    recalcBQAroundPointBig(pos);

    getNotifications().publish(ExpeditionNote(ExpeditionNote::ColonyFounded, player, pos));

    return true;
}

void GameWorld::removeHarborBuildingSiteFromSea(noBuildingSite* building_site)
{
    RTTR_Assert(building_site->getBuildingType() == BuildingType::HarborBuilding);
    harbor_building_sites_from_sea.remove(building_site);
}

bool GameWorld::isHarborBuildingSiteFromSea(const noBuildingSite* building_site) const
{
    return helpers::contains(harbor_building_sites_from_sea, building_site);
}

std::vector<HarborId> GameWorld::getUnexploredHarborPoints(const HarborId hbIdToSkip, const SeaId seaId,
                                                           unsigned playerId) const
{
    std::vector<HarborId> hps;
    for(const auto i : helpers::idRange<HarborId>(getNumHarborPoints()))
    {
        if(i == hbIdToSkip || !isHarborAtSea(i, seaId))
            continue;
        if(calcVisiblityWithAllies(getHarborPoint(i), playerId) != Visibility::Visible)
            hps.push_back(i);
    }
    return hps;
}

MapNode& GameWorld::getNodeWriteable(const MapPoint pt)
{
    return getNodeInt(pt);
}

void GameWorld::visibilityChanged(const MapPoint pt, unsigned player, Visibility oldVis, Visibility newVis)
{
    GameWorldBase::visibilityChanged(pt, player, oldVis, newVis);
    if(oldVis == Visibility::Invisible && newVis == Visibility::Visible && hasLua())
        getLua().eventExplored(player, pt, getNode(pt).owner);
    // Minimap Bescheid sagen
    if(gi)
        gi->GI_UpdateMinimap(pt);
}
