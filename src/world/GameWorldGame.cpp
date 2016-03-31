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
#include "world/GameWorldGame.h"

#include "GameClient.h"
#include "GameClientPlayer.h"
#include "TradePathCache.h"
#include "GameInterface.h"
#include "PostMsg.h"
#include "ai/AIEvents.h"
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
#include "gameData/MilitaryConsts.h"
#include "gameData/TerrainData.h"
#include "gameData/GameConsts.h"
#include "helpers/containerUtils.h"

#include <stdexcept>

// Include last!
#include "DebugNew.h" // IWYU pragma: keep
class CatapultStone;
class MilitarySquares;

GameWorldGame::GameWorldGame()
{
    TradePathCache::inst().Clear();
}

GameWorldGame::~GameWorldGame()
{}

MilitarySquares& GameWorldGame::GetMilitarySquares()
{
    return militarySquares;
}

void GameWorldGame::RecalcBQAroundPoint(const MapPoint pt)
{
    // Drumherum BQ neu berechnen, da diese sich ja jetzt hätten ändern können
    CalcAndSetBQ(pt, GAMECLIENT.GetPlayerID());
    for(unsigned char i = 0; i < 6; ++i)
        CalcAndSetBQ(GetNeighbour(pt, i), GAMECLIENT.GetPlayerID());
}

void GameWorldGame::RecalcBQAroundPointBig(const MapPoint pt)
{
    RecalcBQAroundPoint(pt);

    // 2. Außenschale
    for(unsigned i = 0; i < 12; ++i)
        CalcAndSetBQ(GetNeighbour2(pt, i), GAMECLIENT.GetPlayerID());
}

void GameWorldGame::SetFlag(const MapPoint pt, const unsigned char player, const unsigned char dis_dir)
{
    if(CalcBQ(pt, player, true, false) != BQ_FLAG)
        return;

    // Gucken, nicht, dass schon eine Flagge dasteht
    if(GetNO(pt)->GetType() != NOP_FLAG)
    {
        DestroyNO(pt, false);
        SetNO(pt, new noFlag(pt, player, dis_dir));

        RecalcBQAroundPointBig(pt);
    }
}

