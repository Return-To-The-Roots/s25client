// $Id: GameWorldGame.cpp 9601 2015-02-07 11:09:14Z marcus $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#include "defines.h"
#include "GameWorld.h"

#include "GameClient.h"
#include "GameClientPlayer.h"
#include "Random.h"
#include "SoundManager.h"
#include "SerializedGameData.h"

#include "figures/nofCarrier.h"
#include "nodeObjs/noEnvObject.h"
#include "nodeObjs/noStaticObject.h"
#include "nodeObjs/noGranite.h"
#include "nodeObjs/noTree.h"
#include "nodeObjs/noFlag.h"
#include "buildings/nobHQ.h"
#include "nodeObjs/noFire.h"
#include "buildings/nobUsual.h"
#include "buildings/noBuildingSite.h"
#include "Ware.h"
#include "gameData/MilitaryConsts.h"
#include "TerritoryRegion.h"
#include "buildings/nobMilitary.h"
#include "figures/nofAttacker.h"
#include "figures/nofPassiveSoldier.h"
#include "nodeObjs/noAnimal.h"
#include "nodeObjs/noFighting.h"
#include "CatapultStone.h"
#include "MapGeometry.h"
#include "figures/nofScout_Free.h"
#include "nodeObjs/noShip.h"

#include "WindowManager.h"
#include "GameInterface.h"
#include "drivers/VideoDriverWrapper.h"
#include "helpers/containerUtils.h"

#include <algorithm>
#include <stdexcept>

GameWorldGame::~GameWorldGame()
{
    for(unsigned i = 0; i < tgs.size(); ++i)
        delete tgs[i];
}

void GameWorldGame::RecalcBQAroundPoint(const MapPoint pt)
{
    // Drumherum BQ neu berechnen, da diese sich ja jetzt hätten ändern können
    GetNode(pt).bq = CalcBQ(pt, GAMECLIENT.GetPlayerID());
    for(unsigned char i = 0; i < 6; ++i)
        GetNode(GetNeighbour(pt, i)).bq = CalcBQ(GetNeighbour(pt, i), GAMECLIENT.GetPlayerID());
}

void GameWorldGame::RecalcBQAroundPointBig(const MapPoint pt)
{
    RecalcBQAroundPoint(pt);

    // 2. Außenschale
    for(unsigned i = 0; i < 12; ++i)
        GetNode(GetNeighbour2(pt, i)).bq = CalcBQ(GetNeighbour2(pt, i), GAMECLIENT.GetPlayerID());
}

void GameWorldGame::SetFlag(const MapPoint pt, const unsigned char player, const unsigned char dis_dir)
{
    // TODO: Verzögerungsbugabfrage, kann später ggf. weg
    if(CalcBQ(pt, player, true, false) != BQ_FLAG)
        return;
    //
    //// Abfragen, ob schon eine Flagge in der Nähe ist (keine Mini-1-Wege)
    //for(unsigned char i = 0;i<6;++i)
    //{
    //  if(GetNO(GetXA(x, y, i), GetYA(x, y, i))->GetType() == NOP_FLAG)
    //      return;
    //}

    //// TODO: Verzögerungsbugabfrage, kann später ggf. weg
    //// Abfragen, ob evtl ein Baum gepflanzt wurde, damit der nicht überschrieben wird
    //if(GetNO(x, y)->GetType() == NOP_TREE)
    //  return;

    // Gucken, nicht, dass schon eine Flagge dasteht
    if(GetNO(pt)->GetType() != NOP_FLAG)
    {
        noBase* no = GetSpecObj<noBase>(pt);
        if(no)
        {
            no->Destroy();
            delete no;
        }

        SetNO(NULL, pt);
        SetNO(new noFlag(pt, player, dis_dir), pt);

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
                // Abort the whole thing
                return;
        }


        // Demolish, also the building
        noFlag* flag = GetSpecObj<noFlag>(pt);

        SetNO(NULL, pt);
        flag->DestroyAttachedBuilding();
        flag->Destroy();
        delete flag;

        RecalcBQAroundPointBig(pt);
    }

    gi->GI_FlagDestroyed(pt);
}



///////////////////////////////////////////////////////////////////////////////
/**
 *  setzt den echten Straßen-Wert an der Stelle X, Y (berichtigt).
 *
 * Bit 0-6 jeweils 2 Bit für jede Richtung jeweils der Typ, Bit 7
 *  @author OLiver
 */
void GameWorldGame::SetRoad(const MapPoint pt, unsigned char dir, unsigned char type)
{
    assert(dir < 6);

    // Virtuelle Straße setzen
    SetVirtualRoad(pt, dir, type);

    unsigned pos = GetIdx(pt);


    // Flag nullen wenn nur noch das real-flag da ist oder es setzen
    if(!nodes[pos].roads[dir])
        nodes[pos].roads_real[dir] = false;
    else
        nodes[pos].roads_real[dir] = true;

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
    assert(dir < 6);

    if(dir >= 3)
        SetRoad(pt, dir - 3, type);
    else
        SetRoad(GetNeighbour(pt, dir), dir, type);
}



void GameWorldGame::AddFigure(noBase* fig, const MapPoint pt)
{
    if(!fig)
        return;

    std::list<noBase*>& figures = GetNode(pt).figures;
    assert(!helpers::contains(figures, fig));
    figures.push_back(fig);

#ifndef NDEBUG
    for(unsigned char i = 0; i < 6; ++i)
    {
        MapPoint nb = GetNeighbour(pt, i);

        const std::list<noBase*>& figures = GetNode(nb).figures;
        if(helpers::contains(figures, fig))
            throw std::runtime_error("Added figure that is in surrounding?");
    }
#endif // NDEBUG

    //if(fig->GetDir() == 1 || fig->GetDir() == 2)
    //  figures[y*width+x].push_front(fig);
    //else
    //  figures[y*width+x].push_back(fig);
}

void GameWorldGame::RemoveFigure(noBase* fig, const MapPoint pt)
{
    GetNode(pt).figures.remove(fig);
}



