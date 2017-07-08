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

#include "defines.h" // IWYU pragma: keep
#include "world/GameWorldGame.h"

#include "GameClient.h"
#include "GamePlayer.h"
#include "TradePathCache.h"
#include "GameInterface.h"
#include "postSystem/PostMsgWithBuilding.h"
#include "buildings/nobUsual.h"
#include "buildings/noBuildingSite.h"
#include "buildings/nobMilitary.h"
#include "figures/nofAttacker.h"
#include "figures/nofPassiveSoldier.h"
#include "figures/nofScout_Free.h"
#include "nodeObjs/noEnvObject.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noFighting.h"
#include "nodeObjs/noShip.h"
#include "lua/LuaInterfaceGame.h"
#include "world/TerritoryRegion.h"
#include "world/MapGeometry.h"
#include "pathfinding/PathConditions.h"
#include "EventManager.h"
#include "notifications/BuildingNote.h"
#include "notifications/ExpeditionNote.h"
#include "notifications/RoadNote.h"
#include "addons/const_addons.h"
#include "gameData/MilitaryConsts.h"
#include "gameData/TerrainData.h"
#include "gameData/GameConsts.h"
#include "gameData/SettingTypeConv.h"
#include "helpers/containerUtils.h"
#include <stdexcept>

inline std::vector<GamePlayer> CreatePlayers(const std::vector<PlayerInfo>& playerInfos, GameWorldGame& gwg)
{
    std::vector<GamePlayer> players;
    players.reserve(playerInfos.size());
    for(unsigned i = 0; i < playerInfos.size(); ++i)
        players.push_back(GamePlayer(i, playerInfos[i], gwg));
    return players;
}

GameWorldGame::GameWorldGame(const std::vector<PlayerInfo>& players, const GlobalGameSettings& gameSettings, EventManager& em):
    GameWorldBase(CreatePlayers(players, *this), gameSettings, em)
{
    TradePathCache::inst().Clear();
}

GameWorldGame::~GameWorldGame()
{}

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
        noBase* building = GetNO(GetNeighbour(pt, 1));

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

void GameWorldGame::SetPointRoad(MapPoint pt, unsigned char dir, unsigned char type)
{
    RTTR_Assert(dir < 6);

    if(dir >= 3)
        dir -= 3;
    else
        pt = GetNeighbour(pt, dir);

    SetRoad(pt, dir, type);

    if(gi)
        gi->GI_UpdateMinimap(pt);
}

void GameWorldGame::SetBuildingSite(const BuildingType type, const MapPoint pt, const unsigned char player)
{
    if(!GetPlayer(player).IsBuildingEnabled(type))
        return;

    // Gucken, ob das Gebäude hier überhaupt noch gebaut wrden kann
    const BuildingQuality bq = GetBQ(pt, player);

    switch(BUILDING_SIZE[type])
    {
        case BQ_HUT: if(!((bq >= BQ_HUT && bq <= BQ_CASTLE) || bq == BQ_HARBOR)) return; break;
        case BQ_HOUSE: if(!((bq >= BQ_HOUSE && bq <= BQ_CASTLE) || bq == BQ_HARBOR)) return; break;
        case BQ_CASTLE: if(!( bq == BQ_CASTLE || bq == BQ_HARBOR)) return; break;
        case BQ_HARBOR: if(bq != BQ_HARBOR) return; break;
        case BQ_MINE: if(bq != BQ_MINE) return; break;
        default: RTTR_Assert(false); break;
    }

    // Wenn das ein Militärgebäude ist und andere Militärgebäude bereits in der Nähe sind, darf dieses nicht gebaut werden
    if(type >= BLD_BARRACKS && type <= BLD_FORTRESS)
    {
        if(IsMilitaryBuildingNearNode(pt, player))
            return;
    }

    // Prüfen ob Katapult und ob Katapult erlaubt ist
    if (type == BLD_CATAPULT && !GetPlayer(player).CanBuildCatapult())
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
        noBaseBuilding* nbb  = GetSpecObj<noBaseBuilding>(pt);

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


void GameWorldGame::BuildRoad(const unsigned char playerId, const bool boat_road, const MapPoint start, const std::vector<unsigned char>& route)
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
            if (!RoadAlreadyBuilt(boat_road, start, route))
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
    }
    else
    {
        // Check if we can build a flag there
        if(GetBQ(curPt, playerId) == BQ_NOTHING || IsFlagAround(curPt))
        {
            GetNotifications().publish(RoadNote(RoadNote::ConstructionFailed, playerId, start, route));
            return;
        }
        //keine Flagge bisher aber spricht auch nix gegen ne neue Flagge -> Flagge aufstellen!
        SetFlag(curPt, playerId, (route[route.size() - 1] + 3) % 6);
    }

    // Evtl Zierobjekte abreißen (Anfangspunkt)
    if(IsObjectionableForRoad(start))
        DestroyNO(start);

    MapPoint end(start);
    for(unsigned i = 0; i < route.size(); ++i)
    {
        SetPointRoad(end, route[i], boat_road ? (RoadSegment::RT_BOAT + 1) : (RoadSegment::RT_NORMAL + 1));
        RecalcBQForRoad(end);
        end = GetNeighbour(end, route[i]);

        // Evtl Zierobjekte abreißen
        if(IsObjectionableForRoad(end))
            DestroyNO(end);
    }

    RoadSegment* rs = new RoadSegment(boat_road ? RoadSegment::RT_BOAT : RoadSegment::RT_NORMAL, 
                                      GetSpecObj<noFlag>(start), GetSpecObj<noFlag>(end), route);

    GetSpecObj<noFlag>(start)->routes[route.front()] = rs;
    GetSpecObj<noFlag>(end)->routes[(route.back() + 3) % 6] = rs;

    // Der Wirtschaft mitteilen, dass eine neue Straße gebaut wurde, damit sie alles Nötige macht
    GetPlayer(playerId).NewRoadConnection(rs);
    GetNotifications().publish(RoadNote(RoadNote::Constructed, playerId, start, route));
}

