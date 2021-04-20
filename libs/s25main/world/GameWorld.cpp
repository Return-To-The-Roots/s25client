// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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
#include "helpers/containerUtils.h"
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
#include <cmath>
#include <functional>
#include <set>
#include <stdexcept>

inline std::vector<GamePlayer> CreatePlayers(const std::vector<PlayerInfo>& playerInfos, GameWorld& world)
{
    std::vector<GamePlayer> players;
    players.reserve(playerInfos.size());
    for(unsigned i = 0; i < playerInfos.size(); ++i)
        players.push_back(GamePlayer(i, playerInfos[i], world));
    return players;
}

GameWorld::GameWorld(const std::vector<PlayerInfo>& players, const GlobalGameSettings& gameSettings, EventManager& em)
    : GameWorldBase(CreatePlayers(players, *this), gameSettings, em)
{
    GameObject::AttachWorld(this);
}

GameWorld::~GameWorld()
{
    GameObject::DetachWorld(this);
}

MilitarySquares& GameWorld::GetMilitarySquares()
{
    return militarySquares;
}

void GameWorld::SetFlag(const MapPoint pt, const unsigned char player)
{
    if(GetBQ(pt, player) == BuildingQuality::Nothing)
        return;
    // There must be no other flag around that point
    if(IsFlagAround(pt))
        return;

    // Gucken, nicht, dass schon eine Flagge dasteht
    if(GetNO(pt)->GetType() != NodalObjectType::Flag)
    {
        DestroyNO(pt, false);
        SetNO(pt, new noFlag(pt, player));

        RecalcBQAroundPointBig(pt);
    }
}

void GameWorld::DestroyFlag(const MapPoint pt, unsigned char playerId)
{
    // Let's see if there is a flag
    if(GetNO(pt)->GetType() == NodalObjectType::Flag)
    {
        auto* flag = GetSpecObj<noFlag>(pt);
        if(flag->GetPlayer() != playerId)
            return;

        // Get the attached building if existing
        noBase* building = GetNO(GetNeighbour(pt, Direction::NorthWest));

        // Is this a military building?
        if(building->GetGOT() == GO_Type::NobMilitary)
        {
            // Maybe demolition of the building is not allowed?
            if(!static_cast<nobMilitary*>(building)->IsDemolitionAllowed())
                return; // Abort the whole thing
        }

        // Demolish, also the building
        flag->DestroyAttachedBuilding();

        DestroyNO(pt, false);
        RecalcBQAroundPointBig(pt);
    }

    if(gi)
        gi->GI_FlagDestroyed(pt);
}

void GameWorld::SetPointRoad(MapPoint pt, const Direction dir, const PointRoad type)
{
    const RoadDir rDir = toRoadDir(pt, dir);
    SetRoad(pt, rDir, type);

    if(gi)
        gi->GI_UpdateMinimap(pt);
}

void GameWorld::SetBuildingSite(const BuildingType type, const MapPoint pt, const unsigned char player)
{
    if(!GetPlayer(player).IsBuildingEnabled(type))
        return;

    // Gucken, ob das Gebäude hier überhaupt noch gebaut wrden kann
    if(!canUseBq(GetBQ(pt, player), BUILDING_SIZE[type]))
        return;

    // Wenn das ein Militärgebäude ist und andere Militärgebäude bereits in der Nähe sind, darf dieses nicht gebaut
    // werden
    if(BuildingProperties::IsMilitary(type))
    {
        if(IsMilitaryBuildingNearNode(pt, player))
            return;
    }

    // Prüfen ob Katapult und ob Katapult erlaubt ist
    if(type == BuildingType::Catapult && !GetPlayer(player).CanBuildCatapult())
        return;

    DestroyNO(pt, false);

    // Baustelle setzen
    SetNO(pt, new noBuildingSite(type, pt, player));
    if(gi)
        gi->GI_UpdateMinimap(pt);

    // Bauplätze drumrum neu berechnen
    RecalcBQAroundPointBig(pt);
}

void GameWorld::DestroyBuilding(const MapPoint pt, const unsigned char player)
{
    // Steht da auch ein Gebäude oder eine Baustelle, nicht dass wir aus Verzögerung Feuer abreißen wollen, das geht
    // schief
    if(GetNO(pt)->GetType() == NodalObjectType::Building || GetNO(pt)->GetType() == NodalObjectType::Buildingsite)
    {
        auto* nbb = GetSpecObj<noBaseBuilding>(pt);

        // Ist das Gebäude auch von dem Spieler, der es abreißen will?
        if(nbb->GetPlayer() != player)
            return;

        // Militärgebäude?
        if(nbb->GetGOT() == GO_Type::NobMilitary)
        {
            // Darf das Gebäude abgerissen werden?
            if(!static_cast<nobMilitary*>(nbb)->IsDemolitionAllowed())
                return;
        }

        DestroyNO(pt);
        // Bauplätze drumrum neu berechnen
        RecalcBQAroundPointBig(pt);
    }
}

