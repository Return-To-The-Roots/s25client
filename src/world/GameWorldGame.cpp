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
#include "world/GameWorldGame.h"
#include "EventManager.h"
#include "GameInterface.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "TradePathCache.h"
#include "addons/const_addons.h"
#include "buildings/noBuildingSite.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobUsual.h"
#include "figures/nofAttacker.h"
#include "figures/nofPassiveSoldier.h"
#include "figures/nofScout_Free.h"
#include "helpers/containerUtils.h"
#include "lua/LuaInterfaceGame.h"
#include "network/GameClient.h"
#include "notifications/BuildingNote.h"
#include "notifications/ExpeditionNote.h"
#include "notifications/RoadNote.h"
#include "pathfinding/PathConditionHuman.h"
#include "pathfinding/PathConditionRoad.h"
#include "postSystem/PostMsgWithBuilding.h"
#include "world/MapGeometry.h"
#include "world/TerritoryRegion.h"
#include "nodeObjs/noEnvObject.h"
#include "nodeObjs/noFighting.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noShip.h"
#include "gameData/BuildingConsts.h"
#include "gameData/BuildingProperties.h"
#include "gameData/GameConsts.h"
#include "gameData/MilitaryConsts.h"
#include "gameData/SettingTypeConv.h"
#include "gameData/TerrainData.h"
#include <boost/foreach.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/if.hpp>
#include <boost/lambda/lambda.hpp>
#include <algorithm>
#include <functional>
#include <stdexcept>

inline std::vector<GamePlayer> CreatePlayers(const std::vector<PlayerInfo>& playerInfos, GameWorldGame& gwg)
{
    std::vector<GamePlayer> players;
    players.reserve(playerInfos.size());
    for(unsigned i = 0; i < playerInfos.size(); ++i)
        players.push_back(GamePlayer(i, playerInfos[i], gwg));
    return players;
}

GameWorldGame::GameWorldGame(const std::vector<PlayerInfo>& players, const GlobalGameSettings& gameSettings, EventManager& em)
    : GameWorldBase(CreatePlayers(players, *this), gameSettings, em)
{
    TradePathCache::inst().Clear();
    GameObject::SetPointers(this);
}

GameWorldGame::~GameWorldGame()
{
    GameObject::SetPointers(NULL);
}

MilitarySquares& GameWorldGame::GetMilitarySquares()
{
    return militarySquares;
}

void GameWorldGame::SetFlag(const MapPoint pt, const unsigned char player, const unsigned char dis_dir)
{
    if(GetBQ(pt, player) == BQ_NOTHING)
        return;
    // There must be no other flag around that point
    if(IsFlagAround(pt))
        return;

    // Gucken, nicht, dass schon eine Flagge dasteht
    if(GetNO(pt)->GetType() != NOP_FLAG)
    {
        DestroyNO(pt, false);
        SetNO(pt, new noFlag(pt, player, dis_dir));

        RecalcBQAroundPointBig(pt);
    }
}