bool GameWorldGame::IsObjectionableForRoad(const MapPoint pt)
{
    if(GetNO(pt)->GetGOT() == GOT_ENVOBJECT)
    {
        noEnvObject* no = GetSpecObj<noEnvObject>(pt);
        unsigned short type = no->GetItemID();
        switch(no->GetItemFile())
        {
            case 0xFFFF: // map_?_z.lst
            {
                if(type == 505 || type == 506 || type == 507 || type == 508 || type == 510 || (type >= 542 && type <= 546) ||
                        type == 512 || type == 513 || // Kakteen
                        type == 536 || type == 541) // abgeerntete Getreidefelder
                    return true;
            } break;
            case 0:
            {
                // todo:
            } break;
            case 1:
            {
                if(type <= 12)
                    return true;
                // todo:
            } break;
            case 2:
            {
                // todo:
            } break;
            case 3:
            {
                // todo:
            } break;
            case 4:
            {
                // todo:
            } break;
            case 5:
            {
                // todo:
            } break;
            // Charburner rests
            case 6:
            {
                return true;
            } break;
        }
    }

    return false;
}

void GameWorldGame::RecalcTerritory(const noBaseBuilding& building, const bool destroyed, const bool newBuilt)
{
    RTTR_Assert(!destroyed || !newBuilt); // Both set is pointless

    // Radius der noch draufaddiert wird auf den eigentlich ausreichenden Bereich, für das Eliminieren von
    // herausragenden Landesteilen und damit Grenzsteinen
    static const int ADD_RADIUS = 2;
    // Get the military radius this building affects. Bld is either a military building or a harbor building site
    RTTR_Assert((building.GetBuildingType() == BLD_HARBORBUILDING && dynamic_cast<const noBuildingSite*>(&building)) || dynamic_cast<const nobBaseMilitary*>(&building));
    const unsigned militaryRadius = (building.GetBuildingType() == BLD_HARBORBUILDING) ? HARBOR_RADIUS : static_cast<const nobBaseMilitary&>(building).GetMilitaryRadius();

    TerritoryRegion region = CreateTerritoryRegion(building, militaryRadius + ADD_RADIUS, destroyed);

    // Set to true, where owner has changed (initially all false)
    std::vector<bool> ownerChanged(region.size.x * region.size.y, false);

    std::vector<int> sizeChanges(GetPlayerCount());
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
			if (noAlliedBorderPush)
			{
				//rule 1: only take territory from an ally if that ally loses a building - special case: headquarter can take territory
                if(oldOwner > 0 && newOwner > 0 && GetPlayer(oldOwner - 1).IsAlly(newOwner - 1) && (ownerOfTriggerBld != oldOwner || newBuilt) && building.GetBuildingType() != BLD_HEADQUARTERS)
					continue;
				//rule 2: do not gain territory when you lose a building (captured or destroyed)
                if(ownerOfTriggerBld == newOwner && !newBuilt)
					continue;
				//rule 3: do not lose territory when you gain a building (newBuilt or capture)
                if((ownerOfTriggerBld == oldOwner && oldOwner > 0 && newBuilt) || (newOwnerOfTriggerBld == oldOwner && !destroyed && !newBuilt))
					continue;
			}
            SetOwner(curMapPt, newOwner);
            ownerChanged[region.GetIdx(pt)] = true;
            if (newOwner != 0)
                sizeChanges[newOwner - 1]++;
            if (oldOwner != 0)
                sizeChanges[oldOwner - 1]--;

            // Event for map scripting
            if(newOwner != 0 && HasLua())
                GetLua().EventOccupied(newOwner - 1, curMapPt);
        }
    }

    for (unsigned i = 0; i < GetPlayerCount(); ++i)
    {
        GetPlayer(i).ChangeStatisticValue(STAT_COUNTRY, sizeChanges[i]);

        // Negatives Wachstum per Post dem/der jeweiligen Landesherren/dame melden, nur bei neugebauten Gebäuden
        if (newBuilt && sizeChanges[i] < 0)
        {
            GetPostMgr().SendMsg(i, new PostMsgWithBuilding(GetEvMgr().GetCurrentGF(), _("Lost land by this building"), PostCategory::Military, building));
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
                    if(IsPlayerTerritory(GetNeighbour(curMapPt, d)))
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
                    MapPoint neighbourPt = GetNeighbour(curMapPt, i);

                    DestroyPlayerRests(neighbourPt, GetNode(curMapPt).owner, &building, false);

                    // BQ neu berechnen
                    RecalcBQ(neighbourPt);
                    // ggf den noch darüber, falls es eine Flagge war (kann ja ein Gebäude entstehen)
                    if(GetNeighbourNode(neighbourPt, 1).bq != BQ_NOTHING)
                        RecalcBQ(GetNeighbour(neighbourPt, 1));
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
    if(destroyed)
        RecalcVisibilitiesAroundPoint(building.GetPos(), visualRadius, building.GetPlayer(), &building);
    else
        SetVisibilitiesAroundPoint(building.GetPos(), visualRadius, building.GetPlayer());
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
                    if(IsBorderNode(GetNeighbour(curMapPt, 3 + i), owner))
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

bool GameWorldGame::DoesTerritoryChange(const noBaseBuilding& building, const bool destroyed, const bool  /*newBuilt*/) const
{
    // Get the military radius this building affects. Bld is either a military building or a harbor building site
    RTTR_Assert((building.GetBuildingType() == BLD_HARBORBUILDING && dynamic_cast<const noBuildingSite*>(&building)) || dynamic_cast<const nobBaseMilitary*>(&building));
    const unsigned militaryRadius = (building.GetBuildingType() == BLD_HARBORBUILDING) ? HARBOR_RADIUS : static_cast<const nobBaseMilitary&>(building).GetMilitaryRadius();

    TerritoryRegion region = CreateTerritoryRegion(building, militaryRadius, destroyed);

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
                return true;
            TerrainType t1 = GetNode(curMapPt).t1, t2 = GetNode(curMapPt).t2;
            if(TerrainData::IsUseable(t1) && TerrainData::IsUseable(t2))
                return true;
            //also check neighboring nodes for their terrain since border will still count as player territory but not allow any buildings !
            for(int dir = 0; dir < 6; dir++)
            {
                t1 = GetNeighbourNode(curMapPt, dir).t1;
                t2 = GetNeighbourNode(curMapPt, dir).t2;
                if(TerrainData::IsUseable(t1) || TerrainData::IsUseable(t2))
                    return true;
            }
        }
    }
    return false;
}

