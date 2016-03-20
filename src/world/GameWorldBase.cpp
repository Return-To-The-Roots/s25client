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
#include "world/GameWorldBase.h"
#include "GameClient.h"
#include "buildings/nobHarborBuilding.h"
#include "buildings/nobMilitary.h"
#include "figures/nofPassiveSoldier.h"
#include "nodeObjs/noMovable.h"
#include "nodeObjs/noFlag.h"
#include "lua/LuaInterfaceGame.h"
#include "pathfinding/RoadPathFinder.h"
#include "pathfinding/FreePathFinder.h"
#include "gameData/TerrainData.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

GameWorldBase::GameWorldBase() : roadPathFinder(new RoadPathFinder(*this)), freePathFinder(new FreePathFinder(*this)), gi(NULL), players(NULL)
{}

GameWorldBase::~GameWorldBase()
{}

void GameWorldBase::Init(const unsigned short width, const unsigned short height, LandscapeType lt)
{
    World::Init(width, height, lt);
    freePathFinder->Init(GetWidth(), GetHeight());
}

GameClientPlayer& GameWorldBase::GetPlayer(const unsigned int id) const
{
    return *players->getElement(id);
}

unsigned GameWorldBase::GetPlayerCt() const
{
    return players->getCount();
}

bool GameWorldBase::RoadAvailable(const bool boat_road, const MapPoint pt, const bool visual) const
{
    // Hindernisse
    if(GetNode(pt).obj)
    {
        noBase::BlockingManner bm = GetNode(pt).obj->GetBM();
        if(bm != noBase::BM_NOTBLOCKING)
            return false;
    }

    //dont build on the border
    if(GetNode(pt).boundary_stones[0])
        return false;

    for(unsigned char z = 0; z < 6; ++z)
    {
        // Roads around charburner piles are not possible
        if(GetNO(GetNeighbour(pt, z))->GetBM() == noBase::BM_CHARBURNERPILE)
            return false;

        // Other roads at this point?
        if(GetPointRoad(pt, z, visual))
            return false;
    }

    for(unsigned char i = 3; i < 6; ++i)
    {
        if(GetNO(GetNeighbour(pt, i))->GetBM() == noBase::BM_CASTLE)
            return false;
    }

    // Terrain (unterscheiden, ob Wasser und Landweg)
    if(!boat_road)
    {
        bool flagPossible = false;

        for(unsigned char i = 0; i < 6; ++i)
        {
            BuildingQuality bq = TerrainData::GetBuildingQuality(GetTerrainAround(pt, i));
            if(bq == BQ_CASTLE || bq == BQ_MINE || bq == BQ_FLAG)
                flagPossible = true;
            else if(bq == BQ_DANGER)
                return false;
        }

        return flagPossible;
    }
    else
    {
        // Beim Wasserweg muss um den Punkt herum Wasser sein
        for(unsigned i = 0; i < 6; ++i)
            if(!TerrainData::IsWater(GetTerrainAround(pt, i)))
                return false;
    }

    return true;
}

bool GameWorldBase::RoadAlreadyBuilt(const bool  /*boat_road*/, const MapPoint start, const std::vector<unsigned char>& route)
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

void GameWorldBase::CalcRoad(const MapPoint pt, const unsigned char  /*player*/)
{
    CalcAndSetBQ(pt, GAMECLIENT.GetPlayerID());

    for(unsigned i = 3; i < 6; ++i)
        CalcAndSetBQ(GetNeighbour(pt, i), GAMECLIENT.GetPlayerID());
}

bool GameWorldBase::IsMilitaryBuildingNearNode(const MapPoint nPt, const unsigned char player) const
{
    // Im Umkreis von 4 Punkten ein Militärgebäude suchen
    MapPoint pt(nPt);

    for(int r = 1; r <= 4; ++r)
    {
        // Eins weiter nach links gehen
        pt = GetNeighbour(pt, 0);

        for(unsigned dir = 0; dir < 6; ++dir)
        {
            for(unsigned short i = 0; i < r; ++i)
            {
                if(IsMilitaryBuilding(pt) && (GetNode(pt).owner == player + 1))
                    return true;
                // Nach rechts oben anfangen
                pt = GetNeighbour(pt, (2 + dir) % 6);
            }
        }
    }

    // Keins gefunden
    return false;
}