void GameWorldGame::SetBuildingSite(const BuildingType type, const MapPoint pt, const unsigned char player)
{
    // Gucken, ob das Gebäude hier überhaupt noch gebaut wrden kann
    BuildingQuality bq = CalcBQ(pt, player, false, false);

    switch(BUILDING_SIZE[type])
    {
        case BQ_HUT: if(!((bq >= BQ_HUT && bq <= BQ_CASTLE) || bq == BQ_HARBOR)) return; break;
        case BQ_HOUSE: if(!((bq >= BQ_HOUSE && bq <= BQ_CASTLE) || bq == BQ_HARBOR)) return; break;
        case BQ_CASTLE: if(!( bq == BQ_CASTLE || bq == BQ_HARBOR)) return; break;
        case BQ_HARBOR: if(bq != BQ_HARBOR) return; break;
        case BQ_MINE: if(bq != BQ_MINE) return; break;
        default: break;
    }

    // TODO: Verzögerungsbugabfrage, kann später ggf. weg
    // Wenn das ein Militärgebäude ist und andere Militärgebäude bereits in der Nähe sind, darf dieses nicht gebaut werden
    if(type >= BLD_BARRACKS && type <= BLD_FORTRESS)
    {
        if(IsMilitaryBuildingNearNode(pt, player))
            return;
    }

    // Prüfen ob Katapult und ob Katapult erlaubt ist
    if (type == BLD_CATAPULT && !GetPlayer(player)->CanBuildCatapult())
        return;

    // ggf. vorherige Objekte löschen
    noBase* no = GetSpecObj<noBase>(pt);
    if(no)
    {
        no->Destroy();
        delete no;
    }

    // Baustelle setzen
    SetNO(new noBuildingSite(type, pt, player), pt);
    gi->GI_UpdateMinimap(pt);

    // Bauplätze drumrum neu berechnen
    RecalcBQAroundPointBig(pt);
}

void GameWorldGame::DestroyBuilding(const MapPoint pt, const unsigned char player)
{
    // Steht da auch ein Gebäude oder eine Baustelle, nicht dass wir aus Verzögerung Feuer abreißen wollen, das geht schief
    if(GetNO(pt)->GetType() == NOP_BUILDING ||
            GetNO(pt)->GetType() == NOP_BUILDINGSITE)
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
                // Nein, darf nicht abgerissen werden
                return;
        }


        nbb->Destroy();
        delete nbb;
        // Bauplätze drumrum neu berechnen
        RecalcBQAroundPointBig(pt);
    }
}