TerritoryRegion GameWorldGame::CreateTerritoryRegion(const noBaseBuilding& building, unsigned radius, const bool destroyed) const
{
    const MapPoint bldPos = building.GetPos();
    sortedMilitaryBlds buildings = LookForMilitaryBuildings(bldPos, 3);

    // Span at most half the map size (assert even sizes, given due to layout)
    RTTR_Assert(GetWidth() % 2 == 0);
    RTTR_Assert(GetHeight() % 2 == 0);
    unsigned radiusX = std::min(radius, GetWidth() / 2u);
    unsigned radiusY = std::min(radius, GetHeight() / 2u);

    // Koordinaten erzeugen für TerritoryRegion
    const Point<int> startPt = Point<int>(bldPos) - Point<int>(radiusX, radiusY);
    // All points in the region are less than endPt. If we want to check the same number of points right of bld as left we need a +1
    // unless radius is already half the map size in which case we would check the first point twice -> clamp to size/2
    const Point<int> endPt   = Point<int>(bldPos) + Point<int>(std::min(radiusX + 1, GetWidth() / 2u), std::min(radiusY + 1, GetHeight() / 2u));
    TerritoryRegion region(startPt, endPt, *this);

    // Alle Gebäude ihr Terrain in der Nähe neu berechnen
    for(sortedMilitaryBlds::iterator it = buildings.begin(); it != buildings.end(); ++it)
    {
        // Ist es ein richtiges Militärgebäude?
        if((*it)->GetBuildingType() >= BLD_BARRACKS && (*it)->GetBuildingType() <= BLD_FORTRESS)
        {
            // Wenn es noch nicht besetzt war(also gerade neu gebaut), darf es nicht mit einberechnet werden!
            if(static_cast<nobMilitary*>(*it)->IsNewBuilt())
                continue;
        }

        // Wenn das Gebäude abgerissen wird oder wenn es noch nicht besetzt war, natürlich nicht mit einberechnen
        if(*it != &building || !destroyed)
            region.CalcTerritoryOfBuilding(**it);
    }

    // Baustellen von Häfen mit einschließen
    for(std::list<noBuildingSite*>::const_iterator it = harbor_building_sites_from_sea.begin(); it != harbor_building_sites_from_sea.end(); ++it)
    {
        if(*it != &building || !destroyed)
            region.CalcTerritoryOfBuilding(**it);
    }

    return region;
}