bool GameWorldBase::IsMilitaryBuilding(const MapPoint pt) const
{
    if(GetNO(pt)->GetType() == NOP_BUILDING || GetNO(pt)->GetType() == NOP_BUILDINGSITE)
    {
        if( (GetSpecObj<noBaseBuilding>(pt)->GetBuildingType() >= BLD_BARRACKS &&
                GetSpecObj<noBaseBuilding>(pt)->GetBuildingType() <= BLD_FORTRESS) ||
                GetSpecObj<noBaseBuilding>(pt)->GetBuildingType() == BLD_HEADQUARTERS ||
                GetSpecObj<noBaseBuilding>(pt)->GetBuildingType() == BLD_HARBORBUILDING)
            return true;
    }


    return false;
}

sortedMilitaryBlds GameWorldBase::LookForMilitaryBuildings(const MapPoint pt, unsigned short radius) const
{
    return militarySquares.GetBuildingsInRange(pt, radius);
}

/// Baut eine (bisher noch visuell gebaute) Straße wieder zurück
void GameWorldBase::RemoveVisualRoad(const MapPoint start, const std::vector<unsigned char>& route)
{
    MapPoint pt(start);
    // Wieder zurückbauen
    for(unsigned z = 0; z < route.size(); ++z)
    {
        if (!GetPointRoad(pt, route[z], false))
        {
            SetPointVirtualRoad(pt, route[z], 0);
            CalcRoad(pt, GAMECLIENT.GetPlayerID());
        }

        pt = GetNeighbour(pt, route[z]);
    }
}

bool GameWorldBase::IsNodeToNodeForFigure(const MapPoint pt, const unsigned dir) const
{
    // Wenn ein Weg da drüber geht, dürfen wir das sowieso, aber kein Wasserweg!
    unsigned char road = GetPointRoad(pt, dir);
    if(road && road != RoadSegment::RT_BOAT + 1)
        return true;

    // Nicht über Wasser, Lava, Sümpfe gehen
    // Als Boot dürfen wir das natürlich
    TerrainType t1 = GetWalkingTerrain1(pt, dir), 
        t2 = GetWalkingTerrain2(pt, dir);

    return (TerrainData::IsUseable(t1) || TerrainData::IsUseable(t2));
}

noFlag* GameWorldBase::GetRoadFlag(MapPoint pt, unsigned char& dir, unsigned last_i)
{
    unsigned char i = 0;

    while(true)
    {
        // suchen, wo der Weg weitergeht
        for(i = 0; i < 6; ++i)
        {
            if(GetPointRoad(pt, i) && i != last_i)
                break;
        }

        if(i == 6)
            return 0;

        pt = GetNeighbour(pt, i);

        // endlich am Ende des Weges und an einer Flagge angekommen?
        if(GetNO(pt)->GetType() == NOP_FLAG)
        {
            dir = (i + 3) % 6;
            return GetSpecObj<noFlag>(pt);
        }
        last_i = (i + 3) % 6;
    }
}

/// Verändert die Höhe eines Punktes und die damit verbundenen Schatten
void GameWorldBase::AltitudeChanged(const MapPoint pt)
{
    // Baumöglichkeiten neu berechnen
    // Direkt drumherum
    for(unsigned i = 0; i < 6; ++i)
        CalcAndSetBQ(GetNeighbour(pt, i), GAMECLIENT.GetPlayerID());
    // noch eine Schale weiter außen
    for(unsigned i = 0; i < 12; ++i)
        CalcAndSetBQ(GetNeighbour2(pt, i), GAMECLIENT.GetPlayerID());
}

Visibility GameWorldBase::CalcWithAllyVisiblity(const MapPoint pt, const unsigned char player) const
{
    Visibility best_visibility = GetNode(pt).fow[player].visibility;

    if (best_visibility == VIS_VISIBLE)
        return best_visibility;

    /// Teamsicht aktiviert?
    if(GAMECLIENT.GetGGS().team_view)
    {
        // Dann prüfen, ob Teammitglieder evtl. eine bessere Sicht auf diesen Punkt haben
        for(unsigned i = 0; i < GAMECLIENT.GetPlayerCount(); ++i)
        {
            if(GAMECLIENT.GetPlayer(i).IsAlly(player))
            {
                if(GetNode(pt).fow[i].visibility > best_visibility)
                    best_visibility = GetNode(pt).fow[i].visibility;
            }
        }
    }

    return best_visibility;
}