void GameWorldGame::DestroyFlag(const MapPoint pt, unsigned char playerId)
{
    // Let's see if there is a flag
    if(GetNO(pt)->GetType() == NOP_FLAG)
    {
        noFlag* flag = GetSpecObj<noFlag>(pt);
        if(flag->GetPlayer() != playerId)
            return;

        // Get the attached building if existing
        noBase* building = GetNO(GetNeighbour(pt, Direction::NORTHWEST));

        // Is this a military building?
        if(building->GetGOT() == GOT_NOB_MILITARY)
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

void GameWorldGame::SetPointRoad(MapPoint pt, Direction dir, unsigned char type)
{
    if(dir.toUInt() >= 3)
        dir = dir - 3u;
    else
        pt = GetNeighbour(pt, dir);

    SetRoad(pt, dir.toUInt(), type);

    if(gi)
        gi->GI_UpdateMinimap(pt);
}

void GameWorldGame::SetBuildingSite(const BuildingType type, const MapPoint pt, const unsigned char player)
{
    if(!GetPlayer(player).IsBuildingEnabled(type))
        return;

    // Gucken, ob das Gebäude hier überhaupt noch gebaut wrden kann
    if(!canUseBq(GetBQ(pt, player), BUILDING_SIZE[type]))
        return;

    // Wenn das ein Militärgebäude ist und andere Militärgebäude bereits in der Nähe sind, darf dieses nicht gebaut werden
    if(BuildingProperties::IsMilitary(type))
    {
        if(IsMilitaryBuildingNearNode(pt, player))
            return;
    }

    // Prüfen ob Katapult und ob Katapult erlaubt ist
    if(type == BLD_CATAPULT && !GetPlayer(player).CanBuildCatapult())
        return;

    DestroyNO(pt, false);

    // Baustelle setzen
    SetNO(pt, new noBuildingSite(type, pt, player));
    if(gi)
        gi->GI_UpdateMinimap(pt);

    // Bauplätze drumrum neu berechnen
    RecalcBQAroundPointBig(pt);
}

void GameWorldGame::DestroyBuilding(const MapPoint pt, const unsigned char player)
{
    // Steht da auch ein Gebäude oder eine Baustelle, nicht dass wir aus Verzögerung Feuer abreißen wollen, das geht schief
    if(GetNO(pt)->GetType() == NOP_BUILDING || GetNO(pt)->GetType() == NOP_BUILDINGSITE)
    {
        noBaseBuilding* nbb = GetSpecObj<noBaseBuilding>(pt);

        // Ist das Gebäude auch von dem Spieler, der es abreißen will?
        if(nbb->GetPlayer() != player)
            return;

        // Militärgebäude?
        if(nbb->GetGOT() == GOT_NOB_MILITARY)
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

void GameWorldGame::BuildRoad(const unsigned char playerId, const bool boat_road, const MapPoint start, const std::vector<Direction>& route)
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
    if(GetNO(curPt)->GetGOT() == GOT_FLAG)
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
        if(GetBQ(curPt, playerId) == BQ_NOTHING || IsFlagAround(curPt))
        {
            GetNotifications().publish(RoadNote(RoadNote::ConstructionFailed, playerId, start, route));
            return;
        }
        // keine Flagge bisher aber spricht auch nix gegen ne neue Flagge -> Flagge aufstellen!
        SetFlag(curPt, playerId, (route[route.size() - 1] + 3u).toUInt());
    }

    // Evtl Zierobjekte abreißen (Anfangspunkt)
    if(HasRemovableObjForRoad(start))
        DestroyNO(start);

    MapPoint end(start);
    for(unsigned i = 0; i < route.size(); ++i)
    {
        SetPointRoad(end, route[i], boat_road ? (RoadSegment::RT_BOAT + 1) : (RoadSegment::RT_NORMAL + 1));
        RecalcBQForRoad(end);
        end = GetNeighbour(end, route[i]);

        // Evtl Zierobjekte abreißen
        if(HasRemovableObjForRoad(end))
            DestroyNO(end);
    }

    RoadSegment* rs =
      new RoadSegment(boat_road ? RoadSegment::RT_BOAT : RoadSegment::RT_NORMAL, GetSpecObj<noFlag>(start), GetSpecObj<noFlag>(end), route);

    GetSpecObj<noFlag>(start)->SetRoute(route.front(), rs);
    GetSpecObj<noFlag>(end)->SetRoute(route.back() + 3u, rs);

    // Der Wirtschaft mitteilen, dass eine neue Straße gebaut wurde, damit sie alles Nötige macht
    GetPlayer(playerId).NewRoadConnection(rs);
    GetNotifications().publish(RoadNote(RoadNote::Constructed, playerId, start, route));
}

bool GameWorldGame::HasRemovableObjForRoad(const MapPoint pt) const
{
    const noStaticObject* obj = GetSpecObj<noStaticObject>(pt);
    if(obj && obj->GetSize() == 0)
        return true;
    return false;
    /*if(GetNO(pt)->GetGOT() == GOT_ENVOBJECT)
    {
        const noEnvObject* no = GetSpecObj<noEnvObject>(pt);
        unsigned short type = no->GetItemID();
        switch(no->GetItemFile())
        {
            case 0xFFFF: // map_?_z.lst
                if(type == 505 || type == 506 || type == 507 || type == 508 || type == 510 || (type >= 542 && type <= 546) || type == 512
                   || type == 513 ||           // Kakteen
                   type == 536 || type == 541) // abgeerntete Getreidefelder
                    return true;
                break;
            case 0:
                // todo:
                break;
            case 1:
                if(type <= 12)
                    return true;
                // todo:
                break;
            case 2:
                // todo:
                break;
            case 3:
                // todo:
                break;
            case 4:
                // todo:
                break;
            case 5:
                // todo:
                break;
            // Charburner rests
            case 6: return true; break;
        }
    }

    return false;*/
}

// When defined the game tries to remove "blocks" of border stones that look ugly (TODO: Example?)
// DISABLED: This currently leads to bugs. If you enable/fix this, please add tests and document the conditions this tries to fix
//#define PREVENT_BORDER_STONE_BLOCKING

void GameWorldGame::RecalcBorderStones(Point<int> startPt, Point<int> endPt)
{
    Point<int> size = endPt - startPt;
    // Add a bit extra space as this influences also border stones around the region
    // But not so much we wrap completely around the map (+1 to round up, /2 to have extra space centered)
    Point<int> EXTRA_SPACE(std::min(3, (GetWidth() - size.x + 1) / 2), std::min(3, (GetHeight() - size.y + 1) / 2));
    startPt -= EXTRA_SPACE;
    endPt += EXTRA_SPACE;
    // We might still be 1 node to big, make sure we have don't exceed the mapsize
    size = endPt - startPt;
    if(size.x > GetWidth())
        endPt.x = startPt.x + GetWidth();
    if(size.y > GetHeight())
        endPt.y = startPt.y + GetHeight();

#ifdef PREVENT_BORDER_STONE_BLOCKING
    const int width = endPt.x - startPt.x;
    // Store how many neighbors a border stone has
    std::vector<uint8_t> neighbors(width * (endPt.y - startPt.y), 0);
#endif

    for(Point<int> pt(startPt); pt.y < endPt.y; ++pt.y)
    {
        for(pt.x = startPt.x; pt.x < endPt.x; ++pt.x)
        {
            // Make map point
            const MapPoint curMapPt = MakeMapPoint(pt);
            const unsigned char owner = GetNode(curMapPt).owner;
            BoundaryStones& boundaryStones = GetBoundaryStones(curMapPt);

            // Is this a border node?
            if(owner && IsBorderNode(curMapPt, owner))
            {
                boundaryStones[0] = owner;

                // Check which neighbors are also border nodes and place the half-way stones to them
                for(unsigned i = 0; i < 3; ++i)
                {
                    if(IsBorderNode(GetNeighbour(curMapPt, Direction::fromInt(3 + i)), owner))
                        boundaryStones[i + 1] = owner;
                    else
                        boundaryStones[i + 1] = 0;
                }

#ifdef PREVENT_BORDER_STONE_BLOCKING
                // Count number of border nodes with same owner
                Point<int> offset(pt - startPt);
                int idx = offset.y * width + offset.x;
                for(unsigned i = 0; i < 6; ++i)
                {
                    if(GetNeighbourNode(curMapPt, i).boundary_stones[0] == owner)
                        ++neighbors[idx];
                }
#endif
            } else
            {
                // Not a border node -> Delete all border stones
                std::fill(boundaryStones.begin(), boundaryStones.end(), 0);
            }
        }
    }

#ifdef PREVENT_BORDER_STONE_BLOCKING
    // Do a second pass and delete some stones with 3 or more neighbors to avoid blocks of stones
    for(Point<int> pt(startPt); pt.y < endPt.y; ++pt.y)
    {
        for(pt.x = startPt.x; pt.x < endPt.x; ++pt.x)
        {
            const MapPoint curMapPt = MakeMapPoint(pt);

            // Do we have a stone here?
            const unsigned char owner = GetNode(curMapPt).boundary_stones[0];
            if(!owner)
                continue;

            Point<int> offset(pt - startPt);
            int idx = offset.y * width + offset.x;
            if(neighbors[idx] < 3)
                continue;

            for(unsigned dir = 0; dir < 3 && neighbors[idx] > 2; ++dir)
            {
                // Do we have a border stone of the same owner on the node in that direction?
                BoundaryStones& nbBoundStones = GetBoundaryStones(GetNeighbour(curMapPt, dir + 3));

                if(nbBoundStones[0] != owner)
                    continue;

                Point<int> pa = ::GetNeighbour(pt, Direction(dir + 3));
                if(pa.x < startPt.x || pa.x >= endPt.x || pa.y < startPt.y || pa.y >= endPt.y)
                    continue;
                // If that one has to many stones too, we delete the connection stone
                Point<int> offset(pa - startPt);
                int idxNb = offset.y * width + offset.x;
                if(neighbors[idxNb] > 2)
                {
                    nbBoundStones[dir + 1] = 0;
                    --neighbors[idx];
                    --neighbors[idxNb];
                }
            }
        }
    }
#endif
}

void GameWorldGame::RecalcTerritory(const noBaseBuilding& building, TerritoryChangeReason reason)
{
    // Radius der noch draufaddiert wird auf den eigentlich ausreichenden Bereich, für das Eliminieren von
    // herausragenden Landesteilen und damit Grenzsteinen
    static const int ADD_RADIUS = 2;
    // Get the military radius this building affects. Bld is either a military building or a harbor building site
    RTTR_Assert((building.GetBuildingType() == BLD_HARBORBUILDING && dynamic_cast<const noBuildingSite*>(&building))
                || dynamic_cast<const nobBaseMilitary*>(&building));
    const unsigned militaryRadius = building.GetMilitaryRadius();
    RTTR_Assert(militaryRadius > 0u);

    TerritoryRegion region = CreateTerritoryRegion(building, militaryRadius + ADD_RADIUS, reason == TerritoryChangeReason::Destroyed);

    // Set to true, where owner has changed (initially all false)
    std::vector<bool> ownerChanged(region.size.x * region.size.y, false);

    std::vector<int> sizeChanges(GetNumPlayers());
    // Daten von der TR kopieren in die richtige Karte, dabei zus. Grenzen korrigieren und Objekte zerstören, falls
    // das Land davon jemanden anders nun gehört

    const unsigned char ownerOfTriggerBld = GetNode(building.GetPos()).owner;
    const unsigned char newOwnerOfTriggerBld = region.GetOwner(Point<int>(building.GetPos()));
    const bool noAlliedBorderPush = GetGGS().isEnabled(AddonId::NO_ALLIED_PUSH);

    for(Point<int> pt(region.startPt); pt.y < region.endPt.y; ++pt.y)
    {
        for(pt.x = region.startPt.x; pt.x < region.endPt.x; ++pt.x)
        {
            const MapPoint curMapPt = MakeMapPoint(pt);
            const unsigned char oldOwner = GetNode(curMapPt).owner;
            const unsigned char newOwner = region.GetOwner(pt);

            // If nothing changed, there is nothing to do (ownerChanged was already initialized)
            if(oldOwner == newOwner)
                continue;

            // Dann entsprechend neuen Besitzer setzen - bei improved alliances addon noch paar extra bedingungen prüfen
            if(noAlliedBorderPush)
            {
                // rule 1: only take territory from an ally if that ally loses a building - special case: headquarter can take territory
                const bool ownersAllied = oldOwner > 0 && newOwner > 0 && GetPlayer(oldOwner - 1).IsAlly(newOwner - 1);
                if(ownersAllied && (ownerOfTriggerBld != oldOwner || reason == TerritoryChangeReason::Build)
                   && building.GetBuildingType() != BLD_HEADQUARTERS)
                    continue;
                // rule 2: do not gain territory when you lose a building (captured or destroyed)
                if(ownerOfTriggerBld == newOwner && reason != TerritoryChangeReason::Build)
                    continue;
                // rule 3: do not lose territory when you gain a building (newBuilt or capture)
                if((ownerOfTriggerBld == oldOwner && oldOwner > 0 && reason == TerritoryChangeReason::Build)
                   || (newOwnerOfTriggerBld == oldOwner && reason == TerritoryChangeReason::Captured))
                    continue;
            }
            SetOwner(curMapPt, newOwner);
            ownerChanged[region.GetIdx(pt)] = true;
            if(newOwner != 0)
                sizeChanges[newOwner - 1]++;
            if(oldOwner != 0)
                sizeChanges[oldOwner - 1]--;

            // Event for map scripting
            if(newOwner != 0 && HasLua())
                GetLua().EventOccupied(newOwner - 1, curMapPt);
        }
    }

    for(unsigned i = 0; i < GetNumPlayers(); ++i)
    {
        GetPlayer(i).ChangeStatisticValue(NUM_STATSRY, sizeChanges[i]);

        // Negatives Wachstum per Post dem/der jeweiligen Landesherren/dame melden, nur bei neugebauten Gebäuden
        if(reason == TerritoryChangeReason::Build && sizeChanges[i] < 0)
        {
            GetPostMgr().SendMsg(
              i, new PostMsgWithBuilding(GetEvMgr().GetCurrentGF(), _("Lost land by this building"), PostCategory::Military, building));
            GetNotifications().publish(BuildingNote(BuildingNote::LostLand, i, building.GetPos(), building.GetBuildingType()));
        }
    }

    for(Point<int> pt(region.startPt); pt.y < region.endPt.y; ++pt.y)
    {
        for(pt.x = region.startPt.x; pt.x < region.endPt.x; ++pt.x)
        {
            MapPoint curMapPt = MakeMapPoint(pt);
            if(GetNode(curMapPt).owner != 0)
            {
                /// Grenzsteine, die alleine "rausragen" und nicht mit einem richtigen Territorium verbunden sind, raushauen
                bool isPlayerTerritoryNear = false;
                for(unsigned d = 0; d < 6; ++d)
                {
                    if(IsPlayerTerritory(GetNeighbour(curMapPt, Direction::fromInt(d))))
                    {
                        isPlayerTerritoryNear = true;
                        break;
                    }
                }

                // Wenn kein Land angrenzt, dann nicht nehmen
                if(!isPlayerTerritoryNear)
                    SetOwner(curMapPt, 0);
            }

            // Drumherum (da ja Grenzen mit einberechnet werden ins Gebiet, da darf trotzdem nichts stehen) alles vom Spieler zerstören
            // nicht das Militärgebäude oder dessen Flagge nochmal abreißen
            if(ownerChanged[region.GetIdx(pt)])
            {
                for(unsigned char i = 0; i < 6; ++i)
                {
                    MapPoint neighbourPt = GetNeighbour(curMapPt, Direction::fromInt(i));

                    DestroyPlayerRests(neighbourPt, GetNode(curMapPt).owner, &building, false);

                    // BQ neu berechnen
                    RecalcBQ(neighbourPt);
                    // ggf den noch darüber, falls es eine Flagge war (kann ja ein Gebäude entstehen)
                    if(GetNeighbourNode(neighbourPt, Direction::NORTHWEST).bq != BQ_NOTHING)
                        RecalcBQ(GetNeighbour(neighbourPt, Direction::NORTHWEST));
                }

                if(gi)
                    gi->GI_UpdateMinimap(curMapPt);
            }
        }
    }

    RecalcBorderStones(region.startPt, region.endPt);

    // Sichtbarkeiten berechnen

    // Wurde es zerstört, müssen die Sichtbarkeiten entsprechend neu berechnet werden, ansonsten reicht es auch
    // sie einfach auf sichtbar zu setzen
    const unsigned visualRadius = militaryRadius + VISUALRANGE_MILITARY;
    if(reason == TerritoryChangeReason::Destroyed)
        RecalcVisibilitiesAroundPoint(building.GetPos(), visualRadius, building.GetPlayer(), &building);
    else
        MakeVisibleAroundPoint(building.GetPos(), visualRadius, building.GetPlayer());
}

bool GameWorldGame::DoesDestructionChangeTerritory(const noBaseBuilding& building) const
{
    // Get the military radius this building affects. Bld is either a military building or a harbor building site
    RTTR_Assert((building.GetBuildingType() == BLD_HARBORBUILDING && dynamic_cast<const noBuildingSite*>(&building))
                || dynamic_cast<const nobBaseMilitary*>(&building));
    const unsigned militaryRadius = building.GetMilitaryRadius();
    RTTR_Assert(militaryRadius > 0u);

    TerritoryRegion region = CreateTerritoryRegion(building, militaryRadius, true);

    // schaun ob sich was ändern würd im berechneten gebiet
    for(Point<int> pt(region.startPt); pt.y < region.endPt.y; ++pt.y)
    {
        for(pt.x = region.startPt.x; pt.x < region.endPt.x; ++pt.x)
        {
            MapPoint curMapPt = MakeMapPoint(pt);
            if(GetNode(curMapPt).owner == region.GetOwner(pt))
                continue;
            // if gameobjective is 75% ai can ignore water/snow/lava/swamp terrain (because it wouldnt help win the game)
            if(GetGGS().objective == GO_CONQUER3_4)
            {
                // So we check if any terrain is usable and if it is -> Land is important
                TerrainType t1 = GetNode(curMapPt).t1, t2 = GetNode(curMapPt).t2;
                if(TerrainData::IsUseable(t1) && TerrainData::IsUseable(t2))
                    return true;
                // also check neighboring nodes since border will still count as player territory but not allow any buildings!
                for(unsigned dir = 0; dir < Direction::COUNT; dir++)
                {
                    t1 = GetNeighbourNode(curMapPt, Direction::fromInt(dir)).t1;
                    t2 = GetNeighbourNode(curMapPt, Direction::fromInt(dir)).t2;
                    if(TerrainData::IsUseable(t1) || TerrainData::IsUseable(t2))
                        return true;
                }
            }
        }
    }
    return false;
}

TerritoryRegion GameWorldGame::CreateTerritoryRegion(const noBaseBuilding& building, unsigned radius, bool ignoreRegionOfBld) const
{
    const MapPoint bldPos = building.GetPos();

    // Span at most half the map size (assert even sizes, given due to layout)
    RTTR_Assert(GetWidth() % 2 == 0);
    RTTR_Assert(GetHeight() % 2 == 0);
    Point<int> halfSize(GetSize() / 2u);
    Point<int> radius2D = elMin(Point<int>::all(radius), halfSize);

    // Koordinaten erzeugen für TerritoryRegion
    const Point<int> startPt = Point<int>(bldPos) - radius2D;
    // All points in the region are less than endPt. If we want to check the same number of points right of bld as left we need a +1
    // unless radius is already half the map size in which case we would check the first point twice -> clamp to size/2
    const Point<int> endPt = Point<int>(bldPos) + elMin(radius2D + Point<int>(1, 1), halfSize);
    TerritoryRegion region(startPt, endPt, *this);

    // Alle Gebäude ihr Terrain in der Nähe neu berechnen
    sortedMilitaryBlds buildings = LookForMilitaryBuildings(bldPos, 3);
    BOOST_FOREACH(const nobBaseMilitary* milBld, buildings)
    {
        if(!(ignoreRegionOfBld && milBld == &building))
            region.CalcTerritoryOfBuilding(*milBld);
    }

    // Baustellen von Häfen mit einschließen
    BOOST_FOREACH(const noBuildingSite* bldSite, harbor_building_sites_from_sea)
    {
        if(!(ignoreRegionOfBld && bldSite == &building))
            region.CalcTerritoryOfBuilding(*bldSite);
    }

    return region;
}

void GameWorldGame::DestroyPlayerRests(const MapPoint pt, const unsigned char newOwner, const noBaseBuilding* exception,
                                       bool allowDestructionOfMilBuildings)
{
    noBase* no = GetNO(pt);

    // Flaggen, Gebäude und Baustellen zerstören, aber keine übernommenen und nicht die Ausahme oder dessen Flagge!
    if((no->GetType() == NOP_FLAG || no->GetType() == NOP_BUILDING || no->GetType() == NOP_BUILDINGSITE) && exception != no)
    {
        // Wurde das Objekt auch nicht vom Gegner übernommen?
        if(static_cast<noRoadNode*>(no)->GetPlayer() + 1 != newOwner)
        {
            // maybe buildings that push territory should not be destroyed right now?- can happen with improved alliances addon or in rare
            // cases even without the addon so allow those buildings & their flag to survive.
            if(!allowDestructionOfMilBuildings)
            {
                const noBase* noCheckMil = (no->GetType() == NOP_FLAG) ? GetNO(GetNeighbour(pt, Direction::NORTHWEST)) : no;
                if(noCheckMil->GetGOT() == GOT_NOB_HQ || noCheckMil->GetGOT() == GOT_NOB_HARBORBUILDING
                   || (noCheckMil->GetGOT() == GOT_NOB_MILITARY && !static_cast<const nobMilitary*>(noCheckMil)->IsNewBuilt())
                   || (noCheckMil->GetType() == NOP_BUILDINGSITE
                       && static_cast<const noBuildingSite*>(noCheckMil)->IsHarborBuildingSiteFromSea()))
                {
                    // LOG.write(("DestroyPlayerRests of hq, military, harbor or colony-harbor in construction stopped at x, %i y, %i type,
                    // %i \n", x, y, no->GetType());
                    return;
                }
            }
            // vorher Bescheid sagen
            if(no->GetType() == NOP_FLAG && (!exception || no != exception->GetFlag()))
                static_cast<noFlag*>(no)->DestroyAttachedBuilding();

            DestroyNO(pt, false);
            return;
        }
    }

    // TODO: This might not be required. Roads are destroyed when their flags are destroyed

    // ggf. Weg kappen
    Direction dir(Direction::WEST);
    noFlag* flag = GetRoadFlag(pt, dir, 0xFF);
    if(flag)
    {
        // Die Ministraße von dem Militärgebäude nich abreißen!
        if(flag->GetRoute(dir)->GetLength() == 1)
        {
            if(flag->GetRoute(dir)->GetF2() == exception)
                return;
        }

        flag->DestroyRoad(dir);
    }
}

void GameWorldGame::RoadNodeAvailable(const MapPoint pt)
{
    // Figuren direkt daneben
    for(unsigned char i = 0; i < 6; ++i)
    {
        // Nochmal prüfen, ob er nun wirklich verfügbar ist (evtl blocken noch mehr usw.)
        if(!IsRoadNodeForFigures(pt))
            continue;

        // Koordinaten um den Punkt herum
        MapPoint nb = GetNeighbour(pt, Direction::fromInt(i));

        // Figuren Bescheid sagen
        BOOST_FOREACH(noBase* object, GetFigures(nb))
        {
            if(object->GetType() == NOP_FIGURE)
                static_cast<noFigure*>(object)->NodeFreed(pt);
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

void GameWorldGame::Attack(const unsigned char player_attacker, const MapPoint pt, const unsigned short soldiers_count,
                           const bool strong_soldiers)
{
    nobBaseMilitary* attacked_building = GetSpecObj<nobBaseMilitary>(pt);
    if(!attacked_building || !attacked_building->IsAttackable(player_attacker))
        return;

    // Militärgebäude in der Nähe finden
    sortedMilitaryBlds buildings = LookForMilitaryBuildings(pt, 3);

    // Liste von verfügbaren Soldaten, geordnet einfügen, damit man dann starke oder schwache Soldaten nehmen kann
    std::list<PotentialAttacker> soldiers;

    for(sortedMilitaryBlds::iterator it = buildings.begin(); it != buildings.end(); ++it)
    {
        // Muss ein Gebäude von uns sein und darf nur ein "normales Militärgebäude" sein (kein HQ etc.)
        if((*it)->GetPlayer() != player_attacker || !BuildingProperties::IsMilitary((*it)->GetBuildingType()))
            continue;

        unsigned soldiers_count = static_cast<nobMilitary*>(*it)->GetNumSoldiersForAttack(pt);
        if(!soldiers_count)
            continue;

        // Take soldier(s)
        unsigned i = 0;
        const SortedTroops& troops = static_cast<nobMilitary*>(*it)->GetTroops();
        const unsigned distance = CalcDistance((*it)->GetPos(), pt);
        if(strong_soldiers)
        {
            // Strong soldiers first
            for(SortedTroops::const_reverse_iterator it2 = troops.rbegin(); it2 != troops.rend() && i < soldiers_count; ++it2, ++i)
            {
                bool inserted = false;
                for(std::list<PotentialAttacker>::iterator it3 = soldiers.begin(); it3 != soldiers.end(); ++it3)
                {
                    /* Insert new soldier before current one if:
                            new soldiers rank is greater
                            OR new soldiers rank is equal AND new soldiers distance is smaller */
                    if(it3->soldier->GetRank() < (*it2)->GetRank()
                       || (it3->soldier->GetRank() == (*it2)->GetRank() && it3->distance > distance))
                    {
                        PotentialAttacker pa = {*it2, distance};
                        soldiers.insert(it3, pa);
                        inserted = true;
                        break;
                    }
                }
                if(!inserted)
                {
                    PotentialAttacker pa = {*it2, distance};
                    soldiers.push_back(pa);
                }
            }
        } else
        {
            // Weak soldiers first
            for(SortedTroops::const_iterator it2 = troops.begin(); it2 != troops.end() && i < soldiers_count; ++it2, ++i)
            {
                bool inserted = false;
                for(std::list<PotentialAttacker>::iterator it3 = soldiers.begin(); it3 != soldiers.end(); ++it3)
                {
                    /* Insert new soldier before current one if:
                            new soldiers rank is less
                            OR new soldiers rank is equal AND new soldiers distance is smaller */
                    if(it3->soldier->GetRank() > (*it2)->GetRank()
                       || (it3->soldier->GetRank() == (*it2)->GetRank() && it3->distance > distance))
                    {
                        PotentialAttacker pa = {*it2, distance};
                        soldiers.insert(it3, pa);
                        inserted = true;
                        break;
                    }
                }
                if(!inserted)
                {
                    PotentialAttacker pa = {*it2, distance};
                    soldiers.push_back(pa);
                }
            }
        } // End weak/strong check
    }

    // Send the soldiers to attack
    unsigned short i = 0;

    BOOST_FOREACH(PotentialAttacker& pa, soldiers)
    {
        if(i >= soldiers_count)
            break;
        // neuen Angreifer-Soldaten erzeugen
        new nofAttacker(pa.soldier, attacked_building);
        // passiven Soldaten entsorgen
        destroyAndDelete(pa.soldier);
        i++;
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

void GameWorldGame::AttackViaSea(const unsigned char player_attacker, const MapPoint pt, const unsigned short soldiers_count,
                                 const bool strong_soldiers)
{
    // Verfügbare Soldaten herausfinden
    std::vector<GameWorldBase::PotentialSeaAttacker> attackers = GetSoldiersForSeaAttack(player_attacker, pt);
    if(attackers.empty())
        return;

    // Sort them
    if(strong_soldiers)
        std::sort(attackers.begin(), attackers.end(), CmpSeaAttacker<std::greater<unsigned> >());
    else
        std::sort(attackers.begin(), attackers.end(), CmpSeaAttacker<std::less<unsigned> >());

    nobBaseMilitary* attacked_building = GetSpecObj<nobBaseMilitary>(pt);
    unsigned counter = 0;
    BOOST_FOREACH(GameWorldBase::PotentialSeaAttacker& pa, attackers)
    {
        if(counter >= soldiers_count)
            break;
        // neuen Angreifer-Soldaten erzeugen
        new nofAttacker(pa.soldier, attacked_building, pa.harbor);
        // passiven Soldaten entsorgen
        destroyAndDelete(pa.soldier);
        counter++;
    }
}

bool GameWorldGame::IsRoadNodeForFigures(const MapPoint pt)
{
    // Figuren durchgehen, bei Kämpfen und wartenden Angreifern sowie anderen wartenden Figuren stoppen!
    BOOST_FOREACH(noBase* object, GetFigures(pt))
    {
        // andere wartende Figuren
        /*
                ATTENTION! This leads to figures on the same node blocking each other. -> Ghost jams

                if((*it)->GetType() == NOP_FIGURE)
                {
                    noFigure * fig = static_cast<noFigure*>(*it);
                    // Figuren dürfen sich nicht gegenüber stehen, sonst warten sie ja ewig aufeinander
                    // Außerdem muss auch die Position stimmen, sonst spinnt der ggf. rum, da
                    if(fig->IsWaitingForFreeNode() && (fig->GetDir()+3)%6 != dir)
                        return false;
                }*/

        // Kampf
        if(object->GetGOT() == GOT_FIGHTING)
        {
            if(static_cast<noFighting*>(object)->IsActive())
                return false;
        }

        //// wartende Angreifer
        if(object->GetGOT() == GOT_NOF_ATTACKER)
        {
            if(static_cast<nofAttacker*>(object)->IsBlockingRoads())
                return false;
        }
    }

    // alles ok
    return true;
}

/// Lässt alle Figuren, die auf diesen Punkt  auf Wegen zulaufen, anhalten auf dem Weg (wegen einem Kampf)
void GameWorldGame::StopOnRoads(const MapPoint pt, const unsigned char dir)
{
    // Figuren drumherum sammeln (auch von dem Punkt hier aus)
    std::vector<noBase*> figures;

    // Auch vom Ausgangspunkt aus, da sie im GameWorldGame wegem Zeichnen auch hier hängen können!
    const std::list<noBase*>& fieldFigures = GetFigures(pt);
    for(std::list<noBase*>::const_iterator it = fieldFigures.begin(); it != fieldFigures.end(); ++it)
        if((*it)->GetType() == NOP_FIGURE)
            figures.push_back(*it);

    // Und natürlich in unmittelbarer Umgebung suchen
    for(unsigned d = 0; d < Direction::COUNT; ++d)
    {
        const std::list<noBase*>& fieldFigures = GetFigures(GetNeighbour(pt, Direction::fromInt(d)));
        for(std::list<noBase*>::const_iterator it = fieldFigures.begin(); it != fieldFigures.end(); ++it)
            if((*it)->GetType() == NOP_FIGURE)
                figures.push_back(*it);
    }

    for(std::vector<noBase*>::iterator it = figures.begin(); it != figures.end(); ++it)
    {
        if(dir < Direction::COUNT)
        {
            if(Direction(dir + 3) == static_cast<noFigure*>(*it)->GetCurMoveDir())
            {
                if(GetNeighbour(pt, Direction::fromInt(dir)) == static_cast<noFigure*>(*it)->GetPos())
                    continue;
            }
        }

        // Derjenige muss ggf. stoppen, wenn alles auf ihn zutrifft
        static_cast<noFigure*>(*it)->StopIfNecessary(pt);
    }
}

void GameWorldGame::Armageddon()
{
    RTTR_FOREACH_PT(MapPoint, GetSize())
    {
        noFlag* flag = GetSpecObj<noFlag>(pt);
        if(flag)
        {
            flag->DestroyAttachedBuilding();
            DestroyNO(pt, false);
        }
    }
}

void GameWorldGame::Armageddon(const unsigned char player)
{
    RTTR_FOREACH_PT(MapPoint, GetSize())
    {
        noFlag* flag = GetSpecObj<noFlag>(pt);
        if(flag && flag->GetPlayer() == player)
        {
            flag->DestroyAttachedBuilding();
            DestroyNO(pt, false);
        }
    }
}

bool GameWorldGame::ValidWaitingAroundBuildingPoint(const MapPoint pt, nofAttacker* /*attacker*/, const MapPoint center)
{
    // Gültiger Punkt für Figuren?
    if(!PathConditionHuman(*this).IsNodeOk(pt))
        return false;

    // Objekte, die sich hier befinden durchgehen
    const std::list<noBase*>& figures = GetFigures(pt);
    for(std::list<noBase*>::const_iterator it = figures.begin(); it != figures.end(); ++it)
    {
        // Ist hier ein anderer Soldat, der hier ebenfalls wartet?
        if((*it)->GetGOT() == GOT_NOF_ATTACKER || (*it)->GetGOT() == GOT_NOF_AGGRESSIVEDEFENDER || (*it)->GetGOT() == GOT_NOF_DEFENDER)
        {
            if(static_cast<nofActiveSoldier*>(*it)->GetState() == nofActiveSoldier::STATE_WAITINGFORFIGHT
               || static_cast<nofActiveSoldier*>(*it)->GetState() == nofActiveSoldier::STATE_ATTACKING_WAITINGAROUNDBUILDING)
                return false;
        }

        // Oder ein Kampf, der hier tobt?
        if((*it)->GetGOT() == GOT_FIGHTING)
            return false;
    }
    // object wall or impassable terrain increasing my path to target length to a higher value than the direct distance?
    return FindHumanPath(pt, center, CalcDistance(pt, center)) != 0xff;
}

bool GameWorldGame::ValidPointForFighting(const MapPoint pt, const bool avoid_military_building_flags, nofActiveSoldier* exception)
{
    // Is this a flag of a military building?
    if(avoid_military_building_flags && GetNO(pt)->GetGOT() == GOT_FLAG)
    {
        GO_Type got = GetNO(GetNeighbour(pt, Direction::NORTHWEST))->GetGOT();
        if(got == GOT_NOB_MILITARY || got == GOT_NOB_HARBORBUILDING || got == GOT_NOB_HQ)
            return false;
    }

    // Objekte, die sich hier befinden durchgehen
    const std::list<noBase*>& figures = GetFigures(pt);
    for(std::list<noBase*>::const_iterator it = figures.begin(); it != figures.end(); ++it)
    {
        // Ist hier ein anderer Soldat, der hier ebenfalls wartet?
        if((*it)->GetGOT() == GOT_NOF_ATTACKER || (*it)->GetGOT() == GOT_NOF_AGGRESSIVEDEFENDER || (*it)->GetGOT() == GOT_NOF_DEFENDER)
        {
            if(static_cast<nofActiveSoldier*>(*it) == exception)
                continue;
            switch(static_cast<nofActiveSoldier*>(*it)->GetState())
            {
                default: break;
                case nofActiveSoldier::STATE_WAITINGFORFIGHT:
                case nofActiveSoldier::STATE_ATTACKING_WAITINGAROUNDBUILDING:
                case nofActiveSoldier::STATE_ATTACKING_WAITINGFORDEFENDER:
                case nofActiveSoldier::STATE_DEFENDING_WAITING: return false;
            }
        }

        // Oder ein Kampf, der hier tobt?
        if((*it)->GetGOT() == GOT_FIGHTING)
        {
            if(static_cast<noFighting*>(*it)->IsActive() && !static_cast<noFighting*>(*it)->IsFighter(exception))
                return false;
        }
    }
    // Liegt hier was rum auf dem man nicht kämpfen sollte?
    const BlockingManner bm = GetNO(pt)->GetBM();
    return bm == BlockingManner::None || bm == BlockingManner::Tree || bm == BlockingManner::Flag;
}

bool GameWorldGame::IsPointCompletelyVisible(const MapPoint& pt, unsigned char player, const noBaseBuilding* exception) const
{
    sortedMilitaryBlds buildings = LookForMilitaryBuildings(pt, 3);

    // Sichtbereich von Militärgebäuden
    BOOST_FOREACH(const nobBaseMilitary* milBld, buildings)
    {
        if(milBld->GetPlayer() == player && milBld != exception)
        {
            // Prüfen, obs auch unbesetzt ist
            if(milBld->GetGOT() == GOT_NOB_MILITARY)
            {
                if(static_cast<const nobMilitary*>(milBld)->IsNewBuilt())
                    continue;
            }

            if(CalcDistance(pt, milBld->GetPos()) <= unsigned(milBld->GetMilitaryRadius() + VISUALRANGE_MILITARY))
                return true;
        }
    }

    // Sichtbereich von Hafenbaustellen
    BOOST_FOREACH(const noBuildingSite* bldSite, harbor_building_sites_from_sea)
    {
        if(bldSite->GetPlayer() == player && bldSite != exception)
        {
            if(CalcDistance(pt, bldSite->GetPos()) <= unsigned(HARBOR_RADIUS + VISUALRANGE_MILITARY))
                return true;
        }
    }

    // Sichtbereich von Spähtürmen
    BOOST_FOREACH(const nobUsual* bld, GetPlayer(player).GetBuildingRegister().GetBuildings(BLD_LOOKOUTTOWER)) //-V807
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
    namespace bl = boost::lambda;
    const unsigned range = std::max(VISUALRANGE_SCOUT, VISUALRANGE_SOLDIER);
    if(CheckPointsInRadius(pt, range, bl::bind(&GameWorldGame::IsScoutingFigureOnNode, this, bl::_1, player, bl::_2), true))
        return true;
    return IsPointScoutedByShip(pt, player);
}

bool GameWorldGame::IsScoutingFigureOnNode(const MapPoint& pt, unsigned player, unsigned distance) const
{
    BOOST_STATIC_ASSERT_MSG(VISUALRANGE_SCOUT >= VISUALRANGE_SOLDIER, "Visual range changed. Check loop below!");

    // Späher/Soldaten in der Nähe prüfen und direkt auf dem Punkt
    BOOST_FOREACH(noBase* obj, GetFigures(pt))
    {
        const GO_Type got = obj->GetGOT();
        // Check for scout. Note: no need to check for distance as scouts have higher distance than soldiers
        if(got == GOT_NOF_SCOUT_FREE)
        {
            // Prüfen, ob er auch am Erkunden ist und an der Position genau und ob es vom richtigen Spieler ist
            if(static_cast<nofScout_Free*>(obj)->GetPlayer() == player)
                return true;
            else
                continue;
        } else if(distance <= VISUALRANGE_SOLDIER)
        {
            // Soldaten?
            if(got == GOT_NOF_ATTACKER || got == GOT_NOF_AGGRESSIVEDEFENDER)
            {
                if(static_cast<nofActiveSoldier*>(obj)->GetPlayer() == player)
                    return true;
            }
            // Kämpfe (wo auch Soldaten drin sind)
            else if(got == GOT_FIGHTING)
            {
                // Prüfen, ob da ein Soldat vom angegebenen Spieler dabei ist
                if(static_cast<noFighting*>(obj)->IsSoldierOfPlayer(player))
                    return true;
            }
        }
    }

    return false;
}

bool GameWorldGame::IsPointScoutedByShip(const MapPoint& pt, unsigned player) const
{
    const std::vector<noShip*>& ships = GetPlayer(player).GetShips();
    BOOST_FOREACH(const noShip* ship, ships)
    {
        unsigned shipDistance = CalcDistance(pt, ship->GetPos());
        if(shipDistance <= ship->GetVisualRange())
            return true;
    }
    return false;
}

void GameWorldGame::RecalcVisibility(const MapPoint pt, const unsigned char player, const noBaseBuilding* const exception)
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
            case EXP_DISABLED:
            case EXP_CLASSIC:
                // einmal sichtbare Bereiche bleiben erhalten
                // nichts zu tun
                break;
            case EXP_FOGOFWAR:
            case EXP_FOGOFWARE_EXPLORED:
                // wenn es mal sichtbar war, nun im Nebel des Krieges
                if(visibility_before == VIS_VISIBLE)
                {
                    SetVisibility(pt, player, VIS_FOW, GetEvMgr().GetCurrentGF());
                }
                break;
            default: throw std::logic_error("Invalid exploration value");
        }
    }
}

void GameWorldGame::MakeVisible(const MapPoint pt, const unsigned char player)
{
    SetVisibility(pt, player, VIS_VISIBLE);
}

void GameWorldGame::RecalcVisibilitiesAroundPoint(const MapPoint pt, const MapCoord radius, const unsigned char player,
                                                  const noBaseBuilding* const exception)
{
    std::vector<MapPoint> pts = GetPointsInRadiusWithCenter(pt, radius);
    BOOST_FOREACH(const MapPoint& pt, pts)
        RecalcVisibility(pt, player, exception);
}

/// Setzt die Sichtbarkeiten um einen Punkt auf sichtbar (aus Performancegründen Alternative zu oberem)
void GameWorldGame::MakeVisibleAroundPoint(const MapPoint pt, const MapCoord radius, const unsigned char player)
{
    std::vector<MapPoint> pts = GetPointsInRadiusWithCenter(pt, radius);
    BOOST_FOREACH(const MapPoint& curPt, pts)
        MakeVisible(curPt, player);
}

/// Bestimmt bei der Bewegung eines spähenden Objekts die Sichtbarkeiten an
/// den Rändern neu
void GameWorldGame::RecalcMovingVisibilities(const MapPoint pt, const unsigned char player, const MapCoord radius,
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
        if(current_owner && (old_vis == VIS_INVISIBLE || (old_vis == VIS_FOW && old_owner != current_owner)))
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
        if(current_owner && (old_vis == VIS_INVISIBLE || (old_vis == VIS_FOW && old_owner != current_owner)))
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

    RecalcVisibility(t, player, NULL);
    tt = t;
    dir = anti_moving_dir + 2u;
    for(MapCoord i = 0; i < radius; ++i)
    {
        tt = GetNeighbour(tt, dir);
        RecalcVisibility(tt, player, NULL);
    }

    tt = t;
    dir = anti_moving_dir - 2u;
    for(unsigned i = 0; i < radius; ++i)
    {
        tt = GetNeighbour(tt, dir);
        RecalcVisibility(tt, player, NULL);
    }
}

bool GameWorldGame::IsBorderNode(const MapPoint pt, const unsigned char player) const
{
    return (GetNode(pt).owner == player && !IsPlayerTerritory(pt));
}

/**
 *  Konvertiert Ressourcen zwischen Typen hin und her oder löscht sie.
 *  Für Spiele ohne Gold.
 */
void GameWorldGame::ConvertMineResourceTypes(Resource::Type from, Resource::Type to)
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

/**
* Fills water depending on terrain and Addon setting
*/
void GameWorldGame::PlaceAndFixWater()
{
    bool waterEverywhere = GetGGS().getSelection(AddonId::EXHAUSTIBLE_WATER) == 1;

    RTTR_FOREACH_PT(MapPoint, GetSize())
    {
        Resource curNodeResource = GetNode(pt).resources;

        if (curNodeResource.getType() != Resource::Nothing && curNodeResource.getType() != Resource::Water) {
            // do not override maps resource.
            continue;
        }

        int amount = 0;

        // only set water if no desert, water, mountain or lava
        if (!(World::IsOfTerrain(pt, TT_DESERT) || World::IsOfTerrain(pt, TerrainData::IsWater)
            || World::IsOfTerrain(pt, TerrainData::IsMountain) || World::IsOfTerrain(pt, TerrainData::IsLava)))
        {
            
            if (waterEverywhere)
                amount = 7;
            // do not touch tile if waterEverywhere is disabled and no resource was stored by maploader
            else if (curNodeResource.getType() == Resource::Nothing) 
                amount = 0;
            else if (World::IsOfTerrain(pt, TT_SAVANNAH)) // reduce water on stepppe or savannah tiles.
                amount = 4;
            else if (World::IsOfTerrain(pt, TT_STEPPE))
                amount = 2;
            else
                amount = 7;
        }
    
        if (amount != 0)
        {
            curNodeResource.setType(Resource::Water);
            curNodeResource.setAmount(amount);
        } else
            curNodeResource.setType(Resource::Nothing);

        SetResource(pt, curNodeResource);
    }
}


/// Gründet vom Schiff aus eine neue Kolonie
bool GameWorldGame::FoundColony(const unsigned harbor_point, const unsigned char player, const unsigned short seaId)
{
    // Ist es hier überhaupt noch möglich, eine Kolonie zu gründen?
    if(!IsHarborAtSea(harbor_point, seaId) || !IsHarborPointFree(harbor_point, player))
        return false;

    MapPoint pos(GetHarborPoint(harbor_point));
    DestroyNO(pos, false);

    // Hafenbaustelle errichten
    noBuildingSite* bs = new noBuildingSite(pos, player);
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

void GameWorldGame::RemoveHarborBuildingSiteFromSea(noBuildingSite* building_site)
{
    RTTR_Assert(building_site->GetBuildingType() == BLD_HARBORBUILDING);
    harbor_building_sites_from_sea.remove(building_site);
}

bool GameWorldGame::IsHarborBuildingSiteFromSea(const noBuildingSite* building_site) const
{
    return helpers::contains(harbor_building_sites_from_sea, building_site);
}

std::vector<unsigned> GameWorldGame::GetUnexploredHarborPoints(const unsigned hbIdToSkip, const unsigned seaId, unsigned playerId) const
{
    std::vector<unsigned> hps;
    for(unsigned i = 1; i <= GetNumHarborPoints(); ++i)
    {
        if(i == hbIdToSkip || !IsHarborAtSea(i, seaId))
            continue;
        if(CalcVisiblityWithAllies(GetHarborPoint(i), playerId) != VIS_VISIBLE)
            hps.push_back(i);
    }
    return hps;
}

MapNode& GameWorldGame::GetNodeWriteable(const MapPoint pt)
{
    return GetNodeInt(pt);
}

void GameWorldGame::VisibilityChanged(const MapPoint pt, unsigned player, Visibility oldVis, Visibility newVis)
{
    GameWorldBase::VisibilityChanged(pt, player, oldVis, newVis);
    if(oldVis == VIS_INVISIBLE && newVis == VIS_VISIBLE && HasLua())
        GetLua().EventExplored(player, pt, GetNode(pt).owner);
    // Minimap Bescheid sagen
    if(gi)
        gi->GI_UpdateMinimap(pt);
}

/// Create Trade graphs
void GameWorldGame::CreateTradeGraphs()
{
    // Only if trade is enabled
    if(!GetGGS().isEnabled(AddonId::TRADE))
        return;

    TradePathCache::inst().Clear();
}