void GameWorldGame::DestroyPlayerRests(const MapPoint pt, const unsigned char newOwner, const noBaseBuilding* exception, bool allowDestructionOfMilBuildings)
{
    noBase* no = GetNO(pt);

    // Flaggen, Gebäude und Baustellen zerstören, aber keine übernommenen und nicht die Ausahme oder dessen Flagge!
    if((no->GetType() == NOP_FLAG || no->GetType() == NOP_BUILDING || no->GetType() == NOP_BUILDINGSITE) && exception != no)
    {
        // Wurde das Objekt auch nicht vom Gegner übernommen?
        if(static_cast<noRoadNode*>(no)->GetPlayer() + 1 != newOwner)
        {
			//maybe buildings that push territory should not be destroyed right now?- can happen with improved alliances addon or in rare cases even without the addon so allow those buildings & their flag to survive.
			if(!allowDestructionOfMilBuildings)
			{
                const noBase* noCheckMil = (no->GetType() == NOP_FLAG) ? GetNO(GetNeighbour(pt, 1)) : no;
				if(noCheckMil->GetGOT() == GOT_NOB_HQ ||
                    noCheckMil->GetGOT() == GOT_NOB_HARBORBUILDING ||
                    (noCheckMil->GetGOT() == GOT_NOB_MILITARY && !dynamic_cast<const nobMilitary*>(noCheckMil)->IsNewBuilt()) ||
                    (noCheckMil->GetType() == NOP_BUILDINGSITE && dynamic_cast<const noBuildingSite*>(noCheckMil)->IsHarborBuildingSiteFromSea()))
				{
					//LOG.write(("DestroyPlayerRests of hq, military, harbor or colony-harbor in construction stopped at x, %i y, %i type, %i \n", x, y, no->GetType());
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
    unsigned char dir;
    noFlag* flag = GetRoadFlag(pt, dir, 0xFF);
    if(flag)
    {
        // Die Ministraße von dem Militärgebäude nich abreißen!
        if(flag->routes[dir]->GetLength() == 1)
        {
            if(flag->routes[dir]->GetF2() == exception)
                return;
        }

        flag->DestroyRoad(dir);
    }
}

bool GameWorldGame::IsNodeForFigures(const MapPoint pt) const
{
    // Nicht über die Kante gehen!
    if(pt.x >= GetWidth() || pt.y >= GetHeight())
        return false;

    // Irgendwelche Objekte im Weg?
    const BlockingManner bm = GetNO(pt)->GetBM();
    if(bm != BlockingManner::None && bm != BlockingManner::Tree && bm != BlockingManner::Flag)
        return false;

    // Terrain untersuchen
    unsigned char good_terrains = 0;
    for(unsigned char i = 0; i < 6; ++i)
    {
        TerrainBQ bq = TerrainData::GetBuildingQuality(GetTerrainAround(pt, i));
        if(bq == TerrainBQ::DANGER)
            return false; // in die Nähe von Lava usw. dürfen die Figuren gar nich kommen!
        else if(bq != TerrainBQ::NOTHING)
            ++good_terrains;
    }

    // Darf nicht im Wasser liegen, 
    return good_terrains != 0;
}

void GameWorldGame::RoadNodeAvailable(const MapPoint pt)
{
    // Figuren direkt daneben
    for(unsigned char i = 0; i < 6; ++i)
    {
        // Nochmal prüfen, ob er nun wirklich verfügbar ist (evtl blocken noch mehr usw.)
        if(!IsRoadNodeForFigures(pt, (i + 3) % 6))
            continue;

        // Koordinaten um den Punkt herum
        MapPoint nb = GetNeighbour(pt, i);

        // Figuren Bescheid sagen, es können auch auf den Weg gestoppte sein, die müssen auch berücksichtigt
        // werden, daher die *From-Methode
        std::vector<noBase*> objects = GetDynamicObjectsFrom(nb);

        // Auch Figuren da, die rumlaufen können?
        if(!objects.empty())
        {
            for(std::vector<noBase*>::iterator it = objects.begin(); it != objects.end(); ++it)
            {
                if((*it)->GetType() == NOP_FIGURE)
                    static_cast<noFigure*>(*it)->NodeFreed(pt);
            }
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

void GameWorldGame::Attack(const unsigned char player_attacker, const MapPoint pt, const unsigned short soldiers_count, const bool strong_soldiers)
{
    nobBaseMilitary* attacked_building = GetSpecObj<nobBaseMilitary>(pt);
    if(!attacked_building || !attacked_building->IsAttackable(player_attacker))
        return;

    // Prüfen, ob der angreifende Spieler das Gebäude überhaupt sieht (Cheatvorsorge)
    if(CalcVisiblityWithAllies(pt, player_attacker) != VIS_VISIBLE)
        return;

    // Militärgebäude in der Nähe finden
    sortedMilitaryBlds buildings = LookForMilitaryBuildings(pt, 3);

    // Liste von verfügbaren Soldaten, geordnet einfügen, damit man dann starke oder schwache Soldaten nehmen kann
    std::list<PotentialAttacker> soldiers;


    for(sortedMilitaryBlds::iterator it = buildings.begin(); it != buildings.end(); ++it) {
        // Muss ein Gebäude von uns sein und darf nur ein "normales Militärgebäude" sein (kein HQ etc.)
        if((*it)->GetPlayer() != player_attacker || (*it)->GetBuildingType() < BLD_BARRACKS || (*it)->GetBuildingType() > BLD_FORTRESS)
            continue;

        // Soldaten ausrechnen, wie viel man davon nehmen könnte, je nachdem wie viele in den
        // Militäreinstellungen zum Angriff eingestellt wurden
        unsigned soldiers_count =
            (static_cast<nobMilitary*>(*it)->GetTroopsCount() > 1) ?
            ((static_cast<nobMilitary*>(*it)->GetTroopsCount() - 1) * GetPlayer(player_attacker).GetMilitarySetting(3) / MILITARY_SETTINGS_SCALE[3]) : 0;

        unsigned int distance = CalcDistance(pt, (*it)->GetPos());

        // Falls Entfernung größer als Basisreichweite, Soldaten subtrahieren
        if (distance > BASE_ATTACKING_DISTANCE)
        {
            // je einen soldaten zum entfernen vormerken für jeden EXTENDED_ATTACKING_DISTANCE großen Schritt
            unsigned soldiers_to_remove = ((distance - BASE_ATTACKING_DISTANCE + EXTENDED_ATTACKING_DISTANCE - 1) / EXTENDED_ATTACKING_DISTANCE);
            if (soldiers_to_remove < soldiers_count)
                soldiers_count -= soldiers_to_remove;
            else
                continue;
        }

        if(!soldiers_count)
            continue;

        // The path should not be to far. If it is skip this
        // Also use a bit of tolerance for the path
        if(FindHumanPath(pt, (*it)->GetPos(), MAX_ATTACKING_RUN_DISTANCE) == 0xFF) // TODO check: hier wird ne random-route berechnet? soll das so?
            continue;

        // Take soldier(s)
        unsigned i = 0;
        const SortedTroops& troops = static_cast<nobMilitary*>(*it)->GetTroops();
        if(strong_soldiers){
            // Strong soldiers first
            for(SortedTroops::const_reverse_iterator it2 = troops.rbegin();
                    it2 != troops.rend() && i < soldiers_count;
                    ++it2, ++i){
                bool inserted = false;
                for(std::list<PotentialAttacker>::iterator it3 = soldiers.begin(); it3 != soldiers.end(); ++it3){
                    /* Insert new soldier before current one if:
                            new soldiers rank is greater
                            OR new soldiers rank is equal AND new soldiers distance is smaller */
                    if(it3->soldier->GetRank() < (*it2)->GetRank() ||
                            (it3->soldier->GetRank() == (*it2)->GetRank() && it3->distance > distance)){
                        PotentialAttacker pa = { *it2, distance };
                        soldiers.insert(it3, pa);
                        inserted = true;
                        break;
                    }
                }
                if(!inserted){
                    PotentialAttacker pa = { *it2, distance };
                    soldiers.push_back(pa);
                }
            }
        }else{
            // Weak soldiers first
            for(SortedTroops::const_iterator it2 = troops.begin();
                    it2 != troops.end() && i < soldiers_count;
                    ++it2, ++i){
                bool inserted = false;
                for(std::list<PotentialAttacker>::iterator it3 = soldiers.begin(); it3 != soldiers.end(); ++it3){
                    /* Insert new soldier before current one if:
                            new soldiers rank is less
                            OR new soldiers rank is equal AND new soldiers distance is smaller */
                    if(it3->soldier->GetRank() > (*it2)->GetRank() ||
                            (it3->soldier->GetRank() == (*it2)->GetRank() && it3->distance > distance)){
                        PotentialAttacker pa = { *it2, distance };
                        soldiers.insert(it3, pa);
                        inserted = true;
                        break;
                    }
                }
                if(!inserted){
                    PotentialAttacker pa = { *it2, distance };
                    soldiers.push_back(pa);
                }
            }
        } // End weak/strong check
    }

    // Send the soldiers to attack
    unsigned short i = 0;

    for(std::list<PotentialAttacker>::iterator it = soldiers.begin(); it != soldiers.end() && i < soldiers_count; ++i, ++it)
    {
        // neuen Angreifer-Soldaten erzeugen
        new nofAttacker(it->soldier, attacked_building);
        // passiven Soldaten entsorgen
        it->soldier->Destroy();
        delete it->soldier;
    }

    /*if(soldiers.empty())
        LOG.write(S("GameWorldGame::Attack: WARNING: Attack failed. No Soldiers available!\n");*/
}

void  GameWorldGame::AttackViaSea(const unsigned char player_attacker, const MapPoint pt, const unsigned short soldiers_count, const bool strong_soldiers)
{
    //sea attack abgeschaltet per addon?
    if(GetGGS().getSelection(AddonId::SEA_ATTACK) == 2)
        return;

    // Prüfen, ob der angreifende Spieler das Gebäude überhaupt sieht (Cheatvorsorge)
    if(CalcVisiblityWithAllies(pt, player_attacker) != VIS_VISIBLE)
        return;

    // Ist das angegriffenne ein normales Gebäude?
    nobBaseMilitary* attacked_building = GetSpecObj<nobBaseMilitary>(pt);
    if(!attacked_building || !attacked_building->IsAttackable(player_attacker))
        return;

    // Verfügbare Soldaten herausfinden
    std::vector<GameWorldBase::PotentialSeaAttacker> attackers = GetSoldiersForSeaAttack(player_attacker, pt);

    unsigned counter = 0;
    if(strong_soldiers)
        for(std::vector<GameWorldBase::PotentialSeaAttacker>::iterator it = attackers.begin(); it != attackers.end() &&
                counter < soldiers_count; ++it, ++counter)
        {
            // neuen Angreifer-Soldaten erzeugen
            new nofAttacker(it->soldier, attacked_building, it->harbor);
            // passiven Soldaten entsorgen
            it->soldier->Destroy();
            delete it->soldier;
        }
    else
        for(std::vector<GameWorldBase::PotentialSeaAttacker>::reverse_iterator it = attackers.rbegin(); it != attackers.rend() &&
                counter < soldiers_count; ++it, ++counter)
        {
            // neuen Angreifer-Soldaten erzeugen
            new nofAttacker(it->soldier, attacked_building, it->harbor);
            // passiven Soldaten entsorgen
            it->soldier->Destroy();
            delete it->soldier;
        }
}


bool GameWorldGame::IsRoadNodeForFigures(const MapPoint pt, const unsigned char  /*dir*/)
{
    /// Objekte sammeln
    std::vector<noBase*> objects = GetDynamicObjectsFrom(pt);

    // Figuren durchgehen, bei Kämpfen und wartenden Angreifern sowie anderen wartenden Figuren stoppen!
    for(std::vector<noBase*>::iterator it = objects.begin(); it != objects.end(); ++it)
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
        if((*it)->GetGOT() == GOT_FIGHTING)
        {
            if(static_cast<noFighting*>(*it)->IsActive())
                return false;
        }

        //// wartende Angreifer
        if((*it)->GetGOT() == GOT_NOF_ATTACKER)
        {
            if(static_cast<nofAttacker*>(*it)->IsBlockingRoads())
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
    for(unsigned d = 0; d < 6; ++d)
    {
        const std::list<noBase*>& fieldFigures = GetFigures(GetNeighbour(pt, d));
        for(std::list<noBase*>::const_iterator it = fieldFigures.begin(); it != fieldFigures.end(); ++it)
            if((*it)->GetType() == NOP_FIGURE)
                figures.push_back(*it);
    }

    for(std::vector<noBase*>::iterator it = figures.begin(); it != figures.end(); ++it)
    {
        if(dir < 6)
        {
            if((dir + 3) % 6 == static_cast<noFigure*>(*it)->GetCurMoveDir())
            {
                if(GetNeighbour(pt, dir) == static_cast<noFigure*>(*it)->GetPos())
                    continue;
            }
        }

        // Derjenige muss ggf. stoppen, wenn alles auf ihn zutrifft
        static_cast<noFigure*>(*it)->StopIfNecessary(pt);
    }
}

void GameWorldGame::Armageddon()
{
    MapPoint pt(0, 0);
    for(pt.y = 0; pt.y < GetHeight(); pt.y++)
        for(pt.x = 0; pt.x < GetWidth(); pt.x++)
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
    MapPoint pt(0, 0);
    for(pt.y = 0; pt.y < GetHeight(); pt.y++)
        for(pt.x = 0; pt.x < GetWidth(); pt.x++)
        {
            noFlag* flag = GetSpecObj<noFlag>(pt);
            if(flag && flag->GetPlayer() == player)
            {
                flag->DestroyAttachedBuilding();
                DestroyNO(pt, false);
            }
        }
}

bool GameWorldGame::ValidWaitingAroundBuildingPoint(const MapPoint pt, nofAttacker*  /*attacker*/, const MapPoint center)
{
    // Gültiger Punkt für Figuren?
    if(!IsNodeForFigures(pt))
        return false;

    // Objekte, die sich hier befinden durchgehen
    const std::list<noBase*>& figures = GetFigures(pt);
    for(std::list<noBase*>::const_iterator it = figures.begin(); it != figures.end(); ++it)
    {
        // Ist hier ein anderer Soldat, der hier ebenfalls wartet?
        if((*it)->GetGOT() == GOT_NOF_ATTACKER || (*it)->GetGOT() == GOT_NOF_AGGRESSIVEDEFENDER ||
                (*it)->GetGOT() == GOT_NOF_DEFENDER)
        {
            if(static_cast<nofActiveSoldier*>(*it)->GetState() == nofActiveSoldier::STATE_WAITINGFORFIGHT ||
                    static_cast<nofActiveSoldier*>(*it)->GetState() == nofActiveSoldier::STATE_ATTACKING_WAITINGAROUNDBUILDING )
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
        GO_Type got = GetNO(GetNeighbour(pt, 1))->GetGOT();
        if(got == GOT_NOB_MILITARY || got == GOT_NOB_HARBORBUILDING || got == GOT_NOB_HQ)
            return false;
    }

    // Objekte, die sich hier befinden durchgehen
    const std::list<noBase*>& figures = GetFigures(pt);
    for(std::list<noBase*>::const_iterator it = figures.begin(); it != figures.end(); ++it)
    {
        // Ist hier ein anderer Soldat, der hier ebenfalls wartet?
        if((*it)->GetGOT() == GOT_NOF_ATTACKER || (*it)->GetGOT() == GOT_NOF_AGGRESSIVEDEFENDER ||
                (*it)->GetGOT() == GOT_NOF_DEFENDER)
        {
            if (static_cast<nofActiveSoldier*>(*it) == exception)
                continue;
            switch(static_cast<nofActiveSoldier*>(*it)->GetState())
            {
                default: break;
                case nofActiveSoldier::STATE_WAITINGFORFIGHT:
                case nofActiveSoldier::STATE_ATTACKING_WAITINGAROUNDBUILDING:
                case nofActiveSoldier::STATE_ATTACKING_WAITINGFORDEFENDER:
                case nofActiveSoldier::STATE_DEFENDING_WAITING:
                    return false;
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

bool GameWorldGame::IsPointCompletelyVisible(const MapPoint pt, const unsigned char player, const noBaseBuilding* const exception) const
{
    sortedMilitaryBlds buildings = LookForMilitaryBuildings(pt, 3);

    // Sichtbereich von Militärgebäuden
    for(sortedMilitaryBlds::iterator it = buildings.begin(); it != buildings.end(); ++it)
    {
        if((*it)->GetPlayer() == player && *it != exception)
        {
            // Prüfen, obs auch unbesetzt ist
            if((*it)->GetGOT() == GOT_NOB_MILITARY)
            {
                if(static_cast<nobMilitary*>(*it)->IsNewBuilt())
                    continue;
            }

            if(CalcDistance(pt, (*it)->GetPos()) <= unsigned((*it)->GetMilitaryRadius() + VISUALRANGE_MILITARY))
                return true;
        }
    }

    // Sichtbereich von Hafenbaustellen
    for(std::list<noBuildingSite*>::const_iterator it = harbor_building_sites_from_sea.begin();
            it != harbor_building_sites_from_sea.end(); ++it)
    {
        if((*it)->GetPlayer() == player && *it != exception)
        {

            if(CalcDistance(pt, (*it)->GetPos())
                    <= unsigned(HARBOR_RADIUS + VISUALRANGE_MILITARY))
                return true;
        }
    }

    // Sichtbereich von Spähtürmen

    for(std::list<nobUsual*>::const_iterator it = GetPlayer(player).GetBuildings(BLD_LOOKOUTTOWER).begin();
            it != GetPlayer(player).GetBuildings(BLD_LOOKOUTTOWER).end(); ++it)
    {
        // Ist Späturm überhaupt besetzt?
        if(!(*it)->HasWorker())
            continue;

        // Nicht die Ausnahme wählen
        if(*it == exception)
            continue;

        // Liegt Spähturm innerhalb des Sichtradius?
        if(CalcDistance(pt, (*it)->GetPos()) <= VISUALRANGE_LOOKOUTTOWER)
            return true;
    }



    // Erkunder prüfen

    // Zunächst auf dem Punkt selbst
    if(IsScoutingFigureOnNode(pt, player, 0))
        return true;

    // Und drumherum
    for(MapCoord tx = GetXA(pt.x, pt.y, 0), r = 1; r <= VISUALRANGE_EXPLORATION_SHIP; tx = GetXA(tx, pt.y, 0), ++r)
    {
        MapPoint t2(tx, pt.y);
        for(unsigned i = 2; i < 8; ++i)
        {
            for(MapCoord r2 = 0; r2 < r; t2 = GetNeighbour(t2, i % 6), ++r2)
            {
                if(IsScoutingFigureOnNode(t2, player, r))
                    return true;
            }
        }
    }

    return false;
}

bool GameWorldGame::IsScoutingFigureOnNode(const MapPoint pt, const unsigned player, const unsigned distance) const
{
    std::vector<noBase*> objects = GetDynamicObjectsFrom(pt);

    // Späher/Soldaten in der Nähe prüfen und direkt auf dem Punkt
    for(std::vector<noBase*>::iterator it = objects.begin(); it != objects.end(); ++it)
    {
        if(distance <= VISUALRANGE_SCOUT)
        {
            // Späher?
            if((*it)->GetGOT() == GOT_NOF_SCOUT_FREE)
            {
                // Prüfen, ob er auch am Erkunden ist und an der Position genau und ob es vom richtigen Spieler ist
                nofScout_Free* scout = static_cast<nofScout_Free*>(*it);
                if(scout->GetPos() == pt && scout->GetPlayer() == player)
                    return true;
            }
        }

        // Soldaten?
        if(distance <= VISUALRANGE_SOLDIER)
        {
            // Soldaten?
            if((*it)->GetGOT() == GOT_NOF_ATTACKER || (*it)->GetGOT() == GOT_NOF_AGGRESSIVEDEFENDER)
            {
                nofActiveSoldier* soldier = static_cast<nofActiveSoldier*>(*it);
                if(soldier->GetPos() == pt && soldier->GetPlayer() == player)
                    return true;
            }
            // Kämpfe (wo auch Soldaten drin sind)
            else if((*it)->GetGOT() == GOT_FIGHTING)
            {
                // Prüfen, ob da ein Soldat vom angegebenen Spieler dabei ist
                if(static_cast<noFighting*>(*it)->IsSoldierOfPlayer(player))
                    return true;
            }
        }

        // Schiffe?

        if((*it)->GetGOT() == GOT_SHIP)
        {
            noShip* ship = static_cast<noShip*>(*it);
            if(distance <= ship->GetVisualRange())
            {
                if(ship->GetPos() == pt && ship->GetPlayerId() == player)
                    return true;
            }
        }

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
    {
        if (visibility_before != VIS_VISIBLE && HasLua())
            GetLua().EventExplored(player, pt, GetNode(pt).owner);
        SetVisibility(pt, player, VIS_VISIBLE, GetEvMgr().GetCurrentGF());
    }
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
            default:
                throw std::logic_error("Invalid exploration value");
        }
    }
}

void GameWorldGame::MakeVisible(const MapPoint pt, const unsigned char player)
{
    Visibility visibility_before = GetNode(pt).fow[player].visibility;
    SetVisibility(pt, player, VIS_VISIBLE, GetEvMgr().GetCurrentGF());

    if (visibility_before != VIS_VISIBLE && HasLua())
        GetLua().EventExplored(player, pt, GetNode(pt).owner);
}

void GameWorldGame::RecalcVisibilitiesAroundPoint(const MapPoint pt, const MapCoord radius, const unsigned char player, const noBaseBuilding* const exception)
{
    RecalcVisibility(pt, player, exception);

    for(MapCoord tx = GetXA(pt.x, pt.y, 0), r = 1; r <= radius; tx = GetXA(tx, pt.y, 0), ++r)
    {
        MapPoint t2(tx, pt.y);
        for(unsigned i = 2; i < 8; ++i)
        {
            for(MapCoord r2 = 0; r2 < r; t2 = GetNeighbour(t2, i % 6), ++r2)
                RecalcVisibility(t2, player, exception);
        }
    }
}

/// Setzt die Sichtbarkeiten um einen Punkt auf sichtbar (aus Performancegründen Alternative zu oberem)
void GameWorldGame::SetVisibilitiesAroundPoint(const MapPoint pt, const MapCoord radius, const unsigned char player)
{
    MakeVisible(pt, player);

    for(MapCoord tx = GetXA(pt, 0), r = 1; r <= radius; tx = GetXA(tx, pt.y, 0), ++r)
    {
        MapPoint t2(tx, pt.y);
        for(unsigned i = 2; i < 8; ++i)
        {
            for(MapCoord r2 = 0; r2 < r; t2 = GetNeighbour(t2, i % 6), ++r2)
                MakeVisible(t2, player);
        }
    }
}

/// Bestimmt bei der Bewegung eines spähenden Objekts die Sichtbarkeiten an
/// den Rändern neu
void GameWorldGame::RecalcMovingVisibilities(const MapPoint pt, const unsigned char player, const MapCoord radius, 
        const unsigned char moving_dir, MapPoint * enemy_territory)
{
    // Neue Sichtbarkeiten zuerst setzen
    // Zum Eckpunkt der beiden neuen sichtbaren Kanten gehen
    MapPoint t(pt);
    for(MapCoord i = 0; i < radius; ++i)
        t = GetNeighbour(t, moving_dir);

    // Und zu beiden Abzweigungen weiter gehen und Punkte auf visible setzen
    MakeVisible(t, player);
    MapPoint tt(t);
    unsigned char dir = (moving_dir + 2) % 6;
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
        if(current_owner && (old_vis == VIS_INVISIBLE ||
                             (old_vis == VIS_FOW && old_owner != current_owner)))
        {
            if(GetPlayer(player).IsAttackable(current_owner - 1) && enemy_territory)
            {
                *enemy_territory = tt;
            }
        }
    }

    tt = t;
    dir = (moving_dir + 6 - 2) % 6;
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
        if(current_owner && (old_vis == VIS_INVISIBLE ||
                             (old_vis == VIS_FOW && old_owner != current_owner)))
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
    unsigned char anti_moving_dir = (moving_dir + 3) % 6;
    for(MapCoord i = 0; i < radius + 1; ++i)
        t = GetNeighbour(t, anti_moving_dir);

    RecalcVisibility(t, player, NULL);
    tt = t;
    dir = (anti_moving_dir + 2) % 6;
    for(MapCoord i = 0; i < radius; ++i)
    {
        tt = GetNeighbour(tt, dir);
        RecalcVisibility(tt, player, NULL);
    }

    tt = t;
    dir = (anti_moving_dir + 6 - 2) % 6;
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
void GameWorldGame::ConvertMineResourceTypes(unsigned char from, unsigned char to)
{
    // to == 0xFF heißt löschen
    // in Map-Resource-Koordinaten konvertieren
    from = RESOURCES_MINE_TO_MAP[from];
    to = ((to != 0xFF) ? RESOURCES_MINE_TO_MAP[to] : 0xFF);

    //LOG.write(("Convert map resources from %i to %i\n", from, to);
    // Alle Punkte durchgehen
    MapPoint pt(0, 0);
    for(pt.y = 0; pt.y < GetHeight(); ++pt.y)
        for (pt.x = 0; pt.x < GetWidth(); ++pt.x)
        {
            unsigned char resources = GetNode(pt).resources;
            // Gibt es Ressourcen dieses Typs?
            // Wenn ja, dann umwandeln bzw löschen
            if (resources >= 0x40 + from * 8 && resources < 0x48 + from * 8)
                SetResource(pt, (to != 0xFF) ?  resources - (from * 8 - to * 8) : 0);
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

    RecalcTerritory(*bs, false, true);
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
    for(unsigned i = 1; i <= GetHarborPointCount(); ++i)
    {
        if(i == hbIdToSkip || !IsHarborAtSea(i, seaId))
            continue;
        if(CalcVisiblityWithAllies(GetHarborPoint(i), playerId) != VIS_VISIBLE)
            hps.push_back(i);
    }
    return hps;
}

bool GameWorldGame::IsResourcesOnNode(const MapPoint pt, const unsigned char type) const
{
    RTTR_Assert(pt.x < GetWidth());
    RTTR_Assert(pt.y < GetHeight());

    unsigned char resources = GetNode(pt).resources;

    // wasser?
    if(type == 4)
        return (resources > 0x20 && resources < 0x28);

    // Gibts Ressourcen von dem Typ an diesem Punkt?
    return (resources > 0x40 + type * 8 && resources < 0x48 + type * 8);
}


MapNode& GameWorldGame::GetNodeWriteable(const MapPoint pt)
{
    return GetNodeInt(pt);
}

void GameWorldGame::VisibilityChanged(const MapPoint pt, unsigned player)
{
    GameWorldBase::VisibilityChanged(pt, player);
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