void GameWorldGame::BuildRoad(const unsigned char playerid, const bool boat_road, 
                              const MapPoint start, const std::vector<unsigned char>& route)
{
    // TODO: Verzögerungsbugabfrage, kann später ggf. weg
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

    // TODO: Verzögerungsbugabfrage, kann später ggf. weg
    // Gucken, ob der Weg überhaupt noch gebaut werden kann
    MapPoint test(start);
    assert(route.size() > 1);
    for(unsigned i = 0; i + 1 < route.size(); ++i)
    {
        test = GetNeighbour(test, route[i]);

        // Feld bebaubar und auf unserem Gebiet
        if(!RoadAvailable(boat_road, test, i, false) || !IsPlayerTerritory(test))
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

    test = GetNeighbour(test, route[route.size() - 1]);

    // Prüfen, ob am Ende auch eine Flagge steht oder eine gebaut werden kann
    if(GetNO(test)->GetGOT() == GOT_FLAG)
    {
        // Falscher Spieler?
        if(GetSpecObj<noFlag>(test)->GetPlayer() != playerid)
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
        if(GetNode(test).boundary_stones[0])
        {
            RemoveVisualRoad(start, route);
            // tell ai: road construction failed
            GAMECLIENT.SendAIEvent(new AIEvent::Direction(AIEvent::RoadConstructionFailed, start, route[0]), playerid);
            return;
        }
        // TODO: Verzögerungsbugabfrage, kann später ggf. weg
        // kann Flagge hier nicht gebaut werden?
        if(CalcBQ(test, playerid, true, false) != BQ_FLAG)
        {
            // Dann Weg nicht bauen und ggf. das visuelle wieder zurückbauen
            RemoveVisualRoad(start, route);
            // tell ai: road construction failed
            GAMECLIENT.SendAIEvent(new AIEvent::Direction(AIEvent::RoadConstructionFailed, start, route[0]), playerid);
            return;
        }

        // TODO: Verzögerungsbugabfrage, kann später ggf. weg
        // Abfragen, ob evtl ein Baum gepflanzt wurde, damit der nicht überschrieben wird
        if(GetNO(test)->GetType() == NOP_TREE)
        {
            // Dann Weg nicht bauen und ggf. das visuelle wieder zurückbauen
            RemoveVisualRoad(start, route);
            // tell ai: road construction failed
            GAMECLIENT.SendAIEvent(new AIEvent::Direction(AIEvent::RoadConstructionFailed, start, route[0]), playerid);
            return;
        }
        //keine Flagge bisher aber spricht auch nix gegen ne neue Flagge -> Flagge aufstellen!
        SetFlag(test, playerid, (route[route.size() - 1] + 3) % 6);
    }

    // Evtl Zierobjekte abreißen (Anfangspunkt)
    if(IsObjectionableForRoad(start))
    {
        noBase* obj = GetSpecObj<noBase>(start);
        obj->Destroy();
        delete obj;
        SetNO(0, start);
    }

    MapPoint end(start);
    for(unsigned i = 0; i < route.size(); ++i)
    {
        SetPointRoad(end, route[i], boat_road ? (RoadSegment::RT_BOAT + 1) : (RoadSegment::RT_NORMAL + 1));
        CalcRoad(end, GAMECLIENT.GetPlayerID());
        end = GetNeighbour(end, route[i]);

        // Evtl Zierobjekte abreißen
        if(IsObjectionableForRoad(end))
        {
            noBase* obj = GetSpecObj<noBase>(end);
            obj->Destroy();
            delete obj;
            SetNO(0, end);
        }
    }

    /*if(GetNO(start_x, start_y)->GetType() != NOP_FLAG)
        SetFlag(start_x, start_y, playerid, (route[route.size()-1]+3)%6);*/

    RoadSegment* rs = new RoadSegment(boat_road ? RoadSegment::RT_BOAT : RoadSegment::RT_NORMAL, 
                                      GetSpecObj<noFlag>(start), GetSpecObj<noFlag>(end), route);

    GetSpecObj<noFlag>(start)->routes[route.front()] = rs;
    GetSpecObj<noFlag>(end)->routes[(route.back() + 3) % 6] = rs;

    // Der Wirtschaft mitteilen, dass eine neue Straße gebaut wurde, damit sie alles Näcige macht
    GetPlayer(playerid)->NewRoad(rs);
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

void GameWorldGame::RecalcTerritory(const noBaseBuilding* const building, const unsigned short radius, const bool destroyed, const bool newBuilt)
{
	unsigned char owneroftriggerbuilding = GetNode(building->GetPos()).owner;
	unsigned char new_owner_of_trigger_building;

    // alle Militärgebäude in der Nähe abgrasen
	nobBaseMilitarySet buildings = LookForMilitaryBuildings(building->GetPos(), 3);

    // Radius der noch draufaddiert wird auf den eigentlich ausreichenden Bereich, für das Eliminieren von
    // herausragenden Landesteilen und damit Grenzsteinen
    const int ADD_RADIUS = 2;

    // Koordinaten erzeugen für TerritoryRegion
    int x1 = int(building->GetX()) - (radius + ADD_RADIUS);
    int y1 = int(building->GetY()) - (radius + ADD_RADIUS);
    int x2 = int(building->GetX()) + (radius + ADD_RADIUS) + 1;
    int y2 = int(building->GetY()) + (radius + ADD_RADIUS) + 1;


    TerritoryRegion tr(x1, y1, x2, y2, this);

    // Alle Gebäude ihr Terrain in der Nähe neu berechnen
    for(nobBaseMilitarySet::iterator it = buildings.begin(); it != buildings.end(); ++it)
    {
        // Ist es ein richtiges Militärgebäude?
        if((*it)->GetBuildingType() >= BLD_BARRACKS && (*it)->GetBuildingType() <= BLD_FORTRESS)
        {
            // Wenn es noch nicht besetzt war(also gerade neu gebaut), darf es nicht mit einberechnet werden!
            if(static_cast<nobMilitary*>(*it)->IsNewBuilt())
                continue;
        }

        // Wenn das Gebäude abgerissen wird oder wenn es noch nicht besetzt war, natürlich nicht mit einberechnen
        if(*it != building || !destroyed)
            tr.CalcTerritoryOfBuilding(*it);
    }

    // Baustellen von Häfen mit einschließen
    for(std::list<noBuildingSite*>::iterator it = harbor_building_sites_from_sea.begin();
            it != harbor_building_sites_from_sea.end(); ++it)
    {
        if(*it != building || !destroyed)
            tr.CalcTerritoryOfBuilding(*it);
    }

    // Merken, wo sich der Besitzer geändert hat
    std::vector<bool> owner_changed((x2 - x1) * (y2 - y1));


    std::vector<int> sizeChanges(GAMECLIENT.GetPlayerCount());
    // Daten von der TR kopieren in die richtige Karte, dabei zus. Grenzen korrigieren und Objekte zerstören, falls
    // das Land davon jemanden anders nun gehört
	
	new_owner_of_trigger_building=tr.GetOwner(building->GetPos().x, building->GetPos().y);

    for(int y = y1; y < y2; ++y)
    {
        for(int x = x1; x < x2; ++x)
        {
            unsigned char prev_player, player;
            MapPoint t = ConvertCoords(x, y);
			
			// Wenn der Punkt den Besitz geändert hat
			if ((prev_player = GetNode(t).owner) != (player = tr.GetOwner(x, y)))
            {
                // Dann entsprechend neuen Besitzer setzen - bei improved alliances addon noch paar extra bedingungen prüfen
				if (GAMECLIENT.GetGGS().isEnabled(ADDON_NO_ALLIED_PUSH))
				{
					//rule 1: only take territory from an ally if that ally loses a building - special case: headquarter can take territory
					if (prev_player>0 && player>0 && GetPlayer(prev_player-1)->IsAlly(player-1) && !(owneroftriggerbuilding==prev_player && !newBuilt) && !building->GetBuildingType()==BLD_HEADQUARTERS)
					{
						//LOG.lprintf("rule 1 \n");
						owner_changed[(x2 - x1) * (y - y1) + (x - x1)] = false;
						continue;
					}
					//rule 2: do not gain territory when you lose a building (captured or destroyed)
					if (owneroftriggerbuilding==player && !newBuilt)
					{
						//LOG.lprintf("rule 2 \n");
						owner_changed[(x2 - x1) * (y - y1) + (x - x1)] = false;
						continue;
					}
					//rule 3: do not lose territory when you gain a building (newBuilt or capture)
					if ((owneroftriggerbuilding==prev_player && prev_player>0 && newBuilt) || (new_owner_of_trigger_building==prev_player && !destroyed && !newBuilt))
					{
						//LOG.lprintf("rule 3 \n");
						owner_changed[(x2 - x1) * (y - y1) + (x - x1)] = false;
						continue;
					}
				}
                GetNode(t).owner = player;
                owner_changed[(x2 - x1) * (y - y1) + (x - x1)] = true;
                if (player != 0)
                    sizeChanges[player - 1]++;
                if (prev_player != 0)
                    sizeChanges[prev_player - 1]--;

                // Event for map scripting
                LUA_EventOccupied(player - 1, t);
            }
            else
                owner_changed[(x2 - x1) * (y - y1) + (x - x1)] = false;
        }
    }

    for (unsigned i = 0; i < GAMECLIENT.GetPlayerCount(); ++i)
    {
        GetPlayer(i)->ChangeStatisticValue(STAT_COUNTRY, sizeChanges[i]);

        // Negatives Wachstum per Post dem/der jeweiligen Landesherren/dame melden, nur bei neugebauten Gebäuden
        if (newBuilt && sizeChanges[i] < 0)
        {
            if(GAMECLIENT.GetPlayerID() == i)
                GAMECLIENT.SendPostMessage(
                    new ImagePostMsgWithLocation(_("Lost land by this building"), PMC_MILITARY, building->GetPos(), 
                                                 building->GetBuildingType(), building->GetNation()));
        }
    }

    for(int y = y1; y < y2; ++y)
    {
        for(int x = x1; x < x2; ++x)
        {
            MapPoint t = ConvertCoords(x, y);
            bool isplayerterritory_near = false;
            /// Grenzsteine, die alleine "rausragen" und nicht mit einem richtigen Territorium verbunden sind, raushauen
            for(unsigned d = 0; d < 6; ++d)
            {
                if(IsPlayerTerritory(GetNeighbour(t, d)))
                {
                    isplayerterritory_near = true;
                    break;
                }
            }

            // Wenn kein Land angrenzt, dann nicht nehmen
            if(!isplayerterritory_near)
                GetNode(t).owner = 0;

            // Drumherum (da ja Grenzen mit einberechnet werden ins Gebiet, da darf trotzdem nichts stehen) alles vom Spieler zerstören
            // nicht das Militärgebäude oder dessen Flagge nochmal abreißen
            if(owner_changed[(x2 - x1) * (y - y1) + (x - x1)])
            {
                for(unsigned char i = 0; i < 6; ++i)
                {
                    MapPoint tt = GetNeighbour(t, i);

                    DestroyPlayerRests(tt, GetNode(t).owner, building, false);

                    // BQ neu berechnen
                    GetNode(tt).bq = CalcBQ(tt, GAMECLIENT.GetPlayerID());
                    // ggf den noch darüber, falls es eine Flagge war (kann ja ein Gebäude entstehen)
                    if(GetNodeAround(tt, 1).bq)
                        SetBQ(GetNeighbour(tt, 1), GAMECLIENT.GetPlayerID());
                }

                if(gi)
                    gi->GI_UpdateMinimap(t);
            }
        }
    }

    // Grenzsteine neu berechnen, noch 1 über das Areal hinausgehen, da dieses auch die Grenzsteine rundrum
    // mit beeinflusst

    // In diesem Array merken, wie wieviele Nachbarn ein Grenzstein hat
    //unsigned neighbors[y2-y1+7][x2-x1+7];
    std::vector<std::vector <unsigned> > neighbors(y2 - y1 + 7, std::vector<unsigned>(x2 - x1 + 7, 0));


    for(int y = y1 - 3; y < y2 + 3; ++y)
    {
        //memset(neighbors[y-(y1-3)], 0, x2-x1+7);

        for(int x = x1 - 3; x < x2 + 3; ++x)
        {
            // Korrigierte X-Koordinaten
            MapPoint c = ConvertCoords(x, y);

            unsigned char owner = GetNode(c).owner;

            // Grenzstein direkt auf diesem Punkt?
            if(owner && IsBorderNode(c, owner))
            {
                GetNode(c).boundary_stones[0] = owner;

                // Grenzsteine prüfen auf den Zwischenstücken in die 3 Richtungen nach unten und nach rechts
                for(unsigned i = 0; i < 3; ++i)
                {
                    if(IsBorderNode(GetNeighbour(c, 3 + i), owner))
                        GetNode(c).boundary_stones[i + 1] = owner;
                    else
                        GetNode(c).boundary_stones[i + 1] = 0;

                }

                // Zählen
                for(unsigned i = 0; i < 6; ++i)
                {
                    neighbors[y - (y1 - 3)][x - (x1 - 3)] = 0;
                    if(GetNodeAround(c, i).boundary_stones[0] == owner)
                        ++neighbors[y - (y1 - 3)][x - (x1 - 3)];
                }
            }
            else
            {
                // Kein Grenzstein --> etwaige vorherige Grenzsteine löschen
                for(unsigned i = 0; i < 4; ++i)
                    GetNode(c).boundary_stones[i] = 0;

                //for(unsigned i = 0;i<3;++i)
                //  GetNodeAround(x, y, 3+i).boundary_stones[i+1] = 0;
            }


        }
    }

    /*  // Nochmal durchgehen und bei Grenzsteinen mit mehr als 3 Nachbarn welche löschen
        // da sich sonst gelegentlich solche "Klötzchen" bilden können
        for(int y = y1-3;y < y2+3;++y)
        {
            //memset(neighbors[y-(y1-3)], 0, x2-x1+7);

            for(int x = x1-3;x < x2+3;++x)
            {

                // Korrigierte X-Koordinaten (nicht über den Rand gehen)
                MapCoord xc, yc;
                xcyc = ConvertCoords(x, y);

                // Steht auch hier ein Grenzstein?
                unsigned char owner = GetNode(xc, yc).boundary_stones[0];
                if(!owner)
                    continue;

                if(neighbors[y-(y1-3)][x-(x1-3)] > 2)
                {
                    for(unsigned dir = 0;dir<3 && neighbors[y-(y1-3)][x-(x1-3)] > 2;++dir)
                    {
                        // Da ein Grenzstein vom selben Besitzer?
                        MapCoord xa = GetXA(xc, yc, dir+3);
                        MapCoord ya = GetYA(xc, yc, dir+3);

                        if(GetNode(xa, ya).boundary_stones[0] == owner)
                        {
                            Point<int> p(x, y);
                            Point<int> pa = GetPointAround(p, dir+3);
                            // Hat der auch zu viele Nachbarn?
                            if(neighbors[pa.y-(y1-3)][pa.x-(x1-3)] > 2)
                            {
                                // Dann löschen wir hier einfach die Verbindung
                                GetNode(xc, yc).boundary_stones[dir+1] = 0;
                                --neighbors[y-(y1-3)][x-(x1-3)];
                                --neighbors[pa.y-(y1-3)][pa.x-(x1-3)];
                            }

                        }
                    }

                }
            }
        }*/

    // Sichtbarkeiten berechnen

    // Wurde es zerstört, müssen die Sichtbarkeiten entsprechend neu berechnet werden, ansonsten reicht es auch
    // sie einfach auf sichtbar zu setzen
    unsigned harborRadius = (building->GetBuildingType() == BLD_HARBORBUILDING)
                            ? HARBOR_ALONE_RADIUS : static_cast<const nobBaseMilitary*>(building)->GetMilitaryRadius();
    if(destroyed)
        RecalcVisibilitiesAroundPoint(building->GetPos(), harborRadius + VISUALRANGE_MILITARY, 
                                      building->GetPlayer(), destroyed ? building : 0);
    else
        SetVisibilitiesAroundPoint(building->GetPos(), harborRadius + VISUALRANGE_MILITARY, 
                                   building->GetPlayer());
}

bool GameWorldGame::TerritoryChange(const noBaseBuilding* const building, const unsigned short radius, const bool destroyed, const bool newBuilt)
{
    nobBaseMilitarySet buildings = LookForMilitaryBuildings(building->GetPos(), 3);

    // Radius der noch draufaddiert wird auf den eigentlich ausreichenden Bereich, für das Eliminieren von
    // herausragenden Landesteilen und damit Grenzsteinen
    const int ADD_RADIUS = 2;

    // Koordinaten erzeugen für TerritoryRegion
    int x1 = int(building->GetX()) - (radius + ADD_RADIUS);
    int y1 = int(building->GetY()) - (radius + ADD_RADIUS);
    int x2 = int(building->GetX()) + (radius + ADD_RADIUS) + 1;
    int y2 = int(building->GetY()) + (radius + ADD_RADIUS) + 1;


    TerritoryRegion tr(x1, y1, x2, y2, this);

    // Alle Gebäude ihr Terrain in der Nähe neu berechnen
    for(nobBaseMilitarySet::iterator it = buildings.begin(); it != buildings.end(); ++it)
    {
        // Ist es ein richtiges Militärgebäude?
        if((*it)->GetBuildingType() >= BLD_BARRACKS && (*it)->GetBuildingType() <= BLD_FORTRESS)
        {
            // Wenn es noch nicht besetzt war(also gerade neu gebaut), darf es nicht mit einberechnet werden!
            if(static_cast<nobMilitary*>(*it)->IsNewBuilt())
                continue;
        }

        // Wenn das Gebäude abgerissen wird oder wenn es noch nicht besetzt war, natürlich nicht mit einberechnen
        if(*it != building)
            tr.CalcTerritoryOfBuilding(*it);
    }

    // Baustellen von Häfen mit einschließen
    for(std::list<noBuildingSite*>::iterator it = harbor_building_sites_from_sea.begin();
            it != harbor_building_sites_from_sea.end(); ++it)
    {
        if(*it != building || !destroyed)
            tr.CalcTerritoryOfBuilding(*it);
    }
    // schaun ob sich was ändern würd im berechneten gebiet
    for(int y = y1; y < y2; ++y)
    {
        for(int x = x1; x < x2; ++x)
        {
            unsigned char prev_player, player;
            MapPoint t = ConvertCoords(x, y);
            if((prev_player = GetNode(t).owner) != (player = tr.GetOwner(x, y)))
            {
                // if gameobjective isnt 75% ai can ignore water/snow/lava/swamp terrain (because it wouldnt help win the game)
                if(GAMECLIENT.GetGGS().game_objective == GlobalGameSettings::GO_CONQUER3_4)
                    return false;
                unsigned char t1 = GetNode(t).t1, t2 = GetNode(t).t2;
                if((t1 != TT_WATER && t1 != TT_LAVA && t1 != TT_SWAMPLAND && t1 != TT_SNOW) && (t2 != TT_WATER && t2 != TT_LAVA && t2 != TT_SWAMPLAND && t2 != TT_SNOW))
                    return false;
                //also check neighboring nodes for their terrain since border will still count as player territory but not allow any buildings !
                for(int j = 0; j < 6; j++)
                {
                    t1 = GetNode(GetNeighbour(t, j)).t1;
                    t2 = GetNode(GetNeighbour(t, j)).t2;
                    if((t1 != TT_WATER && t1 != TT_LAVA && t1 != TT_SWAMPLAND && t1 != TT_SNOW) || (t2 != TT_WATER && t2 != TT_LAVA && t2 != TT_SWAMPLAND && t2 != TT_SNOW))
                        return false;
                }
            }
        }
    }
    return true;
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
            if(no->GetType() == NOP_FLAG && no != (exception ? exception->GetFlag() : 0))
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
    if(pt.x >= width || pt.y >= height)
        return false;


    // Irgendwelche Objekte im Weg?
    noBase::BlockingManner bm = GetNO(pt)->GetBM();
    if(bm != noBase::BM_NOTBLOCKING && bm != noBase::BM_TREE && bm != noBase::BM_FLAG)
        return false;

    unsigned char t;

    // Terrain untersuchen
    unsigned char good_terrains = 0;
    for(unsigned char i = 0; i < 6; ++i)
    {
        t = GetTerrainAround(pt, i);
        if(TERRAIN_BQ[t] == BQ_CASTLE || TERRAIN_BQ[t] == BQ_MINE || TERRAIN_BQ[t] == BQ_FLAG) ++good_terrains;
        else if(TERRAIN_BQ[t] == BQ_DANGER) return false; // in die Nähe von Lava usw. dürfen die Figuren gar nich kommen!
    }

    // Darf nicht im Wasser liegen, 
    if(!good_terrains)
        return false;

    return true;
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
    if(!GetPlayer(player_attacker)->IsPlayerAttackable(GetSpecObj<noBuilding>(pt)->GetPlayer()))
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
    nobBaseMilitarySet buildings = LookForMilitaryBuildings(pt, 3);

    // Liste von verfügbaren Soldaten, geordnet einfügen, damit man dann starke oder schwache Soldaten nehmen kann
    std::list<PotentialAttacker> soldiers;


    for(nobBaseMilitarySet::iterator it = buildings.begin(); it != buildings.end(); ++it) {
        // Muss ein Gebäude von uns sein und darf nur ein "normales Militärgebäude" sein (kein HQ etc.)
        if((*it)->GetPlayer() != player_attacker || (*it)->GetBuildingType() < BLD_BARRACKS || (*it)->GetBuildingType() > BLD_FORTRESS)
            continue;

        // Soldaten ausrechnen, wie viel man davon nehmen könnte, je nachdem wie viele in den
        // Militäreinstellungen zum Angriff eingestellt wurden
        unsigned soldiers_count =
            (static_cast<nobMilitary*>(*it)->GetTroopsCount() > 1) ?
            ((static_cast<nobMilitary*>(*it)->GetTroopsCount() - 1) * GetPlayer(player_attacker)->military_settings[3] / MILITARY_SETTINGS_SCALE[3]) : 0;

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
        nobMilitary::SortedTroopsContainer& troops = static_cast<nobMilitary*>(*it)->troops;
        if(strong_soldiers){
            // Strong soldiers first
            for(nobMilitary::SortedTroopsContainer::reverse_iterator it2 = troops.rbegin();
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
            for(nobMilitary::SortedTroopsContainer::iterator it2 = troops.begin();
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

    for(std::list<PotentialAttacker>::iterator it = soldiers.begin();
            it != soldiers.end() && i < soldiers_count; ++i, ++it)
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
    if(GAMECLIENT.GetGGS().getSelection(ADDON_SEA_ATTACK) == 2)
        return;
    // Verzögerungsbug-Abfrage:
    // Existiert das angegriffenen Gebäude überhaupt noch?
    if(GetNO(pt)->GetGOT() != GOT_NOB_MILITARY && GetNO(pt)->GetGOT() != GOT_NOB_HQ
            && GetNO(pt)->GetGOT() != GOT_NOB_HARBORBUILDING)
        return;

    // Auch noch ein Gebäude von einem Feind (nicht inzwischen eingenommen)?
    if(!GetPlayer(player_attacker)->IsPlayerAttackable(GetSpecObj<noBuilding>(pt)->GetPlayer()))
        return;

    // Prüfen, ob der angreifende Spieler das Gebäude überhaupt sieht (Cheatvorsorge)
    if(CalcWithAllyVisiblity(pt, player_attacker) != VIS_VISIBLE)
        return;

    // Verfügbare Soldaten herausfinden
    std::vector<GameWorldBase::PotentialSeaAttacker> attackers = GetAvailableSoldiersForSeaAttack(player_attacker, pt);

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


bool GameWorldGame::IsRoadNodeForFigures(const MapPoint pt, const unsigned char dir)
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
            if((dir + 3) % 6 == static_cast<noFigure*>(*it)->GetDir())
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
    for(unsigned i = 0; i < map_size; ++i)
    {
        if(nodes[i].obj)
        {
            if(nodes[i].obj->GetGOT() == GOT_FLAG)
            {
                noFlag* flag = static_cast<noFlag*>(nodes[i].obj);
                nodes[i].obj = 0;
                flag->DestroyAttachedBuilding();
                flag->Destroy();
                delete flag;
            }
        }
    }
}

void GameWorldGame::Armageddon(const unsigned char player)
{
    for(unsigned i = 0; i < map_size; ++i)
    {
        if(nodes[i].obj)
        {
            if(nodes[i].obj->GetGOT() == GOT_FLAG)
            {
                noFlag* flag = static_cast<noFlag*>(nodes[i].obj);
                if (flag->GetPlayer() == player)
                {
                    nodes[i].obj = 0;
                    flag->DestroyAttachedBuilding();
                    flag->Destroy();
                    delete flag;
                }
            }
        }
    }
}



bool GameWorldGame::ValidWaitingAroundBuildingPoint(const MapPoint pt, nofAttacker* attacker, const MapPoint center)
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
    if(FindHumanPath(pt, center, CalcDistance(pt, center)) == 0xff)
        return false;
    return true;
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
    if(bm != noBase::BM_NOTBLOCKING && bm != noBase::BM_TREE && bm != noBase::BM_FLAG)
        return false;

    return true;
}

bool GameWorldGame::IsPointCompletelyVisible(const MapPoint pt, const unsigned char player, const noBaseBuilding* const exception) const
{
    nobBaseMilitarySet buildings = LookForMilitaryBuildings(pt, 3);

    // Sichtbereich von Militärgebäuden
    for(nobBaseMilitarySet::iterator it = buildings.begin(); it != buildings.end(); ++it)
    {
        if((*it)->GetPlayer() == player && *it != exception)
        {
            // Prüfen, obs auch unbesetzt ist
            if((*it)->GetGOT() == GOT_NOB_MILITARY)
            {
                if(static_cast<nobMilitary*>(*it)->IsNewBuilt())
                    continue;
            }

            if(CalcDistance(pt, (*it)->GetPos())
                    <= unsigned((*it)->GetMilitaryRadius() + VISUALRANGE_MILITARY))
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
                    <= unsigned(HARBOR_ALONE_RADIUS + VISUALRANGE_MILITARY))
                return true;
        }
    }

    // Sichtbereich von Spähtürmen

    for(std::list<nobUsual*>::const_iterator it = GetPlayer(player)->GetBuildings(BLD_LOOKOUTTOWER).begin();
            it != GetPlayer(player)->GetBuildings(BLD_LOOKOUTTOWER).end(); ++it)
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
        if (visibility_before != VIS_VISIBLE)
        {
            LUA_EventExplored(player, pt);
        }

        GetNode(pt).fow[player].visibility = VIS_VISIBLE;

        // Etwaige FOW-Objekte zerstören
        delete GetNode(pt).fow[player].object;
        GetNode(pt).fow[player].object = NULL;
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
                    GetNode(pt).fow[player].visibility = VIS_FOW;

                    SaveFOWNode(pt, player);
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
            (GAMECLIENT.GetGGS().team_view && GAMECLIENT.GetLocalPlayer()->IsAlly(player)))
        VisibilityChanged(pt);
}

void GameWorldGame::SetVisibility(const MapPoint pt,  const unsigned char player)
{
    Visibility visibility_before = GetNode(pt).fow[player].visibility;
    GetNode(pt).fow[player].visibility = VIS_VISIBLE;

    if (visibility_before != VIS_VISIBLE)
    {
        LUA_EventExplored(player, pt);
    }
    
    // Etwaige FOW-Objekte zerstören
    delete GetNode(pt).fow[player].object;
    GetNode(pt).fow[player].object = NULL;

    // Minimap Bescheid sagen
    if(gi && visibility_before != GetNode(pt).fow[player].visibility)
        gi->GI_UpdateMinimap(pt);

    // Lokaler Spieler oder Verbündeter (wenn Team-Sicht an ist)? --> Terrain updaten
    if(player == GAMECLIENT.GetPlayerID() ||
            (GAMECLIENT.GetGGS().team_view && GAMECLIENT.GetLocalPlayer()->IsAlly(player)))
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
    SetVisibility(pt, player);

    for(MapCoord tx = GetXA(pt, 0), r = 1; r <= radius; tx = GetXA(tx, pt.y, 0), ++r)
    {
        MapPoint t2(tx, pt.y);
        for(unsigned i = 2; i < 8; ++i)
        {
            for(MapCoord r2 = 0; r2 < r; t2 = GetNeighbour(t2, i % 6), ++r2)
                SetVisibility(t2, player);
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
    SetVisibility(t, player);
    MapPoint tt(t);
    unsigned char dir = (moving_dir + 2) % 6;
    for(MapCoord i = 0; i < radius; ++i)
    {
        tt = GetNeighbour(tt, dir);
        // Sichtbarkeit und für FOW-Gebiet vorherigen Besitzer merken
        // (d.h. der dort  zuletzt war, als es für Spieler player sichtbar war)
        Visibility old_vis = CalcWithAllyVisiblity(tt, player);
        unsigned char old_owner = GetNode(tt).fow[player].owner;
        SetVisibility(tt, player);
        // Neues feindliches Gebiet entdeckt?
        // Muss vorher undaufgedeckt oder FOW gewesen sein, aber in dem Fall darf dort vorher noch kein
        // Territorium entdeckt worden sein
        unsigned char current_owner = GetNode(tt).owner;
        if(current_owner && (old_vis == VIS_INVISIBLE ||
                             (old_vis == VIS_FOW && old_owner != current_owner)))
        {
            if(GAMECLIENT.GetPlayer(player)->IsPlayerAttackable(current_owner - 1) && enemy_territory)
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
        SetVisibility(tt, player);
        // Neues feindliches Gebiet entdeckt?
        // Muss vorher undaufgedeckt oder FOW gewesen sein, aber in dem Fall darf dort vorher noch kein
        // Territorium entdeckt worden sein
        unsigned char current_owner = GetNode(tt).owner;
        if(current_owner && (old_vis == VIS_INVISIBLE ||
                             (old_vis == VIS_FOW && old_owner != current_owner)))
        {
            if(GAMECLIENT.GetPlayer(player)->IsPlayerAttackable(current_owner - 1) && enemy_territory)
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


void GameWorldGame::SaveFOWNode(const MapPoint pt, const unsigned player)
{
    GetNode(pt).fow[player].last_update_time = GAMECLIENT.GetGFNumber();

    // FOW-Objekt erzeugen
    noBase* obj = GetNO(pt);
    delete GetNode(pt).fow[player].object;
    GetNode(pt).fow[player].object = obj->CreateFOWObject();


    // Wege speichern, aber nur richtige, keine, die gerade gebaut werden
    for(unsigned i = 0; i < 3; ++i)
    {
        if(GetNode(pt).roads_real[i])
            GetNode(pt).fow[player].roads[i] = GetNode(pt).roads[i];
        else
            GetNode(pt).fow[player].roads[i] = 0;
    }

    // Besitzverhältnisse speichern, damit auch die Grenzsteine im FoW gezeichnet werden können
    GetNode(pt).fow[player].owner = GetNode(pt).owner;
    // Grenzsteine merken
    for(unsigned i = 0; i < 4; ++i)
        GetNode(pt).fow[player].boundary_stones[i] = GetNode(pt).boundary_stones[i];
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

    // Zeiger auf zu veränderte Daten
    unsigned char* resources;

    //LOG.lprintf("Convert map resources from %i to %i\n", from, to);
    // Alle Punkte durchgehen
    for (MapCoord x = 0; x < width; ++x)
        for (MapCoord y = 0; y < height; ++y)
        {
            resources = &(GetNode(MapPoint(x, y)).resources);
            // Gibt es Ressourcen dieses Typs?
            // Wenn ja, dann umwandeln bzw löschen
            if (*resources >= 0x40 + from * 8 && *resources < 0x48 + from * 8)
                *resources -= ((to != 0xFF) ?  from * 8 - to * 8 : *resources);
        }
}

/// Prüfen, ob zu einem bestimmten Küsenpunkt ein Hafenpunkt gehört und wenn ja, wird dieser zurückgegeben
unsigned short GameWorldGame::GetHarborPosID(const MapPoint pt)
{
    for(unsigned d = 0; d < 6; ++d)
    {
        for(unsigned i = 1; i < harbor_pos.size(); ++i)
        {
            if(harbor_pos[i].pos == GetNeighbour(pt, d))
            {
                return i;
            }
        }
    }

    return 0;
}

/// Bestimmt die Schifffahrtrichtung, in der ein Punkt relativ zu einem anderen liegt
unsigned char GameWorldGame::GetShipDir(Point<int> pos1, Point<int> pos2)
{
    // Richtung bestimmen, in der dieser Punkt relativ zum Ausgangspunkt steht
    unsigned char exp_dir = 0xff;

    unsigned diff = SafeDiff<int>(pos1.y, pos2.y);
    if(!diff)
        diff = 1;
    // Oben?
    bool marginal_x = ((SafeDiff<int>(pos1.x, pos2.x) * 1000 / diff) < 180);
    if(pos2.y < pos1.y)
    {
        if(marginal_x)
            exp_dir = 0;
        else if(pos2.x < pos1.x)
            exp_dir = 5;
        else
            exp_dir = 1;
    }
    else
    {
        if(marginal_x)
            exp_dir = 3;
        else if(pos2.x < pos1.x)
            exp_dir = 4;
        else
            exp_dir = 2;
    }

    return exp_dir;

}

// class for finding harbor neighbors
class CalcHarborPosNeighborsNode
{
    public:
        CalcHarborPosNeighborsNode() {}
        CalcHarborPosNeighborsNode(const MapPoint pt, unsigned way) : pos(pt), way(way) {}

        MapPoint pos;
        unsigned way;
};

/// Calculate the distance from each harbor to the others
void GameWorldGame::CalcHarborPosNeighbors()
{
    // A high performance open list:
    // - open list is sorted through the way we insert points (we always only add score + 1 to the end of the list)
    // - completed points are just skipped (todo_offset)
    size_t todo_offset = 0;
    size_t todo_length = 0;
    std::vector<CalcHarborPosNeighborsNode> todo_list(width * height);

    // pre-calculate sea-points, as IsSeaPoint is rather expensive
    std::vector<unsigned int> flags_init(width * height);

    for (MapPoint p(0, 0); p.y < height; p.y++)
        for (p.x = 0; p.x < width; p.x++)
            flags_init[GetIdx(p)] = IsSeaPoint(p) ? 1 : 0;

    for (size_t i = 1; i < harbor_pos.size(); ++i)
    {
        todo_offset = 0;
        todo_length = 0;

        // Copy sea points to working flags. Possible values are
        // 0 - visited or no sea point
        // 1 - sea point, not already visited
        // n - harbor_pos[n - 1]
        std::vector<unsigned int> flags(flags_init);

        // add another entry, so that we can use the value from 'flags' directly.
        std::vector<bool> found(harbor_pos.size() + 1, false);

        // mark points around harbors
        for (size_t nr = 1; nr < harbor_pos.size(); ++nr)
        {
            /* Mark sea points belonging to harbor_pos[nr]:

            As sea points are only those fully surrounded by sea, we have to go two
            steps away from a harbor point to find them -> GetXA2/GetYA2.
            */
            for (size_t d = 0; d < 12; ++d)
            {
                MapPoint pa = GetNeighbour2(harbor_pos[nr].pos, d);

                if (flags[GetIdx(pa)] == 1)
                {
                    if (nr == i)
                    {
                        // This is our start harbor. Add the sea points around it to our todo list.
                        todo_list[todo_length++] = CalcHarborPosNeighborsNode(pa, 0);
                        flags[GetIdx(pa)] = 0; // Mark them as visited (flags = 0) to avoid finding a way to our start harbor.
                    }
                    else
                    {
                        flags[GetIdx(pa)] = nr + 1;
                    }
                }
            }
        }

        while (todo_length) // as long as there are sea points on our todo list...
        {
            CalcHarborPosNeighborsNode p = todo_list[todo_offset];
            todo_offset++;
            todo_length--;

            for (size_t d = 0; d < 6; ++d)
            {
                MapPoint pa = GetNeighbour(p.pos, d);
                size_t idx = GetIdx(pa);

                if ((flags[idx] > 1) && !found[flags[idx]]) // found harbor we haven't already found
                {
                    harbor_pos[i].neighbors[GetShipDir(Point<int>(harbor_pos[i].pos), Point<int>(pa))].push_back(HarborPos::Neighbor(flags[idx] - 1, p.way + 1));

                    todo_list[todo_offset + todo_length] = CalcHarborPosNeighborsNode(pa, p.way + 1);
                    todo_length++;

                    found[flags[idx]] = 1;

                    flags[idx] = 0; // mark as visited, so we do not go here again
                }
                else if (flags[idx])    // this detects any sea point plus harbors we already visited
                {
                    todo_list[todo_offset + todo_length] = CalcHarborPosNeighborsNode(pa, p.way + 1);
                    todo_length++;

                    flags[idx] = 0; // mark as visited, so we do not go here again
                }
            }
        }
    }
}


/// Gründet vom Schiff aus eine neue Kolonie
bool GameWorldGame::FoundColony(const unsigned harbor_point, const unsigned char player, const unsigned short sea_id)
{
    // Ist es hier überhaupt noch möglich, eine Kolonie zu gründen?
    if(!IsHarborPointFree(harbor_point, player, sea_id))
        return false;

    MapPoint pos(GetHarborPoint(harbor_point));

    noBase* no = GetSpecObj<noBase>(pos);

    if(no)
    {
        no->Destroy();
        delete no;
    }

    // Hafenbaustelle errichten
    noBuildingSite* bs = new noBuildingSite(pos, player);
    SetNO(bs, pos);
    AddHarborBuildingSiteFromSea(bs);

    gi->GI_UpdateMinimap(pos);

    RecalcTerritory(bs, HARBOR_ALONE_RADIUS, false, true);

    // BQ neu berechnen (evtl durch RecalcTerritory noch nicht geschehen)
    RecalcBQAroundPointBig(pos);
    //notify the ai
    GAMECLIENT.SendAIEvent(new AIEvent::Location(AIEvent::NewColonyFounded, pos), player);

    return true;
}

/// Gibt zurück, ob eine bestimmte Baustellen eine Baustelle ist, die vom Schiff aus errichtet wurde
bool GameWorldGame::IsHarborBuildingSiteFromSea(const noBuildingSite* building_site) const
{
    return helpers::contains(harbor_building_sites_from_sea, building_site);
}


/// Liefert eine Liste der Hafenpunkte, die von einem bestimmten Hafenpunkt erreichbar sind
void GameWorldGame::GetHarborPointsWithinReach(const unsigned hp, std::vector<unsigned>& hps) const
{
    for(unsigned i = 1; i < harbor_pos.size(); ++i)
    {
        if(i == hp)
            continue;
        unsigned dist = CalcHarborDistance(hp, i);
        if(dist == 0xffffffff)
            continue;

        hps.push_back(i);
    }
}

/// Create Trade graphs
void GameWorldGame::CreateTradeGraphs()
{
    // Only if trade is enabled
    if(!GAMECLIENT.GetGGS().isEnabled(ADDON_TRADE))
        return;

    unsigned tt = VIDEODRIVER.GetTickCount();


    for(unsigned i = 0; i < tgs.size(); ++i)
        delete tgs[i];
    tgs.resize(GAMECLIENT.GetPlayerCount());
    for(unsigned i = 0; i < tgs.size(); ++i)
        tgs[i] = new TradeGraph(i, this);

    // Calc the graph for the first player completely
    tgs[0]->Create();

    printf("first: %u ms;\n", VIDEODRIVER.GetTickCount() - tt);


    // And use this one for the others
    for(unsigned i = 1; i < GAMECLIENT.GetPlayerCount(); ++i)
        tgs[i]->CreateWithHelpOfAnotherPlayer(*tgs[0], *players);
    printf("others: %u ms;\n", VIDEODRIVER.GetTickCount() - tt);
}

/// Creates a Trade Route from one point to another
void GameWorldGame::CreateTradeRoute(const MapPoint start, MapPoint dest, const unsigned char player, TradeRoute** tr)
{
    *tr = new TradeRoute(tgs[player], start, dest);
}