unsigned short GameWorldBase::IsCoastalPointToSeaWithHarbor(const MapPoint pt) const
{
    short sea = IsCoastalPoint(pt);
    if(sea)
    {
        const unsigned numHarborPts = GetHarborPointCount();
        for(unsigned i = 0; i < numHarborPts; i++)
        {
            if(IsAtThisSea(i + 1, sea))
                return sea;
        }
    }
    return 0;
}

/// Gibt Dynamische Objekte, die von einem bestimmten Punkt aus laufen oder dort stehen sowie andere Objekte, 
/// die sich dort befinden, zurück
std::vector<noBase*> GameWorldBase::GetDynamicObjectsFrom(const MapPoint pt) const
{
    std::vector<noBase*> objects;
    // Look also on the points above and below for figures
    const MapPoint coords[3] =
    {
        pt, 
        MapPoint(GetNeighbour(pt, 1)), 
        MapPoint(GetNeighbour(pt, 2))
    };

    for(unsigned i = 0; i < 3; ++i)
    {
        const std::list<noBase*>& figures = GetFigures(coords[i]);
        for(std::list<noBase*>::const_iterator it = figures.begin(); it != figures.end(); ++it)
        {
            // Ist es auch ein Figur und befindet sie sich an diesem Punkt?
            if((*it)->GetType() == NOP_FIGURE || (*it)->GetGOT() == GOT_ANIMAL || (*it)->GetGOT() == GOT_SHIP)
            {
                if(static_cast<noMovable*>(*it)->GetPos() == pt)
                    objects.push_back(*it);
            }
            else if(i == 0)
                // Den Rest nur bei den richtigen Koordinaten aufnehmen
                objects.push_back(*it);
        }
    }
    return objects;
}