void GameWorldGame::DestroyFlag(const MapPoint pt)
{
    // Let's see if there is a flag
    if(GetNO(pt)->GetType() == NOP_FLAG)
    {
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
        GetSpecObj<noFlag>(pt)->DestroyAttachedBuilding();

        DestroyNO(pt, false);
        RecalcBQAroundPointBig(pt);
    }

    gi->GI_FlagDestroyed(pt);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  setzt den echten Straßen-Wert an der Stelle X, Y (berichtigt).
 *
 *  @author OLiver
 */
void GameWorldGame::SetRoad(const MapPoint pt, unsigned char dir, unsigned char type)
{
    RTTR_Assert(dir < 6);

    // Virtuelle Straße setzen
    SetVirtualRoad(pt, dir, type);
    ApplyRoad(pt, dir);

    if(gi)
        gi->GI_UpdateMinimap(pt);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  setzt den Straßen-Wert um den Punkt X, Y.
 *
 *  @author OLiver
 */
void GameWorldGame::SetPointRoad(const MapPoint pt, unsigned char dir, unsigned char type)
{
    RTTR_Assert(dir < 6);

    if(dir >= 3)
        SetRoad(pt, dir - 3, type);
    else
        SetRoad(GetNeighbour(pt, dir), dir, type);
}

void GameWorldGame::SetBuildingSite(const BuildingType type, const MapPoint pt, const unsigned char player)
{
    if(!GetPlayer(player).IsBuildingEnabled(type))
        return;

    // Gucken, ob das Gebäude hier überhaupt noch gebaut wrden kann
    const BuildingQuality bq = CalcBQ(pt, player, false, false);

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


void GameWorldGame::BuildRoad(const unsigned char playerid, const bool boat_road, const MapPoint start, const std::vector<unsigned char>& route)
{
    if(!GetSpecObj<noFlag>(start))
    {
        RemoveVisualRoad(start, route);
        // tell ai: road construction failed
        GAMECLIENT.SendAIEvent(new AIEvent::Direction(AIEvent::RoadConstructionFailed, start, route[0]), playerid);
        return;
    }
    // Falscher Spieler?
    else if(GetSpecObj<noFlag>(start)->GetPlayer() != playerid)
    {
        // Dann Weg nicht bauen und ggf. das visuelle wieder zurückbauen
        RemoveVisualRoad(start, route);
        // tell ai: road construction failed
        GAMECLIENT.SendAIEvent(new AIEvent::Direction(AIEvent::RoadConstructionFailed, start, route[0]), playerid);
        return;
    }

    // Gucken, ob der Weg überhaupt noch gebaut werden kann
    MapPoint curPt(start);
    RTTR_Assert(route.size() > 1);
    for(unsigned i = 0; i + 1 < route.size(); ++i)
    {
        curPt = GetNeighbour(curPt, route[i]);

        // Feld bebaubar und auf unserem Gebiet
        if(!RoadAvailable(boat_road, curPt, false) || !IsPlayerTerritory(curPt))
        {
            // Nein? Dann prüfen ob genau der gewünscht Weg schon da ist und ansonsten den visuellen wieder zurückbauen
            if (RoadAlreadyBuilt(boat_road, start, route))
            {
                //LOG.lprintf("duplicate road player %i at %i %i\n", playerid, start_x, start_y);
                return;
            }
            else
            {
                RemoveVisualRoad(start, route);
                // tell ai: road construction failed
                GAMECLIENT.SendAIEvent(new AIEvent::Direction(AIEvent::RoadConstructionFailed, start, route[0]), playerid);
                return;
            }
        }
    }

    curPt = GetNeighbour(curPt, route[route.size() - 1]);

    // Prüfen, ob am Ende auch eine Flagge steht oder eine gebaut werden kann
    if(GetNO(curPt)->GetGOT() == GOT_FLAG)
    {
        // Falscher Spieler?
        if(GetSpecObj<noFlag>(curPt)->GetPlayer() != playerid)
        {
            // Dann Weg nicht bauen und ggf. das visuelle wieder zurückbauen
            RemoveVisualRoad(start, route);
            // tell ai: road construction failed
            GAMECLIENT.SendAIEvent(new AIEvent::Direction(AIEvent::RoadConstructionFailed, start, route[0]), playerid);
            return;
        }
    }
    else
    {
        // Es ist keine Flagge dort, dann muss getestet werden, ob da wenigstens eine gebaut werden kann
        //Test ob wir evtl genau auf der Grenze sind (zählt zum eigenen Land kann aber nix gebaut werden egal was bq is!)
        if(GetNode(curPt).boundary_stones[0])
        {
            RemoveVisualRoad(start, route);
            // tell ai: road construction failed
            GAMECLIENT.SendAIEvent(new AIEvent::Direction(AIEvent::RoadConstructionFailed, start, route[0]), playerid);
            return;
        }
        // kann Flagge hier nicht gebaut werden?
        if(CalcBQ(curPt, playerid, true, false) != BQ_FLAG)
        {
            // Dann Weg nicht bauen und ggf. das visuelle wieder zurückbauen
            RemoveVisualRoad(start, route);
            // tell ai: road construction failed
            GAMECLIENT.SendAIEvent(new AIEvent::Direction(AIEvent::RoadConstructionFailed, start, route[0]), playerid);
            return;
        }

        // Abfragen, ob evtl ein Baum gepflanzt wurde, damit der nicht überschrieben wird
        if(GetNO(curPt)->GetType() == NOP_TREE)
        {
            // Dann Weg nicht bauen und ggf. das visuelle wieder zurückbauen
            RemoveVisualRoad(start, route);
            // tell ai: road construction failed
            GAMECLIENT.SendAIEvent(new AIEvent::Direction(AIEvent::RoadConstructionFailed, start, route[0]), playerid);
            return;
        }
        //keine Flagge bisher aber spricht auch nix gegen ne neue Flagge -> Flagge aufstellen!
        SetFlag(curPt, playerid, (route[route.size() - 1] + 3) % 6);
    }

    // Evtl Zierobjekte abreißen (Anfangspunkt)
    if(IsObjectionableForRoad(start))
        DestroyNO(start);

    MapPoint end(start);
    for(unsigned i = 0; i < route.size(); ++i)
    {
        SetPointRoad(end, route[i], boat_road ? (RoadSegment::RT_BOAT + 1) : (RoadSegment::RT_NORMAL + 1));
        CalcRoad(end, GAMECLIENT.GetPlayerID());
        end = GetNeighbour(end, route[i]);

        // Evtl Zierobjekte abreißen
        if(IsObjectionableForRoad(end))
            DestroyNO(end);
    }

    /*if(GetNO(start_x, start_y)->GetType() != NOP_FLAG)
        SetFlag(start_x, start_y, playerid, (route[route.size()-1]+3)%6);*/

    RoadSegment* rs = new RoadSegment(boat_road ? RoadSegment::RT_BOAT : RoadSegment::RT_NORMAL, 
                                      GetSpecObj<noFlag>(start), GetSpecObj<noFlag>(end), route);

    GetSpecObj<noFlag>(start)->routes[route.front()] = rs;
    GetSpecObj<noFlag>(end)->routes[(route.back() + 3) % 6] = rs;

    // Der Wirtschaft mitteilen, dass eine neue Straße gebaut wurde, damit sie alles Nötige macht
    GetPlayer(playerid).NewRoad(rs);
    // notify ai about the new road
    GAMECLIENT.SendAIEvent(new AIEvent::Direction(AIEvent::RoadConstructionComplete, start, route[0]), playerid);
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

void GameWorldGame::DestroyRoad(const MapPoint pt, const unsigned char dir)
{
    // TODO: Verzögerungsbugabfrage, kann später ggf. weg
    if(!GetSpecObj<noFlag>(pt))
        return;

    GetSpecObj<noFlag>(pt)->DestroyRoad(dir);
}

void GameWorldGame::UpgradeRoad(const MapPoint pt, const unsigned char dir)
{
    if(!GetSpecObj<noFlag>(pt))
        return;

    GetSpecObj<noFlag>(pt)->UpgradeRoad(dir);
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
    std::vector<bool> ownerChanged(region.height * region.width, false);

    std::vector<int> sizeChanges(GAMECLIENT.GetPlayerCount());
    // Daten von der TR kopieren in die richtige Karte, dabei zus. Grenzen korrigieren und Objekte zerstören, falls
    // das Land davon jemanden anders nun gehört
	
    const unsigned char ownerOfTriggerBld = GetNode(building.GetPos()).owner;
    const unsigned char newOwnerOfTriggerBld = region.GetOwner(building.GetPos().x, building.GetPos().y);
    const bool noAlliedBorderPush = GAMECLIENT.GetGGS().isEnabled(AddonId::NO_ALLIED_PUSH);

    for(int y = region.y1; y < region.y2; ++y)
    {
        for(int x = region.x1; x < region.x2; ++x)
        {
            const MapPoint curPt = MakeMapPoint(Point<int>(x, y));
            const unsigned char oldOwner = GetNode(curPt).owner;
            const unsigned char newOwner = region.GetOwner(x, y);

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
            SetOwner(curPt, newOwner);
            ownerChanged[region.GetIdx(x, y)] = true;
            if (newOwner != 0)
                sizeChanges[newOwner - 1]++;
            if (oldOwner != 0)
                sizeChanges[oldOwner - 1]--;

            // Event for map scripting
            if(newOwner != 0 && HasLua())
                GetLua().EventOccupied(newOwner - 1, curPt);
        }
    }

    for (unsigned i = 0; i < GAMECLIENT.GetPlayerCount(); ++i)
    {
        GetPlayer(i).ChangeStatisticValue(STAT_COUNTRY, sizeChanges[i]);

        // Negatives Wachstum per Post dem/der jeweiligen Landesherren/dame melden, nur bei neugebauten Gebäuden
        if (newBuilt && sizeChanges[i] < 0 && GAMECLIENT.GetPlayerID() == i)
            GAMECLIENT.SendPostMessage(new ImagePostMsgWithLocation(_("Lost land by this building"), PMC_MILITARY, building.GetPos(), building.GetBuildingType(), building.GetNation()));
    }

    for(int y = region.y1; y < region.y2; ++y)
    {
        for(int x = region.x1; x < region.x2; ++x)
        {
            MapPoint curPt = MakeMapPoint(Point<int>(x, y));
            if(GetNode(curPt).owner != 0)
            {
                /// Grenzsteine, die alleine "rausragen" und nicht mit einem richtigen Territorium verbunden sind, raushauen
                bool isPlayerTerritoryNear = false;
                for(unsigned d = 0; d < 6; ++d)
                {
                    if(IsPlayerTerritory(GetNeighbour(curPt, d)))
                    {
                        isPlayerTerritoryNear = true;
                        break;
                    }
                }

                // Wenn kein Land angrenzt, dann nicht nehmen
                if(!isPlayerTerritoryNear)
                    SetOwner(curPt, 0);
            }

            // Drumherum (da ja Grenzen mit einberechnet werden ins Gebiet, da darf trotzdem nichts stehen) alles vom Spieler zerstören
            // nicht das Militärgebäude oder dessen Flagge nochmal abreißen
            if(ownerChanged[region.GetIdx(x, y)])
            {
                for(unsigned char i = 0; i < 6; ++i)
                {
                    MapPoint neighbourPt = GetNeighbour(curPt, i);

                    DestroyPlayerRests(neighbourPt, GetNode(curPt).owner, &building, false);

                    // BQ neu berechnen
                    CalcAndSetBQ(neighbourPt, GAMECLIENT.GetPlayerID());
                    // ggf den noch darüber, falls es eine Flagge war (kann ja ein Gebäude entstehen)
                    if(GetNeighbourNode(neighbourPt, 1).bq != BQ_NOTHING)
                        CalcAndSetBQ(GetNeighbour(neighbourPt, 1), GAMECLIENT.GetPlayerID());
                }

                if(gi)
                    gi->GI_UpdateMinimap(curPt);
            }
        }
    }

    RecalcBorderStones(region);


    // Sichtbarkeiten berechnen

    // Wurde es zerstört, müssen die Sichtbarkeiten entsprechend neu berechnet werden, ansonsten reicht es auch
    // sie einfach auf sichtbar zu setzen
    const unsigned visualRadius = militaryRadius + VISUALRANGE_MILITARY;
    if(destroyed)
        RecalcVisibilitiesAroundPoint(building.GetPos(), visualRadius, building.GetPlayer(), destroyed ? &building : NULL);
    else
        SetVisibilitiesAroundPoint(building.GetPos(), visualRadius, building.GetPlayer());
}

#define PREVENT_BORDER_STONE_BLOCKING

void GameWorldGame::RecalcBorderStones(const TerritoryRegion& region)
{
    // Add a bit extra space as this influences also border stones around the region
    static const int EXTRA_SPACE = 3;

    const int x1 = region.x1 - EXTRA_SPACE;
    const int x2 = region.x2 + EXTRA_SPACE;
    const int y1 = region.y1 - EXTRA_SPACE;
    const int y2 = region.y2 + EXTRA_SPACE;
#ifdef PREVENT_BORDER_STONE_BLOCKING
    const int width = x2 - x1;
    // In diesem Array merken, wie wieviele Nachbarn ein Grenzstein hat
    std::vector<unsigned> neighbors(width * (y2 - y1), 0);
#endif


    for(int y = y1; y < y2; ++y)
    {
        for(int x = x1; x < x2; ++x)
        {
            // Korrigierte X-Koordinaten
            const MapPoint curPt = MakeMapPoint(Point<int>(x, y));
            const unsigned char owner = GetNode(curPt).owner;
            MapNode::BoundaryStones& boundaryStones = GetBoundaryStones(curPt);

            // Grenzstein direkt auf diesem Punkt?
            if(owner && IsBorderNode(curPt, owner))
            {
                boundaryStones[0] = owner;

                // Grenzsteine prüfen auf den Zwischenstücken in die 3 Richtungen nach unten und nach rechts
                for(unsigned i = 0; i < 3; ++i)
                {
                    if(IsBorderNode(GetNeighbour(curPt, 3 + i), owner))
                        boundaryStones[i + 1] = owner;
                    else
                        boundaryStones[i + 1] = 0;

                }

#ifdef PREVENT_BORDER_STONE_BLOCKING
                // Zählen
                for(unsigned i = 0; i < 6; ++i)
                {
                    neighbors[(y - y1)*width + (x - x1)] = 0;
                    if(GetNeighbourNode(curPt, i).boundary_stones[0] == owner)
                        ++neighbors[(y - y1)*width + (x - x1)];
                }
#endif
            } else
            {
                // Kein Grenzstein --> etwaige vorherige Grenzsteine löschen
                for(unsigned i = 0; i < 4; ++i)
                    boundaryStones[i] = 0;

                //for(unsigned i = 0;i<3;++i)
                //  GetNeighbourNode(x, y, 3+i).boundary_stones[i+1] = 0;
            }
        }
    }

#ifdef PREVENT_BORDER_STONE_BLOCKING
    // Nochmal durchgehen und bei Grenzsteinen mit mehr als 3 Nachbarn welche löschen
    // da sich sonst gelegentlich solche "Klötzchen" bilden können
    for(int y = y1; y < y2; ++y)
    {
        for(int x = x1; x < x2; ++x)
        {
            const MapPoint curPt = MakeMapPoint(Point<int>(x, y));

            // Steht auch hier ein Grenzstein?
            unsigned char owner = GetNode(curPt).boundary_stones[0];
            if(!owner)
                continue;

            if(neighbors[(y - y1)*width + (x - x1)] < 3)
                continue;

            for(unsigned dir = 0; dir < 3 && neighbors[(y - y1)*width + (x - x1)] > 2; ++dir)
            {
                // Da ein Grenzstein vom selben Besitzer?
                MapNode::BoundaryStones& nbBoundStones = GetBoundaryStones(GetNeighbour(curPt, dir + 3));

                if(nbBoundStones[0] != owner)
                    continue;

                Point<int> pa = ::GetNeighbour(Point<int>(x, y), Direction(dir + 3));
                if(pa.x < x1 || pa.x >= x2 || pa.y < y1 || pa.y >= y2)
                    continue;
                // Hat der auch zu viele Nachbarn?
                if(neighbors[(pa.y - y1)*width + (pa.x - x1)] > 2)
                {
                    // Dann löschen wir hier einfach die Verbindung
                    nbBoundStones[dir + 1] = 0;
                    --neighbors[(y - y1)*width + (x - x1)];
                    --neighbors[(pa.y - y1)*width + (pa.x - x1)];
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
    for(int y = region.y1; y < region.y2; ++y)
    {
        for(int x = region.x1; x < region.x2; ++x)
        {
            MapPoint curPt = MakeMapPoint(Point<int>(x, y));
            if(GetNode(curPt).owner != region.GetOwner(x, y))
            {
                // if gameobjective is 75% ai can ignore water/snow/lava/swamp terrain (because it wouldnt help win the game)
                if(GAMECLIENT.GetGGS().game_objective == GlobalGameSettings::GO_CONQUER3_4)
                    return true;
                TerrainType t1 = GetNode(curPt).t1, t2 = GetNode(curPt).t2;
                if(TerrainData::IsUseable(t1) && TerrainData::IsUseable(t2))
                    return true;
                //also check neighboring nodes for their terrain since border will still count as player territory but not allow any buildings !
                for(int dir = 0; dir < 6; dir++)
                {
                    t1 = GetNeighbourNode(curPt, dir).t1;
                    t2 = GetNeighbourNode(curPt, dir).t2;
                    if(TerrainData::IsUseable(t1) || TerrainData::IsUseable(t2))
                        return true;
                }
            }
        }
    }
    return false;
}

TerritoryRegion GameWorldGame::CreateTerritoryRegion(const noBaseBuilding& building, const unsigned short radius, const bool destroyed) const
{
    const MapPoint bldPos = building.GetPos();
    sortedMilitaryBlds buildings = LookForMilitaryBuildings(bldPos, 3);

    // Koordinaten erzeugen für TerritoryRegion
    int x1 = int(bldPos.x) - radius;
    int y1 = int(bldPos.y) - radius;
    int x2 = int(bldPos.x) + radius + 1;
    int y2 = int(bldPos.y) + radius + 1;


    TerritoryRegion region(x1, y1, x2, y2, *this);

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

void GameWorldGame::DestroyPlayerRests(const MapPoint pt, const unsigned char new_player, const noBaseBuilding* exception, bool allowdestructionofmilbuildings)
{
    noBase* no = GetNO(pt);

    // Flaggen, Gebäude und Baustellen zerstören, aber keine übernommenen und nicht die Ausahme oder dessen Flagge!
    if((no->GetType() == NOP_FLAG || no->GetType() == NOP_BUILDING || no->GetType() == NOP_BUILDINGSITE) && exception != no)
    {
        // Wurde das Objekt auch nicht vom Gegner übernommen?
        if(static_cast<noRoadNode*>(no)->GetPlayer() + 1 != new_player)
        {
			//maybe buildings that push territory should not be destroyed right now?- can happen with improved alliances addon or in rare cases even without the addon so allow those buildings & their flag to survive.
			if(!allowdestructionofmilbuildings)
			{
				if(no->GetGOT() == GOT_NOB_HQ || no->GetGOT() == GOT_NOB_HARBORBUILDING || (no->GetGOT() == GOT_NOB_MILITARY && !GetSpecObj<nobMilitary>(pt)->IsNewBuilt()) || (no->GetType()==NOP_BUILDINGSITE && GetSpecObj<noBuildingSite>(pt)->IsHarborBuildingSiteFromSea()))
				{
					//LOG.lprintf("DestroyPlayerRests of hq, military, harbor or colony-harbor in construction stopped at x, %i y, %i type, %i \n", x, y, no->GetType());
					return;
				}
				//flag of such a building?				
				if(no->GetType()==NOP_FLAG)
				{
					noBase* no2=GetNO(GetNeighbour(pt, 1));
					if(no2->GetGOT() == GOT_NOB_HQ || no2->GetGOT() == GOT_NOB_HARBORBUILDING || (no2->GetGOT() == GOT_NOB_MILITARY && !GetSpecObj<nobMilitary>(GetNeighbour(pt, 1))->IsNewBuilt()) || (no2->GetType()==NOP_BUILDINGSITE && GetSpecObj<noBuildingSite>(GetNeighbour(pt, 1))->IsHarborBuildingSiteFromSea()))
					{
						//LOG.lprintf("DestroyPlayerRests of a flag of a hq, military, harbor or colony-harbor in construction stopped at x, %i y, %i type, %i \n", GetXA(x, y, 1), GetYA(x, y, 1), no2->GetType());
						return;
					}
				}
			}				
            // vorher Bescheid sagen
            if(no->GetType() == NOP_FLAG && no != (exception ? exception->GetFlag() : NULL))
                static_cast<noFlag*>(no)->DestroyAttachedBuilding();

            no->Destroy();
            delete no;
            return;
        }
    }

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
    noBase::BlockingManner bm = GetNO(pt)->GetBM();
    if(bm != noBase::BM_NOTBLOCKING && bm != noBase::BM_TREE && bm != noBase::BM_FLAG)
        return false;

    // Terrain untersuchen
    unsigned char good_terrains = 0;
    for(unsigned char i = 0; i < 6; ++i)
    {
        BuildingQuality bq = TerrainData::GetBuildingQuality(GetTerrainAround(pt, i));
        if(bq == BQ_CASTLE || bq == BQ_MINE || bq == BQ_FLAG) ++good_terrains;
        else if(bq == BQ_DANGER) return false; // in die Nähe von Lava usw. dürfen die Figuren gar nich kommen!
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
    // Verzögerungsbug-Abfrage:
    // Existiert das angegriffenen Gebäude überhaupt noch?
    if(GetNO(pt)->GetGOT() != GOT_NOB_MILITARY && GetNO(pt)->GetGOT() != GOT_NOB_HQ
            && GetNO(pt)->GetGOT() != GOT_NOB_HARBORBUILDING)
        return;

    // Auch noch ein Gebäude von einem Feind (nicht inzwischen eingenommen)?
    if(!GetPlayer(player_attacker).IsPlayerAttackable(GetSpecObj<noBuilding>(pt)->GetPlayer()))
        return;

    // Prüfen, ob der angreifende Spieler das Gebäude überhaupt sieht (Cheatvorsorge)
    if(CalcWithAllyVisiblity(pt, player_attacker) != VIS_VISIBLE)
        return;

    // Ist das angegriffenne ein normales Gebäude?
    nobBaseMilitary* attacked_building = GetSpecObj<nobBaseMilitary>(pt);
    if(attacked_building->GetBuildingType() >= BLD_BARRACKS && attacked_building->GetBuildingType() <= BLD_FORTRESS)
    {
        // Wird es gerade eingenommen?
        if(static_cast<nobMilitary*>(attacked_building)->IsCaptured())
            // Dann darf es nicht angegriffen werden
            return;
        if (static_cast<nobMilitary*>(attacked_building)->IsNewBuilt())
            return;
    }

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
            ((static_cast<nobMilitary*>(*it)->GetTroopsCount() - 1) * GetPlayer(player_attacker).militarySettings_[3] / MILITARY_SETTINGS_SCALE[3]) : 0;

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
        SortedTroops& troops = static_cast<nobMilitary*>(*it)->troops;
        if(strong_soldiers){
            // Strong soldiers first
            for(SortedTroops::reverse_iterator it2 = troops.rbegin();
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
            for(SortedTroops::iterator it2 = troops.begin();
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
        LOG.lprintfS("GameWorldGame::Attack: WARNING: Attack failed. No Soldiers available!\n");*/
}

void  GameWorldGame::AttackViaSea(const unsigned char player_attacker, const MapPoint pt, const unsigned short soldiers_count, const bool strong_soldiers)
{
    //sea attack abgeschaltet per addon?
    if(GAMECLIENT.GetGGS().getSelection(AddonId::SEA_ATTACK) == 2)
        return;
    // Verzögerungsbug-Abfrage:
    // Existiert das angegriffenen Gebäude überhaupt noch?
    if(GetNO(pt)->GetGOT() != GOT_NOB_MILITARY && GetNO(pt)->GetGOT() != GOT_NOB_HQ
            && GetNO(pt)->GetGOT() != GOT_NOB_HARBORBUILDING)
        return;

    // Auch noch ein Gebäude von einem Feind (nicht inzwischen eingenommen)?
    if(!GetPlayer(player_attacker).IsPlayerAttackable(GetSpecObj<noBuilding>(pt)->GetPlayer()))
        return;

    // Prüfen, ob der angreifende Spieler das Gebäude überhaupt sieht (Cheatvorsorge)
    if(CalcWithAllyVisiblity(pt, player_attacker) != VIS_VISIBLE)
        return;

    // Ist das angegriffenne ein normales Gebäude?
    nobBaseMilitary* attacked_building = GetSpecObj<nobBaseMilitary>(pt);
    if(attacked_building->GetBuildingType() >= BLD_BARRACKS && attacked_building->GetBuildingType() <= BLD_FORTRESS)
    {
        // Wird es gerade eingenommen?
        if(static_cast<nobMilitary*>(attacked_building)->IsCaptured())
            // Dann darf es nicht angegriffen werden
            return;
        if (static_cast<nobMilitary*>(attacked_building)->IsNewBuilt())
            return;
    }

    // Verfügbare Soldaten herausfinden
    std::vector<GameWorldBase::PotentialSeaAttacker> attackers = GetAvailableSoldiersForSeaAttack(player_attacker, pt);

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

void GameWorldGame::AddCatapultStone(CatapultStone* cs)
{
    RTTR_Assert(!helpers::contains(catapult_stones, cs));
    catapult_stones.push_back(cs);
}

void GameWorldGame::RemoveCatapultStone(CatapultStone* cs)
{
     RTTR_Assert(helpers::contains(catapult_stones, cs));
     catapult_stones.remove(cs);
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
                DestroyNO(pt);
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
                DestroyNO(pt);
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
    noBase::BlockingManner bm = GetNO(pt)->GetBM();
    return !(bm != noBase::BM_NOTBLOCKING && bm != noBase::BM_TREE && bm != noBase::BM_FLAG);
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

    ///// Auf eigenem Terrain --> sichtbar
    //if(GetNode(x, y).owner == player+1)
    //  visible = true;
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
                nofScout_Free* scout = dynamic_cast<nofScout_Free*>(*it);
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
                nofActiveSoldier* soldier = dynamic_cast<nofActiveSoldier*>(*it);
                if(soldier->GetPos() == pt && soldier->GetPlayer() == player)
                    return true;
            }
            // Kämpfe (wo auch Soldaten drin sind)
            else if((*it)->GetGOT() == GOT_FIGHTING)
            {
                // Prüfen, ob da ein Soldat vom angegebenen Spieler dabei ist
                if(dynamic_cast<noFighting*>(*it)->IsSoldierOfPlayer(player))
                    return true;
            }
        }

        // Schiffe?

        if((*it)->GetGOT() == GOT_SHIP)
        {
            noShip* ship = dynamic_cast<noShip*>(*it);
            if(distance <= ship->GetVisualRange())
            {
                if(ship->GetPos() == pt && ship->GetPlayer() == player)
                    return true;
            }
        }

    }

    return false;
}

void GameWorldGame::RecalcVisibility(const MapPoint pt, const unsigned char player, const noBaseBuilding* const exception)
{
    ///// Bei völlig ausgeschalteten Nebel muss nur das erste Mal alles auf sichtbar gesetzt werden
    //if(GAMECLIENT.GetGGS().exploration == GlobalGameSettings::EXP_DISABLED && !update_terrain)
    //  GetNode(x, y).fow[player].visibility = VIS_VISIBLE;
    //else if(GAMECLIENT.GetGGS().exploration == GlobalGameSettings::EXP_DISABLED && update_terrain)
    //  return;

    /// Zustand davor merken
    Visibility visibility_before = GetNode(pt).fow[player].visibility;

    /// Herausfinden, ob vollständig sichtbar
    bool visible = IsPointCompletelyVisible(pt, player, exception);

    // Vollständig sichtbar --> vollständig sichtbar logischerweise
    if(visible)
    {
        if (visibility_before != VIS_VISIBLE && HasLua())
            GetLua().EventExplored(player, pt);
        SetVisibility(pt, player, VIS_VISIBLE, GAMECLIENT.GetGFNumber());
    }
    else
    {
        // nicht mehr sichtbar
        // Je nach vorherigen Zustand und Einstellung entscheiden
        switch(GAMECLIENT.GetGGS().exploration)
        {
            case GlobalGameSettings::EXP_DISABLED:
            case GlobalGameSettings::EXP_CLASSIC:
                // einmal sichtbare Bereiche bleiben erhalten
                // nichts zu tun
                break;
            case GlobalGameSettings::EXP_FOGOFWAR:
            case GlobalGameSettings::EXP_FOGOFWARE_EXPLORED:
                // wenn es mal sichtbar war, nun im Nebel des Krieges
                if(visibility_before == VIS_VISIBLE)
                {
                    SetVisibility(pt, player, VIS_FOW, GAMECLIENT.GetGFNumber());
                }
                break;
            default:
                throw std::logic_error("Invalid exploration value");
        }

    }

    // Minimap Bescheid sagen
    if(gi && visibility_before != GetNode(pt).fow[player].visibility)
        gi->GI_UpdateMinimap(pt);

    // Lokaler Spieler oder Verbündeter (wenn Team-Sicht an ist)? --> Terrain updaten
    if(player == GAMECLIENT.GetPlayerID() ||
            (GAMECLIENT.GetGGS().team_view && GAMECLIENT.GetLocalPlayer().IsAlly(player)))
        VisibilityChanged(pt);
}

void GameWorldGame::MakeVisible(const MapPoint pt, const unsigned char player)
{
    Visibility visibility_before = GetNode(pt).fow[player].visibility;
    SetVisibility(pt, player, VIS_VISIBLE, GAMECLIENT.GetGFNumber());

    if (visibility_before != VIS_VISIBLE && HasLua())
        GetLua().EventExplored(player, pt);

    // Minimap Bescheid sagen
    if(gi && visibility_before != GetNode(pt).fow[player].visibility)
        gi->GI_UpdateMinimap(pt);

    // Lokaler Spieler oder Verbündeter (wenn Team-Sicht an ist)? --> Terrain updaten
    if(player == GAMECLIENT.GetPlayerID() ||
            (GAMECLIENT.GetGGS().team_view && GAMECLIENT.GetLocalPlayer().IsAlly(player)))
        VisibilityChanged(pt);
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
        Visibility old_vis = CalcWithAllyVisiblity(tt, player);
        unsigned char old_owner = GetNode(tt).fow[player].owner;
        MakeVisible(tt, player);
        // Neues feindliches Gebiet entdeckt?
        // Muss vorher undaufgedeckt oder FOW gewesen sein, aber in dem Fall darf dort vorher noch kein
        // Territorium entdeckt worden sein
        unsigned char current_owner = GetNode(tt).owner;
        if(current_owner && (old_vis == VIS_INVISIBLE ||
                             (old_vis == VIS_FOW && old_owner != current_owner)))
        {
            if(GAMECLIENT.GetPlayer(player).IsPlayerAttackable(current_owner - 1) && enemy_territory)
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
        Visibility old_vis = CalcWithAllyVisiblity(tt, player);
        unsigned char old_owner = GetNode(tt).fow[player].owner;
        MakeVisible(tt, player);
        // Neues feindliches Gebiet entdeckt?
        // Muss vorher undaufgedeckt oder FOW gewesen sein, aber in dem Fall darf dort vorher noch kein
        // Territorium entdeckt worden sein
        unsigned char current_owner = GetNode(tt).owner;
        if(current_owner && (old_vis == VIS_INVISIBLE ||
                             (old_vis == VIS_FOW && old_owner != current_owner)))
        {
            if(GAMECLIENT.GetPlayer(player).IsPlayerAttackable(current_owner - 1) && enemy_territory)
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

/// Stellt fest, ob auf diesem Punkt ein Grenzstein steht (ob das Grenzgebiet ist)
bool GameWorldGame::IsBorderNode(const MapPoint pt, const unsigned char player) const
{
    // Wenn ich Besitzer des Punktes bin, dieser mir aber nicht gehört
    return (GetNode(pt).owner == player && !IsPlayerTerritory(pt));
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konvertiert Ressourcen zwischen Typen hin und her oder löscht sie.
 *  Für Spiele ohne Gold.
 *
 *  @author Divan
 */
void GameWorldGame::ConvertMineResourceTypes(unsigned char from, unsigned char to)
{
    // to == 0xFF heißt löschen
    // in Map-Resource-Koordinaten konvertieren
    from = RESOURCES_MINE_TO_MAP[from];
    to = ((to != 0xFF) ? RESOURCES_MINE_TO_MAP[to] : 0xFF);

    //LOG.lprintf("Convert map resources from %i to %i\n", from, to);
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
bool GameWorldGame::FoundColony(const unsigned harbor_point, const unsigned char player, const unsigned short sea_id)
{
    // Ist es hier überhaupt noch möglich, eine Kolonie zu gründen?
    if(!IsHarborPointFree(harbor_point, player, sea_id))
        return false;

    MapPoint pos(GetHarborPoint(harbor_point));
    DestroyNO(pos, false);

    // Hafenbaustelle errichten
    noBuildingSite* bs = new noBuildingSite(pos, player);
    SetNO(pos, bs);
    AddHarborBuildingSiteFromSea(bs);

    gi->GI_UpdateMinimap(pos);

    RecalcTerritory(*bs, false, true);

    // BQ neu berechnen (evtl durch RecalcTerritory noch nicht geschehen)
    RecalcBQAroundPointBig(pos);
    //notify the ai
    GAMECLIENT.SendAIEvent(new AIEvent::Location(AIEvent::NewColonyFounded, pos), player);

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

std::vector<unsigned> GameWorldGame::GetHarborPointsWithinReach(const unsigned hbId, const unsigned seaId) const
{
    std::vector<unsigned> hps;
    for(unsigned i = 1; i <= GetHarborPointCount(); ++i)
    {
        if(i == hbId || !IsAtThisSea(i, seaId))
            continue;
        unsigned dist = CalcHarborDistance(hbId, i);
        if(dist == 0xffffffff)
            continue;

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

/// Create Trade graphs
void GameWorldGame::CreateTradeGraphs()
{
    // Only if trade is enabled
    if(!GAMECLIENT.GetGGS().isEnabled(AddonId::TRADE))
        return;

    TradePathCache::inst().Clear();
}