void GameWorld::BuildRoad(const unsigned char playerId, const bool boat_road, const MapPoint start,
                          const std::vector<Direction>& route)
{
    // No routes with less than 2 parts. Actually invalid!
    if(route.size() < 2)
    {
        RTTR_Assert(false);
        return;
    }

    if(!GetSpecObj<noFlag>(start) || GetSpecObj<noFlag>(start)->GetPlayer() != playerId)
    {
        GetNotifications().publish(RoadNote(RoadNote::ConstructionFailed, playerId, start, route));
        return;
    }

    // Gucken, ob der Weg überhaupt noch gebaut werden kann
    PathConditionRoad<GameWorldBase> roadChecker(*this, boat_road);
    MapPoint curPt(start);
    for(unsigned i = 0; i + 1 < route.size(); ++i)
    {
        bool roadOk = roadChecker.IsEdgeOk(curPt, route[i]);
        curPt = GetNeighbour(curPt, route[i]);
        roadOk &= roadChecker.IsNodeOk(curPt);
        if(!roadOk)
        {
            // Nein? Dann prüfen ob genau der gewünscht Weg schon da ist
            if(!RoadAlreadyBuilt(boat_road, start, route))
                GetNotifications().publish(RoadNote(RoadNote::ConstructionFailed, playerId, start, route));
            return;
        }
    }

    curPt = GetNeighbour(curPt, route.back());

    // Prüfen, ob am Ende auch eine Flagge steht oder eine gebaut werden kann
    if(GetNO(curPt)->GetGOT() == GO_Type::Flag)
    {
        // Falscher Spieler?
        if(GetSpecObj<noFlag>(curPt)->GetPlayer() != playerId)
        {
            GetNotifications().publish(RoadNote(RoadNote::ConstructionFailed, playerId, start, route));
            return;
        }
    } else
    {
        // Check if we can build a flag there
        if(GetBQ(curPt, playerId) == BuildingQuality::Nothing || IsFlagAround(curPt))
        {
            GetNotifications().publish(RoadNote(RoadNote::ConstructionFailed, playerId, start, route));
            return;
        }
        // keine Flagge bisher aber spricht auch nix gegen ne neue Flagge -> Flagge aufstellen!
        SetFlag(curPt, playerId);
    }

    // Evtl Zierobjekte abreißen (Anfangspunkt)
    if(HasRemovableObjForRoad(start))
        DestroyNO(start);

    MapPoint end(start);
    for(auto i : route)
    {
        SetPointRoad(end, i, boat_road ? PointRoad::Boat : PointRoad::Normal);
        RecalcBQForRoad(end);
        end = GetNeighbour(end, i);

        // Evtl Zierobjekte abreißen
        if(HasRemovableObjForRoad(end))
            DestroyNO(end);
    }

    auto* rs = new RoadSegment(boat_road ? RoadType::Water : RoadType::Normal, GetSpecObj<noFlag>(start),
                               GetSpecObj<noFlag>(end), route);

    GetSpecObj<noFlag>(start)->SetRoute(route.front(), rs);
    GetSpecObj<noFlag>(end)->SetRoute(route.back() + 3u, rs);

    // Der Wirtschaft mitteilen, dass eine neue Straße gebaut wurde, damit sie alles Nötige macht
    GetPlayer(playerId).NewRoadConnection(rs);
    GetNotifications().publish(RoadNote(RoadNote::Constructed, playerId, start, route));
}

bool GameWorld::HasRemovableObjForRoad(const MapPoint pt) const
{
    const auto* obj = GetSpecObj<noStaticObject>(pt);
    return obj && obj->GetSize() == 0;
}

// When defined the game tries to remove "blocks" of border stones that look ugly (TODO: Example?)
// DISABLED: This currently leads to bugs. If you enable/fix this, please add tests and document the conditions this
// tries to fix
//#define PREVENT_BORDER_STONE_BLOCKING