/// Gibt nächsten Hafenpunkt in einer bestimmten Richtung zurück, bzw. 0, jwenn es keinen gibt
unsigned GameWorldBase::GetNextHarborPoint(const MapPoint pt, 
        const unsigned origin_harbor_id, const unsigned char dir, 
        const unsigned char player, 
        bool (GameWorldBase::*IsPointOK)(const unsigned, const unsigned char, const unsigned short) const) const
{
    RTTR_Assert(origin_harbor_id);
    //unsigned char group_id = harbor_pos[origin_harbor_id-1].cps[

    // Herausfinden, in welcher Richtung sich dieser Punkt vom Ausgangspuknt unterscheidet
    unsigned char coastal_point_dir = 0xFF;
    const MapPoint hbPt = GetHarborPoint(origin_harbor_id);

    for(unsigned char i = 0; i < 6; ++i)
    {
        if(GetNeighbour(hbPt, i) == pt)
        {
            coastal_point_dir = i;
            break;
        }
    }

    RTTR_Assert(coastal_point_dir != 0xff);

    unsigned short sea_id = GetSeaId(origin_harbor_id, Direction::fromInt(coastal_point_dir));
    const std::vector<HarborPos::Neighbor>& neighbors = GetHarborNeighbor(origin_harbor_id, Direction::fromInt(dir));


    for(unsigned i = 0; i < neighbors.size(); ++i)
    {
        // Entspricht der Punkt meinen Erwartungen?
        if((this->*IsPointOK)(neighbors[i].id, player, sea_id))
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

    IsPointOwnerDifferent(const GameWorldBase& gwb, const unsigned char player): gwb(gwb), cmpOwner(player + 1){}

    bool operator()(const MapPoint pt) const
    {
        const unsigned char owner = gwb.GetNode(pt).owner;
        return owner != 0 && owner != cmpOwner;
    }
};

/// Ist es an dieser Stelle für einen Spieler möglich einen Hafen zu bauen
bool GameWorldBase::IsHarborPointFree(const unsigned harbor_id, const unsigned char player, const unsigned short sea_id) const
{
    MapPoint hbPos(GetHarborPoint(harbor_id));

    // Befindet sich der Hafenpunkt auch an dem erforderlichen Meer?
    if(!IsAtThisSea(harbor_id, sea_id))
        return false;

    // Überprüfen, ob das Gebiet in einem bestimmten Radius entweder vom Spieler oder gar nicht besetzt ist außer wenn der Hafen und die Flagge im Spielergebiet liegen
    MapPoint flagPos = GetNeighbour(hbPos, 4);
    if(GetNode(hbPos).owner != player + 1 || GetNode(flagPos).owner != player + 1)
    {
        if(CheckPointsInRadius(hbPos, 4, IsPointOwnerDifferent(*this, player), false))
            return false;
    }

    return (CalcBQ(hbPos, 0, false, false, true) == BQ_HARBOR);
}

/// Sucht freie Hafenpunkte, also wo noch ein Hafen gebaut werden kann
unsigned GameWorldBase::GetNextFreeHarborPoint(const MapPoint pt, const unsigned origin_harbor_id, const unsigned char dir, 
        const unsigned char player) const
{
    return GetNextHarborPoint(pt, origin_harbor_id, dir, player, &GameWorldBase::IsHarborPointFree);
}

/// Berechnet die Entfernung zwischen 2 Hafenpunkten
unsigned GameWorldBase::CalcHarborDistance(const unsigned habor_id1, const unsigned harbor_id2) const
{
    if (habor_id1 == harbor_id2) //special case: distance to self
        return 0;
    for(unsigned i = 0; i < 6; ++i)
    {
        const std::vector<HarborPos::Neighbor>& neighbors = GetHarborNeighbor(habor_id1, Direction::fromInt(i));
        for(unsigned z = 0; z < neighbors.size(); ++z)
        {
            const HarborPos::Neighbor& n = neighbors[z];
            if(n.id == harbor_id2)
                return n.distance;
        }
    }

    return 0xffffffff;
}

/// Bestimmt für einen beliebigen Punkt auf der Karte die Entfernung zum nächsten Hafenpunkt
unsigned GameWorldBase::CalcDistanceToNearestHarbor(const MapPoint pos) const
{
    unsigned min_distance = 0xffffffff;
    for(unsigned i = 1; i <= GetHarborPointCount(); ++i)
        min_distance = std::min(min_distance, this->CalcDistance(pos, GetHarborPoint(i))); // Invalid id=0

    return min_distance;
}

/// returns true when a harborpoint is in SEAATTACK_DISTANCE for figures!
bool GameWorldBase::IsAHarborInSeaAttackDistance(const MapPoint pos) const
{
    for(unsigned i = 1; i <= GetHarborPointCount(); ++i) //poc: harbor dummy at spot 0 ask Oliverr why
    {
        if(CalcDistance(pos, GetHarborPoint(i)) < SEAATTACK_DISTANCE)
        {
            if(FindHumanPath(pos, GetHarborPoint(i), SEAATTACK_DISTANCE) != 0xff)
                return true;
        }
    }
    return false;
}


/// Komperator zum Sortieren
bool GameWorldBase::PotentialSeaAttacker::operator<(const GameWorldBase::PotentialSeaAttacker& pa) const
{
    // Erst nach Rang, an zweiter Stelle nach Entfernung sortieren (
    if(soldier->GetRank() == pa.soldier->GetRank())
    {
    	if (distance == pa.distance)
    	{
    		return(soldier->GetObjId() < pa.soldier->GetObjId());
    	} else
    	{
        	return distance < pa.distance;
    	}
    } else
    {
        return soldier->GetRank() > pa.soldier->GetRank();
    }
}

/// returns all sea_ids found in the given vector from which a given building can be attacked by sea
void GameWorldBase::GetValidSeaIDsAroundMilitaryBuildingForAttackCompare(const MapPoint pt, std::vector<unsigned short>& use_seas, const unsigned char player_attacker)const
{
	// Nach Hafenpunkten in der Nähe des angegriffenen Gebäudes suchen
	// Alle unsere Häfen durchgehen
	std::vector<unsigned short> confirmedseaids;
    for(unsigned i = 1; i <= GetHarborPointCount(); ++i)
	{
		const MapPoint harborPt = GetHarborPoint(i);
		
		if(CalcDistance(harborPt, pt) > SEAATTACK_DISTANCE)
            continue;

		//target isnt the harbor pos AND there is an enemy harbor AND the sea attack addon is set to block on enemy harbor? -> done for this harbor pos
        const nobHarborBuilding *hb = GetSpecObj<nobHarborBuilding>(harborPt);
		if(pt != harborPt && hb && (players->getElement(player_attacker)->IsPlayerAttackable(GetNode(harborPt).owner-1) && GAMECLIENT.GetGGS().getSelection(AddonId::SEA_ATTACK)==1))			
			continue;

		// Ist Ziel der Hafenspot? -> add sea_ids
		if(pt == harborPt)
		{
            for(unsigned z = 0; z < 6; ++z)
			{
                const unsigned short seadId = GetSeaId(i, Direction::fromInt(z));
				if(!seadId)
                    continue;
				//sea id is in compare list and not yet in confirmed list? add to confirmed list if the pathfinding is ok
				if(helpers::contains(use_seas, seadId) && !helpers::contains(confirmedseaids, seadId))
				{
					bool previouslytested=false;
                    for(unsigned k = 0; k < z; k++)
					{	
						if(seadId == GetSeaId(i, Direction::fromInt(k)))
						{
							previouslytested=true;
							break;
						}
					}
					if(previouslytested)
						continue;
					//can figures walk from the flag of the harbor to the coastal point? Important because in some locations where the coast is north of the harbor this might be blocked
					MapPoint coastal = GetCoastalPoint(i, seadId);
							
					if((GetNeighbour(pt, 4) == coastal) || FindHumanPath(GetNeighbour(pt, 4), coastal, SEAATTACK_DISTANCE) != 0xff)
					{
						confirmedseaids.push_back(seadId);
						//all sea ids confirmed? return without changes
						if(confirmedseaids.size()==use_seas.size())
							return;																	
					}								
				}
			}
		}			
		//so our target building is in range of a free or allied harbor pos but not the harborspot - now lets see if we can findhumanpath
		else //if(FindHumanPath(x, y, harbor_x, harbor_y, SEAATTACK_DISTANCE) != 0xff)				
		{
            for(unsigned z = 0; z < 6; ++z)
			{
                const unsigned short seadId = GetSeaId(i, Direction::fromInt(z));
                if(!seadId)
                    continue;
				//sea id is in compare list and not yet in confirmed list? add to confirmed list
				if(helpers::contains(use_seas, seadId) && !helpers::contains(confirmedseaids, seadId))
				{
					bool previouslytested=false;
                    for(unsigned k = 0; k < z; k++) //checks previously tested sea ids to skip pathfinding
					{
                        if(seadId == GetSeaId(i, Direction::fromInt(k)))
						{
							previouslytested=true;
							break;
						}
					}
					if(previouslytested)
						continue;
					//can figures walk from the coastal point to the harbor?
					MapPoint coastal = GetCoastalPoint(i, seadId);
					if(FindHumanPath(pt, coastal, SEAATTACK_DISTANCE) != 0xff) //valid human path from target building to coastal point?
					{
						confirmedseaids.push_back(seadId);
						//all sea ids confirmed? return without changes
						if(confirmedseaids.size()==use_seas.size())
							return;
					}								
				}
			}
		}
	}
	//all harbor positions tested: erase all entries from use_seas we could not confirm
	use_seas.clear();
	use_seas.assign(confirmedseaids.begin(), confirmedseaids.end());
}
	
/// returns all sea_ids from which a given building can be attacked by sea
std::vector<unsigned> GameWorldBase::GetValidSeaIDsAroundMilitaryBuildingForAttack(const MapPoint pt, std::vector<bool>& use_seas, const unsigned char player_attacker) const
{
    std::vector<unsigned> harbor_points;
	// Nach Hafenpunkten in der Nähe des angegriffenen Gebäudes suchen
	// Alle unsere Häfen durchgehen
    for(unsigned i = 1; i <= GetHarborPointCount(); ++i)
	{
        const MapPoint harborPt = GetHarborPoint(i);

		if(CalcDistance(harborPt, pt) > SEAATTACK_DISTANCE)
		    continue;

		//target isnt the harbor pos AND there is an enemy harbor AND the sea attack addon is set to block on enemy harbor? -> done for this harbor pos
		const nobHarborBuilding *hb=GetSpecObj<nobHarborBuilding>(harborPt);
		if(pt != harborPt && hb && (players->getElement(player_attacker)->IsPlayerAttackable(GetNode(harborPt).owner-1) && GAMECLIENT.GetGGS().getSelection(AddonId::SEA_ATTACK)==1))		
			continue;
		// Ist Ziel der Hafenspot? -> add sea_ids from which we can actually attack the harbor
		if(pt == harborPt)
		{
			bool harborinlist=false;					
			for(unsigned z = 0;z<6;++z)
			{
                const unsigned short seadId = GetSeaId(i, Direction::fromInt(z));
                if(!seadId)
                    continue;
				//already tested the path from this coastal point to the goal (pathfinding takes a while so avoid as much as possible)
				bool previouslytested=false;
                for(unsigned k = 0; k < z; k++) //checks previously tested sea ids to skip pathfinding
                {
                    if(seadId == GetSeaId(i, Direction::fromInt(k)))
                    {
                        previouslytested = true;
                        break;
                    }
                }
				if(previouslytested)
					continue;
				//can figures walk from the flag of the harbor to the coastal point?
				MapPoint coastal = GetCoastalPoint(i, seadId);
							
				if(( GetNeighbour(pt, 4) == coastal) || FindHumanPath(GetNeighbour(pt, 4), coastal, SEAATTACK_DISTANCE) != 0xff)
				{
					use_seas.at(seadId) = true;
					if(!harborinlist)
					{
						harbor_points.push_back(i);
						harborinlist=true;
					}
				}
			}
		}			
		//so our target building is in range of a free or allied harbor pos but not the harborspot - now lets see if we can findhumanpath
		else //if(FindHumanPath(x, y, harbor_x, harbor_y, SEAATTACK_DISTANCE) != 0xff)				
		{	//first get sea ids around currently tested harbor, then for each sea id try to find a human path between the coastal point and the goal
			bool harborinlist=false;
			for(unsigned z = 0;z<6;++z) //for all directions check the sea ids
			{
                const unsigned short seadId = GetSeaId(i, Direction::fromInt(z));
                if(!seadId)
                    continue;
				bool previouslytested=false;
                for(unsigned k = 0; k < z; k++) //checks previously tested sea ids to skip pathfinding
                {
                    if(seadId == GetSeaId(i, Direction::fromInt(k)))
                    {
                        previouslytested = true;
                        break;
                    }
                }
				if(previouslytested)
					continue;
				//can figures walk from the coastal point to the harbor?
				MapPoint coastal = GetCoastalPoint(i, seadId);
				if(FindHumanPath(pt, coastal, SEAATTACK_DISTANCE) != 0xff) //valid human path from target building to coastal point?
				{
					use_seas.at(seadId) = true;
					if(!harborinlist)
					{
						harbor_points.push_back(i);
						harborinlist=true;
					}
				}
			}
		}
	}
    return harbor_points;
}

/// Liefert Hafenpunkte im Umkreis von einem bestimmten Militärgebäude
std::vector<unsigned> GameWorldBase::GetHarborPointsAroundMilitaryBuilding(const MapPoint pt) const
{
    std::vector<unsigned> harbor_points;
    // Nach Hafenpunkten in der Nähe des angegriffenen Gebäudes suchen
    // Alle unsere Häfen durchgehen
    for(unsigned i = 1; i <= GetHarborPointCount(); ++i)
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

/// Gibt Anzahl oder geschätzte Stärke(rang summe + anzahl) der verfügbaren Soldaten die zu einem Schiffsangriff starten können von einer bestimmten sea id aus
unsigned int GameWorldBase::GetAvailableSoldiersForSeaAttackAtSea(const unsigned char player_attacker, unsigned short seaid, bool count)const
{
    // Liste alle Militärgebäude des Angreifers, die Soldaten liefern
    std::vector<nobHarborBuilding::SeaAttackerBuilding> buildings;
    unsigned int attackercount = 0;
    // Angrenzende Häfen des Angreifers an den entsprechenden Meeren herausfinden
    const std::list<nobHarborBuilding*>& harbors = players->getElement(player_attacker)->GetHarbors();
    for(std::list<nobHarborBuilding*>::const_iterator it = harbors.begin(); it != harbors.end(); ++it)
    {
        // Bestimmen, ob Hafen an einem der Meere liegt, über die sich auch die gegnerischen
        // Hafenpunkte erreichen lassen
        if(!IsAtThisSea((*it)->GetHarborPosID(), seaid))
            continue;

        std::vector<nobHarborBuilding::SeaAttackerBuilding> tmp = (*it)->GetAttackerBuildingsForSeaIdAttack();
        buildings.insert(buildings.begin(), tmp.begin(), tmp.end());
    }

    // Die Soldaten aus allen Militärgebäuden sammeln
    for(unsigned i = 0; i < buildings.size(); ++i)
    {
        // Soldaten holen
        std::vector<nofPassiveSoldier*> tmp_soldiers = buildings[i].building->GetSoldiersForAttack(buildings[i].harbor->GetPos(), player_attacker);

        // Überhaupt welche gefunden?
        if(tmp_soldiers.empty())
            continue;

        // Soldaten hinzufügen
        for(unsigned j = 0; j < tmp_soldiers.size(); ++j)
        {
            if(count)
                attackercount++;
            else
                attackercount += (tmp_soldiers[j]->GetJobType() - 20); //private is type 21 so this increases soldiercount by 1-5 depending on rank
        }
    }
    return attackercount;
}

/// Sucht verfügbare Soldaten, um dieses Militärgebäude mit einem Seeangriff anzugreifen
std::vector<GameWorldBase::PotentialSeaAttacker> GameWorldBase::GetAvailableSoldiersForSeaAttack(const unsigned char player_attacker, const MapPoint pt) const
{
    std::vector<GameWorldBase::PotentialSeaAttacker> attackers;
    //sea attack abgeschaltet per addon?
    if(GAMECLIENT.GetGGS().getSelection(AddonId::SEA_ATTACK) == 2)
        return attackers;
    // Ist das Ziel auch ein richtiges Militärgebäude?
    if(GetNO(pt)->GetGOT() != GOT_NOB_HARBORBUILDING && GetNO(pt)->GetGOT() !=  GOT_NOB_HQ && GetNO(pt)->GetGOT() !=  GOT_NOB_MILITARY)
        return attackers;
    // Auch noch ein Gebäude von einem Feind (nicht inzwischen eingenommen)?
    if(!GetPlayer(player_attacker).IsPlayerAttackable(GetSpecObj<noBuilding>(pt)->GetPlayer()))
        return attackers;
    // Prüfen, ob der angreifende Spieler das Gebäude überhaupt sieht (Cheatvorsorge)
    if(CalcWithAllyVisiblity(pt, player_attacker) != VIS_VISIBLE)
        return attackers;
    std::vector<bool> use_seas(GetNumSeas());

    // Mögliche Hafenpunkte in der Nähe des Gebäudes
    std::vector< unsigned > defender_harbors = GetValidSeaIDsAroundMilitaryBuildingForAttack(pt, use_seas, player_attacker);

    // Liste alle Militärgebäude des Angreifers, die Soldaten liefern
    std::vector<nobHarborBuilding::SeaAttackerBuilding> buildings;

    // Angrenzende Häfen des Angreifers an den entsprechenden Meeren herausfinden
    const std::list<nobHarborBuilding*>& harbors = players->getElement(player_attacker)->GetHarbors();
    for(std::list<nobHarborBuilding*>::const_iterator it = harbors.begin(); it != harbors.end(); ++it)
    {
        // Bestimmen, ob Hafen an einem der Meere liegt, über die sich auch die gegnerischen
        // Hafenpunkte erreichen lassen
        bool is_at_sea = false;
        for(unsigned i = 0; i < 6; ++i)
        {
            const unsigned short seadId = GetSeaId((*it)->GetHarborPosID(), Direction::fromInt(i));
            if(seadId && use_seas[seadId])
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
            std::vector<nobHarborBuilding::SeaAttackerBuilding>::iterator oldBldIt = std::find_if(buildings.begin(), buildings.end(), nobHarborBuilding::SeaAttackerBuilding::CmpBuilding(itBld->building));
            if(oldBldIt == buildings.end())
            {
                // Not found -> Add
                buildings.push_back(*itBld);
            }else if(oldBldIt->distance > itBld->distance || (oldBldIt->distance == itBld->distance && oldBldIt->harbor->GetObjId() > itBld->harbor->GetObjId()) )
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
        std::vector<nofPassiveSoldier*> tmp_soldiers = it->building->GetSoldiersForAttack(it->harbor->GetPos(), player_attacker);

        // Soldaten hinzufügen
        for(std::vector<nofPassiveSoldier*>::const_iterator itSoldier = tmp_soldiers.begin(); itSoldier != tmp_soldiers.end(); ++itSoldier)
        {
            RTTR_Assert(std::find_if(attackers.begin(), attackers.end(), PotentialSeaAttacker::CmpSoldier(*itSoldier)) == attackers.end());
            PotentialSeaAttacker pa(*itSoldier, it->harbor, it->distance);
            attackers.push_back(pa);
        }
    }

    // Entsprechend nach Rang sortieren
    std::sort(attackers.begin(), attackers.end());
    return attackers;
}