void GameWorld::RecalcBorderStones(Position startPt, Extent areaSize)
{
    // Add a bit extra space as this influences also border stones around the region
    // But not so much we wrap completely around the map (+1 to round up, /2 to have extra space centered)
    Position EXTRA_SPACE = elMin(Position::all(3), (GetSize() - areaSize + Position::all(1)) / 2);
    startPt -= EXTRA_SPACE;
    areaSize += 2u * Extent(EXTRA_SPACE);
    // We might still be 1 node to big, make sure we have don't exceed the mapsize
    areaSize = elMin(areaSize, Extent(GetSize()));

#ifdef PREVENT_BORDER_STONE_BLOCKING
    // Store how many neighbors a border stone has
    std::vector<uint8_t> neighbors(areaSize.x * areaSize.y, 0);
#endif

    RTTR_FOREACH_PT(Position, areaSize)
    {
        // Make map point
        const MapPoint curMapPt = MakeMapPoint(pt + startPt);
        const unsigned char owner = GetNode(curMapPt).owner;
        BoundaryStones& boundaryStones = GetBoundaryStones(curMapPt);

        // Is this a border node?
        if(owner && IsBorderNode(curMapPt, owner))
        {
            // Check which neighbors are also border nodes and place the half-way stones to them
            for(const auto bPos : helpers::EnumRange<BorderStonePos>{})
            {
                if(bPos == BorderStonePos::OnPoint || IsBorderNode(GetNeighbour(curMapPt, toDirection(bPos)), owner))
                    boundaryStones[bPos] = owner;
                else
                    boundaryStones[bPos] = 0;
            }

#ifdef PREVENT_BORDER_STONE_BLOCKING
            // Count number of border nodes with same owner
            int idx = pt.y * width + pt.x;
            for(const MapPoint nb : GetNeighbours(curMapPt))
            {
                if(GetNode(nb).boundary_stones[0] == owner)
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
        const MapPoint curMapPt = MakeMapPoint(pt + startPt);

        // Do we have a stone here?
        const unsigned char owner = GetNode(curMapPt).boundary_stones[BorderStonePos::OnPoint];
        if(!owner)
            continue;

        int idx = pt.y * width + pt.x;
        if(neighbors[idx] < 3)
            continue;

        for(unsigned dir = 0; dir < 3 && neighbors[idx] > 2; ++dir)
        {
            // Do we have a border stone of the same owner on the node in that direction?
            BoundaryStones& nbBoundStones = GetBoundaryStones(GetNeighbour(curMapPt, dir + 3));

            if(nbBoundStones[0] != owner)
                continue;

            Position pa = ::GetNeighbour(pt, Direction(dir + 3));
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

void GameWorld::RecalcTerritory(const noBaseBuilding& building, TerritoryChangeReason reason)
{
    // Additional radius to eliminate border stones or odd remaining territory parts
    static const int ADD_RADIUS = 2;
    // Get the military radius this building affects. Bld is either a military building or a harbor building site
    RTTR_Assert(
      (building.GetBuildingType() == BuildingType::HarborBuilding && dynamic_cast<const noBuildingSite*>(&building))
      || dynamic_cast<const nobBaseMilitary*>(&building));
    const unsigned militaryRadius = building.GetMilitaryRadius();
    RTTR_Assert(militaryRadius > 0u);

    const TerritoryRegion region = CreateTerritoryRegion(building, militaryRadius + ADD_RADIUS, reason);

    std::vector<MapPoint> ptsWithChangedOwners;
    std::vector<int> sizeChanges(GetNumPlayers());

    // Copy owners from territory region to map and do the bookkeeping
    RTTR_FOREACH_PT(Position, region.size)
    {
        const MapPoint curMapPt = MakeMapPoint(pt + region.startPt);
        const uint8_t oldOwner = GetNode(curMapPt).owner;
        const uint8_t newOwner = region.GetOwner(pt);

        // If nothing changed, there is nothing to do (ownerChanged was already initialized)
        if(oldOwner == newOwner)
            continue;

        SetOwner(curMapPt, newOwner);
        ptsWithChangedOwners.push_back(curMapPt);
        if(newOwner != 0)
            sizeChanges[newOwner - 1]++;
        if(oldOwner != 0)
            sizeChanges[oldOwner - 1]--;
    }

    std::set<MapPoint, MapPointLess> ptsHandled;
    // Destroy everything from old player on all nodes where the owner has changed
    for(const MapPoint& curMapPt : ptsWithChangedOwners)
    {
        // Destroy everything around this point as this is at best a border node where nothing should be around
        // Do not destroy the triggering building or its flag
        // TODO: What about this point?
        const uint8_t owner = GetNode(curMapPt).owner;
        for(const MapPoint neighbourPt : GetNeighbours(curMapPt))
        {
            if(ptsHandled.insert(neighbourPt).second)
                DestroyPlayerRests(neighbourPt, owner, &building);
        }

        if(gi)
            gi->GI_UpdateMinimap(curMapPt);
    }

    // Destroy remaining roads going through non-owned territory
    for(const MapPoint& curMapPt : ptsWithChangedOwners)
    {
        // Skip if there is an object. We are looking only for roads going through, not ending here
        // (objects here are already destroyed and if the road ended there it would have been as well)
        if(GetNode(curMapPt).obj)
            continue;
        Direction dir;
        noFlag* flag = GetRoadFlag(curMapPt, dir);
        if(!flag || flag->GetPlayer() + 1 == GetNode(curMapPt).owner)
            continue;
        flag->DestroyRoad(dir);
    }

    // Notify
    for(const MapPoint& curMapPt : ptsWithChangedOwners)
        GetNotifications().publish(NodeNote(NodeNote::Owner, curMapPt));

    for(const MapPoint& pt : ptsHandled)
    {
        // BQ neu berechnen
        RecalcBQ(pt);
        // ggf den noch darüber, falls es eine Flagge war (kann ja ein Gebäude entstehen)
        const MapPoint neighbourPt = GetNeighbour(pt, Direction::NorthWest);
        if(GetNode(neighbourPt).bq != BuildingQuality::Nothing)
            RecalcBQ(neighbourPt);
    }

    RecalcBorderStones(region.startPt, region.size);

    // Recalc visibilities if building was destroyed
    // Otherwise just set everything to visible
    const unsigned visualRadius = militaryRadius + VISUALRANGE_MILITARY;
    if(reason == TerritoryChangeReason::Destroyed)
        RecalcVisibilitiesAroundPoint(building.GetPos(), visualRadius, building.GetPlayer(), &building);
    else
        MakeVisibleAroundPoint(building.GetPos(), visualRadius, building.GetPlayer());

    // Notify players
    for(unsigned i = 0; i < GetNumPlayers(); ++i)
    {
        GetPlayer(i).ChangeStatisticValue(StatisticType::Country, sizeChanges[i]);

        // Negatives Wachstum per Post dem/der jeweiligen Landesherren/dame melden, nur bei neugebauten Gebäuden
        if(reason == TerritoryChangeReason::Build && sizeChanges[i] < 0)
        {
            GetPostMgr().SendMsg(i, std::make_unique<PostMsgWithBuilding>(GetEvMgr().GetCurrentGF(),
                                                                          _("Lost land by this building"),
                                                                          PostCategory::Military, building));
            GetNotifications().publish(
              BuildingNote(BuildingNote::LostLand, i, building.GetPos(), building.GetBuildingType()));
        }
    }

    // Notify script
    if(HasLua())
    {
        for(const MapPoint& pt : ptsWithChangedOwners)
        {
            const uint8_t newOwner = GetNode(pt).owner;
            // Event for map scripting
            if(newOwner != 0)
                GetLua().EventOccupied(newOwner - 1, pt);
        }
    }
}

bool GameWorld::DoesDestructionChangeTerritory(const noBaseBuilding& building) const
{
    // Get the military radius this building affects. Bld is either a military building or a harbor building site
    RTTR_Assert(
      (building.GetBuildingType() == BuildingType::HarborBuilding && dynamic_cast<const noBuildingSite*>(&building))
      || dynamic_cast<const nobBaseMilitary*>(&building));
    const unsigned militaryRadius = building.GetMilitaryRadius();
    RTTR_Assert(militaryRadius > 0u);

    const TerritoryRegion region = CreateTerritoryRegion(building, militaryRadius, TerritoryChangeReason::Destroyed);

    // Look for a node that changed its owner and is important. If any -> return true
    RTTR_FOREACH_PT(Position, region.size)
    {
        MapPoint curMapPt = MakeMapPoint(pt + region.startPt);
        const MapNode& node = GetNode(curMapPt);
        if(node.owner == region.GetOwner(pt))
            continue;
        // AI can ignore water/snow/lava/swamp terrain (because it wouldn't help win the game)
        // So we check if any terrain is usable and if it is -> Land is important
        if(GetDescription().get(node.t1).Is(ETerrain::Walkable) && GetDescription().get(node.t2).Is(ETerrain::Walkable))
            return true;
        // also check neighboring nodes since border will still count as player territory but not allow any buildings!
        for(const MapPoint neighbourPt : GetNeighbours(curMapPt))
        {
            const MapNode& nNode = GetNode(neighbourPt);
            if(GetDescription().get(nNode.t1).Is(ETerrain::Walkable)
               || GetDescription().get(nNode.t2).Is(ETerrain::Walkable))
                return true;
        }
    }
    return false;
}

TerritoryRegion GameWorld::CreateTerritoryRegion(const noBaseBuilding& building, unsigned radius,
                                                 TerritoryChangeReason reason) const
{
    const MapPoint bldPos = building.GetPos();

    // Span at most half the map size (assert even sizes, given due to layout)
    RTTR_Assert(GetWidth() % 2 == 0);
    RTTR_Assert(GetHeight() % 2 == 0);
    Extent halfSize(GetSize() / 2u);
    Extent radius2D = elMin(Extent::all(radius), halfSize);

    // Koordinaten erzeugen für TerritoryRegion
    const Position startPt = Position(bldPos) - radius2D;
    // If we want to check the same number of points right of bld as left we need a +1.
    // But we can't check more than the whole map.
    const Extent size = elMin(2u * radius2D + Extent(1, 1), Extent(GetSize()));
    TerritoryRegion region(startPt, size, *this);

    // Alle Gebäude ihr Terrain in der Nähe neu berechnen
    sortedMilitaryBlds buildings = LookForMilitaryBuildings(bldPos, 3);
    for(const nobBaseMilitary* milBld : buildings)
    {
        if(!(reason == TerritoryChangeReason::Destroyed && milBld == &building))
            region.CalcTerritoryOfBuilding(*milBld);
    }

    // Baustellen von Häfen mit einschließen
    for(const noBuildingSite* bldSite : harbor_building_sites_from_sea)
    {
        if(!(reason == TerritoryChangeReason::Destroyed && bldSite == &building))
            region.CalcTerritoryOfBuilding(*bldSite);
    }
    CleanTerritoryRegion(region, reason, building);

    return region;
}

void GameWorld::CleanTerritoryRegion(TerritoryRegion& region, TerritoryChangeReason reason,
                                     const noBaseBuilding& triggerBld) const
{
    if(GetGGS().isEnabled(AddonId::NO_ALLIED_PUSH))
    {
        const unsigned char ownerOfTriggerBld = GetNode(triggerBld.GetPos()).owner;
        const unsigned char newOwnerOfTriggerBld = region.GetOwner(region.GetPosFromMapPos(triggerBld.GetPos()));

        RTTR_FOREACH_PT(Position, region.size)
        {
            const MapPoint curMapPt = MakeMapPoint(pt + region.startPt);
            const unsigned char oldOwner = GetNode(curMapPt).owner;
            const unsigned char newOwner = region.GetOwner(pt);

            // If nothing changed, there is nothing to do (ownerChanged was already initialized)
            if(oldOwner == newOwner)
                continue;

            // rule 1: only take territory from an ally if that ally loses a building - special case: headquarter can
            // take territory
            const bool ownersAllied = oldOwner > 0 && newOwner > 0 && GetPlayer(oldOwner - 1).IsAlly(newOwner - 1);
            if((ownersAllied && (ownerOfTriggerBld != oldOwner || reason == TerritoryChangeReason::Build)
                && triggerBld.GetBuildingType() != BuildingType::Headquarters)
               ||
               // rule 2: do not gain territory when you lose a building (captured or destroyed)
               (ownerOfTriggerBld == newOwner && reason != TerritoryChangeReason::Build) ||
               // rule 3: do not lose territory when you gain a building (newBuilt or capture)
               ((ownerOfTriggerBld == oldOwner && oldOwner > 0 && reason == TerritoryChangeReason::Build)
                || (newOwnerOfTriggerBld == oldOwner && reason == TerritoryChangeReason::Captured)))
            {
                region.SetOwner(pt, oldOwner);
            }
        }
    }

    // "Cosmetics": Remove points that do not border to territory to avoid edges of border stones
    RTTR_FOREACH_PT(Position, region.size)
    {
        uint8_t owner = region.GetOwner(pt);
        if(!owner)
            continue;
        // Check if any neighbour is fully surrounded by player territory
        bool isPlayerTerritoryNear = false;
        for(const auto d : helpers::EnumRange<Direction>{})
        {
            Position neighbour = ::GetNeighbour(pt + region.startPt, d);
            if(region.SafeGetOwner(neighbour - region.startPt) != owner)
                continue;
            // Don't check this point as it is always true
            const Direction exceptDir = d + 3u;
            if(region.WillBePlayerTerritory(neighbour, owner, exceptDir))
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
            Position neighbour = ::GetNeighbour(pt + region.startPt, d);
            uint8_t nbOwner = region.SafeGetOwner(neighbour - region.startPt);
            // No or same player?
            if(!nbOwner || nbOwner == owner)
                continue;
            // Don't check this point as it would always fail
            const Direction exceptDir = d + 3u;
            // First one found gets it
            if(region.WillBePlayerTerritory(neighbour, nbOwner, exceptDir))
            {
                newOwner = nbOwner;
                break;
            }
        }
        region.SetOwner(pt, newOwner);
    }
}

void GameWorld::CreateTradeGraphs()
{
    // Only if trade is enabled
    if(GetGGS().isEnabled(AddonId::TRADE))
        tradePathCache = std::make_unique<TradePathCache>(*this);
}

void GameWorld::DestroyPlayerRests(const MapPoint pt, unsigned char newOwner, const noBaseBuilding* exception)
{
    noBase* no = GetNode(pt).obj;
    if(!no || no == exception)
        return;

    // Destroy only flags, buildings and building sites
    const NodalObjectType noType = no->GetType();
    if(noType != NodalObjectType::Flag && noType != NodalObjectType::Building
       && noType != NodalObjectType::Buildingsite)
        return;

    // is the building on a node with a different owner?
    if(static_cast<noRoadNode*>(no)->GetPlayer() + 1 == newOwner)
        return;

    // Do not destroy military buildings that hold territory on their own
    // Normally they will be on players territory but it can happen that they don't
    // Examples: Improved alliances or expedition building sites
    const noBase* noCheckMil = (noType == NodalObjectType::Flag) ? GetNO(GetNeighbour(pt, Direction::NorthWest)) : no;
    const GO_Type goType = noCheckMil->GetGOT();
    if(goType == GO_Type::NobHq || goType == GO_Type::NobHarborbuilding
       || (goType == GO_Type::NobMilitary && !static_cast<const nobMilitary*>(noCheckMil)->IsNewBuilt())
       || (noCheckMil->GetType() == NodalObjectType::Buildingsite
           && static_cast<const noBuildingSite*>(noCheckMil)->IsHarborBuildingSiteFromSea()))
    {
        // LOG.write(("DestroyPlayerRests of hq, military, harbor or colony-harbor in construction stopped at x, %i y,
        // %i type, %i \n", x, y, noType);
        return;
    }

    // If it is a flag, destroy the building
    if(noType == NodalObjectType::Flag && (!exception || no != exception->GetFlag()))
        static_cast<noFlag*>(no)->DestroyAttachedBuilding();

    DestroyNO(pt, false);
}

void GameWorld::RoadNodeAvailable(const MapPoint pt)
{
    // Figuren direkt daneben
    for(const MapPoint nb : GetNeighbours(pt))
    {
        // Nochmal prüfen, ob er nun wirklich verfügbar ist (evtl blocken noch mehr usw.)
        if(!IsRoadNodeForFigures(pt))
            continue;

        // Figuren Bescheid sagen
        for(noBase& object : GetFigures(nb))
        {
            if(object.GetType() == NodalObjectType::Figure)
                static_cast<noFigure&>(object).NodeFreed(pt);
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

void GameWorld::Attack(const unsigned char player_attacker, const MapPoint pt, const unsigned short soldiers_count,
                       const bool strong_soldiers)
{
    auto* attacked_building = GetSpecObj<nobBaseMilitary>(pt);
    if(!attacked_building || !attacked_building->IsAttackable(player_attacker))
        return;

    // Militärgebäude in der Nähe finden
    sortedMilitaryBlds buildings = LookForMilitaryBuildings(pt, 3);

    // Liste von verfügbaren Soldaten, geordnet einfügen, damit man dann starke oder schwache Soldaten nehmen kann
    std::list<PotentialAttacker> potentialAttackers;

    for(const auto* building : buildings)
    {
        // Muss ein Gebäude von uns sein und darf nur ein "normales Militärgebäude" sein (kein HQ etc.)
        if(building->GetPlayer() != player_attacker || !BuildingProperties::IsMilitary(building->GetBuildingType()))
            continue;

        const auto& milBld = static_cast<const nobMilitary&>(*building);
        unsigned numSoldiersForCurrentBld = milBld.GetNumSoldiersForAttack(pt);
        if(!numSoldiersForCurrentBld)
            continue;

        // Take soldier(s)
        unsigned curNumSoldiers = 0;
        const unsigned distance = CalcDistance(building->GetPos(), pt);
        if(strong_soldiers)
        {
            // Strong soldiers first
            for(auto& curSoldier : helpers::reverse(milBld.GetTroops()))
            {
                if(curNumSoldiers >= numSoldiersForCurrentBld)
                    break;
                ++curNumSoldiers;
                /* Insert new soldier before current one if:
                        new soldiers rank is greater
                        OR new soldiers rank is equal AND new soldiers distance is smaller */
                const auto itInsertPos = helpers::find_if(
                  potentialAttackers, [curRank = curSoldier.GetRank(), distance](const PotentialAttacker& pa) {
                      return pa.soldier->GetRank() < curRank
                             || (pa.soldier->GetRank() == curRank && pa.distance > distance);
                  });
                potentialAttackers.emplace(itInsertPos, PotentialAttacker{&curSoldier, distance});
            }
        } else
        {
            // Weak soldiers first
            for(auto& curSoldier : milBld.GetTroops())
            {
                if(curNumSoldiers >= numSoldiersForCurrentBld)
                    break;
                ++curNumSoldiers;
                /* Insert new soldier before current one if:
                           new soldiers rank is less
                           OR new soldiers rank is equal AND new soldiers distance is smaller */
                const auto itInsertPos = helpers::find_if(
                  potentialAttackers, [curRank = curSoldier.GetRank(), distance](const PotentialAttacker& pa) {
                      return pa.soldier->GetRank() > curRank
                             || (pa.soldier->GetRank() == curRank && pa.distance > distance);
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
        pa.soldier->getHome()->SendAttacker(pa.soldier, *attacked_building);
        curNumSoldiers++;
    }
}

/// Compare sea attackers by their rank, then by their distance
template<class T_RankCmp>
struct CmpSeaAttacker : private T_RankCmp
{
    bool operator()(const GameWorldBase::PotentialSeaAttacker& lhs, const GameWorldBase::PotentialSeaAttacker& rhs)
    {
        // Sort after rank, then distance then objId
        if(lhs.soldier->GetRank() == rhs.soldier->GetRank())
        {
            if(lhs.distance == rhs.distance)
                return (lhs.soldier->GetObjId() < rhs.soldier->GetObjId()); // tie breaker
            else
                return lhs.distance < rhs.distance;
        } else
            return T_RankCmp::operator()(lhs.soldier->GetRank(), rhs.soldier->GetRank());
    }
};

void GameWorld::AttackViaSea(const unsigned char player_attacker, const MapPoint pt,
                             const unsigned short soldiers_count, const bool strong_soldiers)
{
    // Verfügbare Soldaten herausfinden
    std::vector<GameWorldBase::PotentialSeaAttacker> attackers = GetSoldiersForSeaAttack(player_attacker, pt);
    if(attackers.empty())
        return;

    // Sort them
    if(strong_soldiers)
        std::sort(attackers.begin(), attackers.end(), CmpSeaAttacker<std::greater<>>());
    else
        std::sort(attackers.begin(), attackers.end(), CmpSeaAttacker<std::less<>>());

    auto& attacked_building = *GetSpecObj<nobBaseMilitary>(pt);
    unsigned counter = 0;
    for(GameWorldBase::PotentialSeaAttacker& pa : attackers)
    {
        if(counter >= soldiers_count)
            break;
        pa.soldier->getHome()->SendAttacker(pa.soldier, attacked_building, pa.harbor);
        counter++;
    }
}

TradePathCache& GameWorld::GetTradePathCache()
{
    RTTR_Assert(tradePathCache);
    return *tradePathCache;
}

void GameWorld::setEconHandler(std::unique_ptr<EconomyModeHandler> handler)
{
    RTTR_Assert_Msg(!econHandler, "Can't reset the economy mode handler ATM");
    econHandler = std::move(handler);
}

bool GameWorld::IsRoadNodeForFigures(const MapPoint pt)
{
    // Figuren durchgehen, bei Kämpfen und wartenden Angreifern sowie anderen wartenden Figuren stoppen!
    for(const noBase& object : GetFigures(pt))
    {
        // andere wartende Figuren
        /*
                ATTENTION! This leads to figures on the same node blocking each other. -> Ghost jams

                if((*it)->GetType() == NodalObjectType::Figure)
                {
                    noFigure * fig = static_cast<noFigure*>(*it);
                    // Figuren dürfen sich nicht gegenüber stehen, sonst warten sie ja ewig aufeinander
                    // Außerdem muss auch die Position stimmen, sonst spinnt der ggf. rum, da
                    if(fig->IsWaitingForFreeNode() && (fig->GetDir()+3)%6 != dir)
                        return false;
                }*/

        // Kampf
        if(object.GetGOT() == GO_Type::Fighting)
        {
            if(static_cast<const noFighting&>(object).IsActive())
                return false;
        } else if(object.GetGOT() == GO_Type::NofAttacker) // wartende Angreifer
        {
            if(static_cast<const nofAttacker&>(object).IsBlockingRoads())
                return false;
        }
    }

    // alles ok
    return true;
}

/// Lässt alle Figuren, die auf diesen Punkt  auf Wegen zulaufen, anhalten auf dem Weg (wegen einem Kampf)
void GameWorld::StopOnRoads(const MapPoint pt, const helpers::OptionalEnum<Direction> dir)
{
    // Figuren drumherum sammeln (auch von dem Punkt hier aus)
    std::vector<noFigure*> figures;

    // Auch vom Ausgangspunkt aus, da sie im GameWorld wegem Zeichnen auch hier hängen können!
    for(auto& fieldFigure : GetFigures(pt))
    {
        if(fieldFigure.GetType() == NodalObjectType::Figure)
            figures.push_back(static_cast<noFigure*>(&fieldFigure));
    }

    // Und natürlich in unmittelbarer Umgebung suchen
    for(const MapPoint nb : GetNeighbours(pt))
    {
        for(auto& fieldFigure : GetFigures(nb))
        {
            if(fieldFigure.GetType() == NodalObjectType::Figure)
                figures.push_back(static_cast<noFigure*>(&fieldFigure));
        }
    }

    for(auto& figure : figures)
    {
        if(dir && *dir + 3u == static_cast<noFigure*>(figure)->GetCurMoveDir())
        {
            if(GetNeighbour(pt, *dir) == static_cast<noFigure*>(figure)->GetPos())
                continue;
        }

        // Derjenige muss ggf. stoppen, wenn alles auf ihn zutrifft
        static_cast<noFigure*>(figure)->StopIfNecessary(pt);
    }
}

void GameWorld::Armageddon()
{
    RTTR_FOREACH_PT(MapPoint, GetSize())
    {
        auto* flag = GetSpecObj<noFlag>(pt);
        if(flag)
        {
            flag->DestroyAttachedBuilding();
            DestroyNO(pt, false);
        }
    }
}

void GameWorld::Armageddon(const unsigned char player)
{
    RTTR_FOREACH_PT(MapPoint, GetSize())
    {
        auto* flag = GetSpecObj<noFlag>(pt);
        if(flag && flag->GetPlayer() == player)
        {
            flag->DestroyAttachedBuilding();
            DestroyNO(pt, false);
        }
    }
}

bool GameWorld::ValidWaitingAroundBuildingPoint(const MapPoint pt, const MapPoint center)
{
    // Gültiger Punkt für Figuren?
    if(!PathConditionHuman(*this).IsNodeOk(pt))
        return false;

    // Objekte, die sich hier befinden durchgehen
    for(const auto& figure : GetFigures(pt))
    {
        // Ist hier ein anderer Soldat, der hier ebenfalls wartet?
        if(figure.GetGOT() == GO_Type::NofAttacker || figure.GetGOT() == GO_Type::NofAggressivedefender
           || figure.GetGOT() == GO_Type::NofDefender)
        {
            const auto state = static_cast<const nofActiveSoldier&>(figure).GetState();
            if(state == nofActiveSoldier::SoldierState::WaitingForFight
               || state == nofActiveSoldier::SoldierState::AttackingWaitingAroundBuilding)
                return false;
        }

        // Oder ein Kampf, der hier tobt?
        if(figure.GetGOT() == GO_Type::Fighting)
            return false;
    }
    // object wall or impassable terrain increasing my path to target length to a higher value than the direct distance?
    return FindHumanPath(pt, center, CalcDistance(pt, center)) != boost::none;
}

bool GameWorld::IsValidPointForFighting(MapPoint pt, const nofActiveSoldier& soldier,
                                        bool avoid_military_building_flags)
{
    // Is this a flag of a military building?
    if(avoid_military_building_flags && GetNO(pt)->GetGOT() == GO_Type::Flag)
    {
        GO_Type got = GetNO(GetNeighbour(pt, Direction::NorthWest))->GetGOT();
        if(got == GO_Type::NobMilitary || got == GO_Type::NobHarborbuilding || got == GO_Type::NobHq)
            return false;
    }

    // Objekte, die sich hier befinden durchgehen
    for(const auto& figure : GetFigures(pt))
    {
        // Ist hier ein anderer Soldat, der hier ebenfalls wartet ?
        const GO_Type got = figure.GetGOT();
        if(got == GO_Type::NofAttacker || got == GO_Type::NofAggressivedefender || got == GO_Type::NofDefender)
        {
            if(static_cast<const nofActiveSoldier*>(&figure) == &soldier)
                continue;
            switch(static_cast<const nofActiveSoldier&>(figure).GetState())
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
            RTTR_Assert(!fight.IsFighter(soldier));
            if(fight.IsActive())
                return false;
        }
    }
    // Liegt hier was rum auf dem man nicht kämpfen sollte?
    const BlockingManner bm = GetNO(pt)->GetBM();
    return bm == BlockingManner::None || bm == BlockingManner::Tree || bm == BlockingManner::Flag;
}

bool GameWorld::IsPointCompletelyVisible(const MapPoint& pt, unsigned char player,
                                         const noBaseBuilding* exception) const
{
    sortedMilitaryBlds buildings = LookForMilitaryBuildings(pt, 3);

    // Sichtbereich von Militärgebäuden
    for(const nobBaseMilitary* milBld : buildings)
    {
        if(milBld->GetPlayer() == player && milBld != exception)
        {
            // Prüfen, obs auch unbesetzt ist
            if(milBld->GetGOT() == GO_Type::NobMilitary)
            {
                if(static_cast<const nobMilitary*>(milBld)->IsNewBuilt())
                    continue;
            }

            if(CalcDistance(pt, milBld->GetPos()) <= unsigned(milBld->GetMilitaryRadius() + VISUALRANGE_MILITARY))
                return true;
        }
    }

    // Sichtbereich von Hafenbaustellen
    for(const noBuildingSite* bldSite : harbor_building_sites_from_sea)
    {
        if(bldSite->GetPlayer() == player && bldSite != exception)
        {
            if(CalcDistance(pt, bldSite->GetPos()) <= unsigned(HARBOR_RADIUS + VISUALRANGE_MILITARY))
                return true;
        }
    }

    // Sichtbereich von Spähtürmen
    for(const nobUsual* bld : GetPlayer(player).GetBuildingRegister().GetBuildings(BuildingType::LookoutTower)) //-V807
    {
        // Ist Späturm überhaupt besetzt?
        if(!bld->HasWorker())
            continue;

        // Nicht die Ausnahme wählen
        if(bld == exception)
            continue;

        // Liegt Spähturm innerhalb des Sichtradius?
        if(CalcDistance(pt, bld->GetPos()) <= VISUALRANGE_LOOKOUTTOWER)
            return true;
    }

    // Check scouts and soldiers
    const unsigned range = std::max(VISUALRANGE_SCOUT, VISUALRANGE_SOLDIER);
    if(CheckPointsInRadius(
         pt, range,
         [this, player](auto pt, auto distance) { return this->IsScoutingFigureOnNode(pt, player, distance); }, true))
    {
        return true;
    }
    return IsPointScoutedByShip(pt, player);
}

bool GameWorld::IsScoutingFigureOnNode(const MapPoint& pt, unsigned player, unsigned distance) const
{
    static_assert(VISUALRANGE_SCOUT >= VISUALRANGE_SOLDIER, "Visual range changed. Check loop below!");

    // Späher/Soldaten in der Nähe prüfen und direkt auf dem Punkt
    for(const noBase& obj : GetFigures(pt))
    {
        const GO_Type got = obj.GetGOT();
        // Check for scout. Note: no need to check for distance as scouts have higher distance than soldiers
        if(got == GO_Type::NofScoutFree)
        {
            // Prüfen, ob er auch am Erkunden ist und an der Position genau und ob es vom richtigen Spieler ist
            if(static_cast<const nofScout_Free&>(obj).GetPlayer() == player)
                return true;
            else
                continue;
        } else if(distance <= VISUALRANGE_SOLDIER)
        {
            // Soldaten?
            if(got == GO_Type::NofAttacker || got == GO_Type::NofAggressivedefender)
            {
                if(static_cast<const nofActiveSoldier&>(obj).GetPlayer() == player)
                    return true;
            }
            // Kämpfe (wo auch Soldaten drin sind)
            else if(got == GO_Type::Fighting)
            {
                // Prüfen, ob da ein Soldat vom angegebenen Spieler dabei ist
                if(static_cast<const noFighting&>(obj).IsSoldierOfPlayer(player))
                    return true;
            }
        }
    }

    return false;
}

bool GameWorld::IsPointScoutedByShip(const MapPoint& pt, unsigned player) const
{
    const std::vector<noShip*>& ships = GetPlayer(player).GetShips();
    for(const noShip* ship : ships)
    {
        unsigned shipDistance = CalcDistance(pt, ship->GetPos());
        if(shipDistance <= ship->GetVisualRange())
            return true;
    }
    return false;
}

void GameWorld::RecalcVisibility(const MapPoint pt, const unsigned char player, const noBaseBuilding* const exception)
{
    /// Zustand davor merken
    Visibility visibility_before = GetNode(pt).fow[player].visibility;

    /// Herausfinden, ob vollständig sichtbar
    bool visible = IsPointCompletelyVisible(pt, player, exception);

    // Vollständig sichtbar --> vollständig sichtbar logischerweise
    if(visible)
        MakeVisible(pt, player);
    else
    {
        // nicht mehr sichtbar
        // Je nach vorherigen Zustand und Einstellung entscheiden
        switch(GetGGS().exploration)
        {
            case Exploration::Disabled:
            case Exploration::Classic:
                // einmal sichtbare Bereiche bleiben erhalten
                // nichts zu tun
                break;
            case Exploration::FogOfWar:
            case Exploration::FogOfWarExplored:
                // wenn es mal sichtbar war, nun im Nebel des Krieges
                if(visibility_before == Visibility::Visible)
                {
                    SetVisibility(pt, player, Visibility::FogOfWar, GetEvMgr().GetCurrentGF());
                }
                break;
            default: throw std::logic_error("Invalid exploration value");
        }
    }
}

void GameWorld::MakeVisible(const MapPoint pt, const unsigned char player)
{
    SetVisibility(pt, player, Visibility::Visible);
}

void GameWorld::RecalcVisibilitiesAroundPoint(const MapPoint pt, const MapCoord radius, const unsigned char player,
                                              const noBaseBuilding* const exception)
{
    std::vector<MapPoint> pts = GetPointsInRadiusWithCenter(pt, radius);
    for(const MapPoint& pt : pts)
        RecalcVisibility(pt, player, exception);
}

/// Setzt die Sichtbarkeiten um einen Punkt auf sichtbar (aus Performancegründen Alternative zu oberem)
void GameWorld::MakeVisibleAroundPoint(const MapPoint pt, const MapCoord radius, const unsigned char player)
{
    std::vector<MapPoint> pts = GetPointsInRadiusWithCenter(pt, radius);
    for(const MapPoint& curPt : pts)
        MakeVisible(curPt, player);
}

/// Bestimmt bei der Bewegung eines spähenden Objekts die Sichtbarkeiten an
/// den Rändern neu
void GameWorld::RecalcMovingVisibilities(const MapPoint pt, const unsigned char player, const MapCoord radius,
                                         const Direction moving_dir, MapPoint* enemy_territory)
{
    // Neue Sichtbarkeiten zuerst setzen
    // Zum Eckpunkt der beiden neuen sichtbaren Kanten gehen
    MapPoint t(pt);
    for(MapCoord i = 0; i < radius; ++i)
        t = GetNeighbour(t, moving_dir);

    // Und zu beiden Abzweigungen weiter gehen und Punkte auf visible setzen
    MakeVisible(t, player);
    MapPoint tt(t);
    Direction dir = moving_dir + 2u;
    for(MapCoord i = 0; i < radius; ++i)
    {
        tt = GetNeighbour(tt, dir);
        // Sichtbarkeit und für FOW-Gebiet vorherigen Besitzer merken
        // (d.h. der dort  zuletzt war, als es für Spieler player sichtbar war)
        Visibility old_vis = CalcVisiblityWithAllies(tt, player);
        unsigned char old_owner = GetNode(tt).fow[player].owner;
        MakeVisible(tt, player);
        // Neues feindliches Gebiet entdeckt?
        // Muss vorher undaufgedeckt oder FOW gewesen sein, aber in dem Fall darf dort vorher noch kein
        // Territorium entdeckt worden sein
        unsigned char current_owner = GetNode(tt).owner;
        if(current_owner
           && (old_vis == Visibility::Invisible || (old_vis == Visibility::FogOfWar && old_owner != current_owner)))
        {
            if(GetPlayer(player).IsAttackable(current_owner - 1) && enemy_territory)
            {
                *enemy_territory = tt;
            }
        }
    }

    tt = t;
    dir = moving_dir - 2u;
    for(MapCoord i = 0; i < radius; ++i)
    {
        tt = GetNeighbour(tt, dir);
        // Sichtbarkeit und für FOW-Gebiet vorherigen Besitzer merken
        // (d.h. der dort  zuletzt war, als es für Spieler player sichtbar war)
        Visibility old_vis = CalcVisiblityWithAllies(tt, player);
        unsigned char old_owner = GetNode(tt).fow[player].owner;
        MakeVisible(tt, player);
        // Neues feindliches Gebiet entdeckt?
        // Muss vorher undaufgedeckt oder FOW gewesen sein, aber in dem Fall darf dort vorher noch kein
        // Territorium entdeckt worden sein
        unsigned char current_owner = GetNode(tt).owner;
        if(current_owner
           && (old_vis == Visibility::Invisible || (old_vis == Visibility::FogOfWar && old_owner != current_owner)))
        {
            if(GetPlayer(player).IsAttackable(current_owner - 1) && enemy_territory)
            {
                *enemy_territory = tt;
            }
        }
    }

    // Dasselbe für die zurückgebliebenen Punkte
    // Diese müssen allerdings neu berechnet werden!
    t = pt;
    Direction anti_moving_dir = moving_dir + 3u;
    for(MapCoord i = 0; i < radius + 1; ++i)
        t = GetNeighbour(t, anti_moving_dir);

    RecalcVisibility(t, player, nullptr);
    tt = t;
    dir = anti_moving_dir + 2u;
    for(MapCoord i = 0; i < radius; ++i)
    {
        tt = GetNeighbour(tt, dir);
        RecalcVisibility(tt, player, nullptr);
    }

    tt = t;
    dir = anti_moving_dir - 2u;
    for(unsigned i = 0; i < radius; ++i)
    {
        tt = GetNeighbour(tt, dir);
        RecalcVisibility(tt, player, nullptr);
    }
}

bool GameWorld::IsBorderNode(const MapPoint pt, const unsigned char owner) const
{
    return (GetNode(pt).owner == owner && !IsPlayerTerritory(pt, owner));
}

/**
 *  Konvertiert Ressourcen zwischen Typen hin und her oder löscht sie.
 *  Für Spiele ohne Gold.
 */
void GameWorld::ConvertMineResourceTypes(ResourceType from, ResourceType to)
{
    // LOG.write(("Convert map resources from %i to %i\n", from, to);
    if(from == to)
        return;

    RTTR_FOREACH_PT(MapPoint, GetSize())
    {
        Resource resources = GetNode(pt).resources;
        // Gibt es Ressourcen dieses Typs?
        // Wenn ja, dann umwandeln bzw löschen
        if(resources.getType() == from)
        {
            resources.setType(to);
            SetResource(pt, resources);
        }
    }
}

void GameWorld::SetupResources()
{
    ResourceType target;
    switch(GetGGS().getSelection(AddonId::CHANGE_GOLD_DEPOSITS))
    {
        case 0:
        default: target = ResourceType::Gold; break;
        case 1: target = ResourceType::Nothing; break;
        case 2: target = ResourceType::Iron; break;
        case 3: target = ResourceType::Coal; break;
        case 4: target = ResourceType::Granite; break;
    }
    ConvertMineResourceTypes(ResourceType::Gold, target);
    PlaceAndFixWater();
}

/**
 * Fills water depending on terrain and Addon setting
 */
void GameWorld::PlaceAndFixWater()
{
    bool waterEverywhere = GetGGS().getSelection(AddonId::EXHAUSTIBLE_WATER) == 1;

    RTTR_FOREACH_PT(MapPoint, GetSize())
    {
        Resource curNodeResource = GetNode(pt).resources;

        if(curNodeResource.getType() == ResourceType::Nothing)
        {
            if(!waterEverywhere)
                continue;
        } else if(curNodeResource.getType() != ResourceType::Water)
        {
            // do not override maps resource.
            continue;
        }

        uint8_t minHumidity = 100;
        for(const DescIdx<TerrainDesc> tIdx : GetTerrainsAround(pt))
        {
            const uint8_t curHumidity = GetDescription().get(tIdx).humidity;
            if(curHumidity < minHumidity)
            {
                minHumidity = curHumidity;
                if(minHumidity == 0)
                    break;
            }
        }
        if(minHumidity)
            curNodeResource = Resource(
              ResourceType::Water, waterEverywhere ? 7 : static_cast<uint8_t>(std::lround(minHumidity * 7. / 100.)));
        else
            curNodeResource = Resource(ResourceType::Nothing, 0);

        SetResource(pt, curNodeResource);
    }
}

/// Gründet vom Schiff aus eine neue Kolonie
bool GameWorld::FoundColony(const unsigned harbor_point, const unsigned char player, const unsigned short seaId)
{
    // Ist es hier überhaupt noch möglich, eine Kolonie zu gründen?
    if(!IsHarborAtSea(harbor_point, seaId) || !IsHarborPointFree(harbor_point, player))
        return false;

    MapPoint pos(GetHarborPoint(harbor_point));
    DestroyNO(pos, false);

    // Hafenbaustelle errichten
    auto* bs = new noBuildingSite(pos, player);
    SetNO(pos, bs);
    AddHarborBuildingSiteFromSea(bs);

    if(gi)
        gi->GI_UpdateMinimap(pos);

    RecalcTerritory(*bs, TerritoryChangeReason::Build);
    // BQ neu berechnen (evtl durch RecalcTerritory noch nicht geschehen)
    RecalcBQAroundPointBig(pos);

    GetNotifications().publish(ExpeditionNote(ExpeditionNote::ColonyFounded, player, pos));

    return true;
}

void GameWorld::RemoveHarborBuildingSiteFromSea(noBuildingSite* building_site)
{
    RTTR_Assert(building_site->GetBuildingType() == BuildingType::HarborBuilding);
    harbor_building_sites_from_sea.remove(building_site);
}

bool GameWorld::IsHarborBuildingSiteFromSea(const noBuildingSite* building_site) const
{
    return helpers::contains(harbor_building_sites_from_sea, building_site);
}

std::vector<unsigned> GameWorld::GetUnexploredHarborPoints(const unsigned hbIdToSkip, const unsigned seaId,
                                                           unsigned playerId) const
{
    std::vector<unsigned> hps;
    for(unsigned i = 1; i <= GetNumHarborPoints(); ++i)
    {
        if(i == hbIdToSkip || !IsHarborAtSea(i, seaId))
            continue;
        if(CalcVisiblityWithAllies(GetHarborPoint(i), playerId) != Visibility::Visible)
            hps.push_back(i);
    }
    return hps;
}

MapNode& GameWorld::GetNodeWriteable(const MapPoint pt)
{
    return GetNodeInt(pt);
}

void GameWorld::VisibilityChanged(const MapPoint pt, unsigned player, Visibility oldVis, Visibility newVis)
{
    GameWorldBase::VisibilityChanged(pt, player, oldVis, newVis);
    if(oldVis == Visibility::Invisible && newVis == Visibility::Visible && HasLua())
        GetLua().EventExplored(player, pt, GetNode(pt).owner);
    // Minimap Bescheid sagen
    if(gi)
        gi->GI_UpdateMinimap(pt);
}
