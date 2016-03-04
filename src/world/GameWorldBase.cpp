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
#include "WindowManager.h"
#include "PostMsg.h"
#include "Loader.h"
#include "pathfinding/RoadPathFinder.h"
#include "pathfinding/FreePathFinder.h"
#include "ai/AIEvents.h"
#include "buildings/nobHarborBuilding.h"
#include "buildings/nobMilitary.h"
#include "figures/nofPassiveSoldier.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noMovable.h"
#include "nodeObjs/noEnvObject.h"
#include "nodeObjs/noStaticObject.h"
#include "ogl/glArchivItem_Font.h"
#include "ingameWindows/iwMissionStatement.h"
#include "gameTypes/MessageTypes.h"
#include "gameData/TerrainData.h"
#include "helpers/containerUtils.h"
#include "Log.h"
#include "luaIncludes.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

#define ADD_LUA_CONST(name) lua_pushnumber(lua, name); lua_setglobal(lua, #name);

GameWorldBase::GameWorldBase() : roadPathFinder(new RoadPathFinder(*this)), freePathFinder(new FreePathFinder(*this)), gi(NULL)
{
    // initialize scripting
    lua = luaL_newstate();
    //luaL_openlibs(lua);
    luaopen_base(lua);
    luaopen_package(lua);
    luaopen_string(lua);
    luaopen_table(lua);
    luaopen_math(lua);
    
    static const luaL_Reg meta[] =
    {
        {"EnableBuilding", LUA_EnableBuilding}, 
        {"DisableBuilding", LUA_DisableBuilding}, 
        {"SetRestrictedArea", LUA_SetRestrictedArea}, 
        {"ClearResources", LUA_ClearResources}, 
        {"AddWares", LUA_AddWares}, 
        {"AddPeople", LUA_AddPeople}, 
        {"GetGF", LUA_GetGF}, 
        {"GetPlayerCount", LUA_GetPlayerCount}, 
        {"GetPeopleCount", LUA_GetPeopleCount}, 
        {"GetWareCount", LUA_GetWareCount}, 
        {"GetBuildingCount", LUA_GetBuildingCount}, 
        {"Log", LUA_Log}, 
        {"Chat", LUA_Chat}, 
        {"MissionStatement", LUA_MissionStatement}, 
        {"PostMessage", LUA_PostMessage}, 
        {"PostMessageWithLocation", LUA_PostMessageWithLocation}, 
        {"PostNewBuildings", LUA_PostNewBuildings}, 
        {"AddStaticObject", LUA_AddStaticObject}, 
        {"AddEnvObject", LUA_AddEnvObject}, 
		{"AIConstructionOrder", LUA_AIConstructionOrder}, 
        {NULL, NULL}
    };
    
    luaL_newlibtable(lua, meta);

    lua_setglobal(lua, "rttr");
    lua_getglobal(lua, "rttr");

    lua_pushlightuserdata(lua, this);

    luaL_setfuncs(lua, meta, 1);
    
    ADD_LUA_CONST(BLD_HEADQUARTERS);
    ADD_LUA_CONST(BLD_BARRACKS);
    ADD_LUA_CONST(BLD_GUARDHOUSE);
    ADD_LUA_CONST(BLD_WATCHTOWER);
    ADD_LUA_CONST(BLD_FORTRESS);
    ADD_LUA_CONST(BLD_GRANITEMINE);
    ADD_LUA_CONST(BLD_COALMINE);
    ADD_LUA_CONST(BLD_IRONMINE);
    ADD_LUA_CONST(BLD_GOLDMINE);
    ADD_LUA_CONST(BLD_LOOKOUTTOWER);
    ADD_LUA_CONST(BLD_CATAPULT);
    ADD_LUA_CONST(BLD_WOODCUTTER);
    ADD_LUA_CONST(BLD_FISHERY);
    ADD_LUA_CONST(BLD_QUARRY);
    ADD_LUA_CONST(BLD_FORESTER);
    ADD_LUA_CONST(BLD_SLAUGHTERHOUSE);
    ADD_LUA_CONST(BLD_HUNTER);
    ADD_LUA_CONST(BLD_BREWERY);
    ADD_LUA_CONST(BLD_ARMORY);
    ADD_LUA_CONST(BLD_METALWORKS);
    ADD_LUA_CONST(BLD_IRONSMELTER);
    ADD_LUA_CONST(BLD_CHARBURNER);
    ADD_LUA_CONST(BLD_PIGFARM);
    ADD_LUA_CONST(BLD_STOREHOUSE);
    ADD_LUA_CONST(BLD_MILL);
    ADD_LUA_CONST(BLD_BAKERY);
    ADD_LUA_CONST(BLD_SAWMILL);
    ADD_LUA_CONST(BLD_MINT);
    ADD_LUA_CONST(BLD_WELL);
    ADD_LUA_CONST(BLD_SHIPYARD);
    ADD_LUA_CONST(BLD_FARM);
    ADD_LUA_CONST(BLD_DONKEYBREEDER);
    ADD_LUA_CONST(BLD_HARBORBUILDING);
    
    ADD_LUA_CONST(JOB_HELPER);
    ADD_LUA_CONST(JOB_WOODCUTTER);
    ADD_LUA_CONST(JOB_FISHER);
    ADD_LUA_CONST(JOB_FORESTER);
    ADD_LUA_CONST(JOB_CARPENTER);
    ADD_LUA_CONST(JOB_STONEMASON);
    ADD_LUA_CONST(JOB_HUNTER);
    ADD_LUA_CONST(JOB_FARMER);
    ADD_LUA_CONST(JOB_MILLER);
    ADD_LUA_CONST(JOB_BAKER);
    ADD_LUA_CONST(JOB_BUTCHER);
    ADD_LUA_CONST(JOB_MINER);
    ADD_LUA_CONST(JOB_BREWER);
    ADD_LUA_CONST(JOB_PIGBREEDER);
    ADD_LUA_CONST(JOB_DONKEYBREEDER);
    ADD_LUA_CONST(JOB_IRONFOUNDER);
    ADD_LUA_CONST(JOB_MINTER);
    ADD_LUA_CONST(JOB_METALWORKER);
    ADD_LUA_CONST(JOB_ARMORER);
    ADD_LUA_CONST(JOB_BUILDER);
    ADD_LUA_CONST(JOB_PLANER);
    ADD_LUA_CONST(JOB_PRIVATE);
    ADD_LUA_CONST(JOB_PRIVATEFIRSTCLASS);
    ADD_LUA_CONST(JOB_SERGEANT);
    ADD_LUA_CONST(JOB_OFFICER);
    ADD_LUA_CONST(JOB_GENERAL);
    ADD_LUA_CONST(JOB_GEOLOGIST);
    ADD_LUA_CONST(JOB_SHIPWRIGHT);
    ADD_LUA_CONST(JOB_SCOUT);
    ADD_LUA_CONST(JOB_PACKDONKEY);
    ADD_LUA_CONST(JOB_BOATCARRIER);
    ADD_LUA_CONST(JOB_CHARBURNER);
    
    ADD_LUA_CONST(GD_BEER);
    ADD_LUA_CONST(GD_TONGS);
    ADD_LUA_CONST(GD_HAMMER);
    ADD_LUA_CONST(GD_AXE);
    ADD_LUA_CONST(GD_SAW);
    ADD_LUA_CONST(GD_PICKAXE);
    ADD_LUA_CONST(GD_SHOVEL);
    ADD_LUA_CONST(GD_CRUCIBLE);
    ADD_LUA_CONST(GD_RODANDLINE);
    ADD_LUA_CONST(GD_SCYTHE);
    ADD_LUA_CONST(GD_WATEREMPTY);
    ADD_LUA_CONST(GD_WATER);
    ADD_LUA_CONST(GD_CLEAVER);
    ADD_LUA_CONST(GD_ROLLINGPIN);
    ADD_LUA_CONST(GD_BOW);
    ADD_LUA_CONST(GD_BOAT);
    ADD_LUA_CONST(GD_SWORD);
    ADD_LUA_CONST(GD_IRON);
    ADD_LUA_CONST(GD_FLOUR);
    ADD_LUA_CONST(GD_FISH);
    ADD_LUA_CONST(GD_BREAD);
    ADD_LUA_CONST(GD_SHIELDROMANS);
    ADD_LUA_CONST(GD_WOOD);
    ADD_LUA_CONST(GD_BOARDS);
    ADD_LUA_CONST(GD_STONES);
    ADD_LUA_CONST(GD_SHIELDVIKINGS);
    ADD_LUA_CONST(GD_SHIELDAFRICANS);
    ADD_LUA_CONST(GD_GRAIN);
    ADD_LUA_CONST(GD_COINS);
    ADD_LUA_CONST(GD_GOLD);
    ADD_LUA_CONST(GD_IRONORE);
    ADD_LUA_CONST(GD_COAL);
    ADD_LUA_CONST(GD_MEAT);
    ADD_LUA_CONST(GD_HAM);
    ADD_LUA_CONST(GD_SHIELDJAPANESE);
    
    lua_settop(lua, 0);
}

GameWorldBase::~GameWorldBase()
{
    lua_close(lua);
}

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

bool GameWorldBase::RoadAlreadyBuilt(const bool boat_road, const MapPoint start, const std::vector<unsigned char>& route)
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

void GameWorldBase::CalcRoad(const MapPoint pt, const unsigned char player)
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
		if(pt != harborPt && hb && (players->getElement(player_attacker)->IsPlayerAttackable(GetNode(harborPt).owner-1) && GAMECLIENT.GetGGS().getSelection(ADDON_SEA_ATTACK)==1))			
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
		if(pt != harborPt && hb && (players->getElement(player_attacker)->IsPlayerAttackable(GetNode(harborPt).owner-1) && GAMECLIENT.GetGGS().getSelection(ADDON_SEA_ATTACK)==1))		
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
    if(GAMECLIENT.GetGGS().getSelection(ADDON_SEA_ATTACK) == 2)
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

int GameWorldBase::LUA_EnableBuilding(lua_State* L)
{
//  GameWorldBase *gw = static_cast<GameWorldBase*>(lua_touserdata(L, lua_upvalueindex(1)));
    int argc = lua_gettop(L);

    if (argc < 1)
    {
        lua_pushstring(L, "too few or too many arguments!");
        lua_error(L);
        return(0);
    }

    // player
    unsigned pnr = (unsigned) luaL_checknumber(L, 1);

    if (pnr >= GAMECLIENT.GetPlayerCount())
    {
        lua_pushstring(L, "player number invalid!");
        lua_error(L);
        return(0);
    }

    GameClientPlayer& player = GAMECLIENT.GetPlayer(pnr);

    if (argc == 1)
    {
        for (unsigned building_type = 0; building_type < BUILDING_TYPES_COUNT; building_type++)
        {
            player.EnableBuilding(BuildingType(building_type));
        }
        
        return(0);
    }
    
    int cnt = 2;
    while (cnt <= argc)
    {
        // building type
        unsigned building_type = (unsigned) luaL_checknumber(L, cnt++);

        if (building_type < BUILDING_TYPES_COUNT)
        {
            player.EnableBuilding(BuildingType(building_type));
        }
        else
        {
            lua_pushstring(L, "building type invalid!");
            lua_error(L);
        }
    }

    return(0);
}

int GameWorldBase::LUA_DisableBuilding(lua_State* L)
{
//  GameWorldBase *gw = static_cast<GameWorldBase*>(lua_touserdata(L, lua_upvalueindex(1)));
    int argc = lua_gettop(L);

    if (argc < 1)
    {
        lua_pushstring(L, "too few or too many arguments!");
        lua_error(L);
        return(0);
    }

    // player
    unsigned pnr = (unsigned) luaL_checknumber(L, 1);

    if (pnr >= GAMECLIENT.GetPlayerCount())
    {
        lua_pushstring(L, "player number invalid!");
        lua_error(L);
        return(0);
    }
    
    GameClientPlayer& player = GAMECLIENT.GetPlayer(pnr);
    
    if (argc == 1)
    {
        for (unsigned building_type = 0; building_type < BUILDING_TYPES_COUNT; building_type++)
        {
            player.DisableBuilding(BuildingType(building_type));
        }
        
        return(0);
    }
    
    int cnt = 2;
    while (cnt <= argc)
    {
        // building type
        unsigned building_type = (unsigned) luaL_checknumber(L, cnt++);

        if (building_type < BUILDING_TYPES_COUNT)
        {
            player.DisableBuilding(BuildingType(building_type));
        } else
        {
            lua_pushstring(L, "building type invalid!");
            lua_error(L);
        }
    }

    return(0);
}


int GameWorldBase::LUA_SetRestrictedArea(lua_State* L)
{
//  GameWorldBase *gw = static_cast<GameWorldBase*>(lua_touserdata(L, lua_upvalueindex(1)));
    int argc = lua_gettop(L) - 1;

    if ((argc < 0) || (argc % 2 == 1))
    {
        lua_pushstring(L, "wrong arguments: player, x1, y1, x2, y2, ...");
        lua_error(L);
        return(0);
    }

    // player
    unsigned pnr = (unsigned) luaL_checknumber(L, 1);

    if (pnr >= GAMECLIENT.GetPlayerCount())
    {
        lua_pushstring(L, "player number invalid!");
        lua_error(L);
        return(0);
    }

    GameClientPlayer& player = GAMECLIENT.GetPlayer(pnr);

    std::vector< MapPoint > &restricted_area = player.GetRestrictedArea();

    restricted_area.clear();

    unsigned cnt = 2;
    for (argc >>= 1; argc > 0; --argc)
    {
        MapCoord x = (MapCoord) luaL_checknumber(L, cnt++);
        MapCoord y = (MapCoord) luaL_checknumber(L, cnt++);
//        fprintf(stderr, "RESTRICTED AREA - %u, %u\n", x, y);

        restricted_area.push_back(MapPoint(x, y));
    }

    return(0);
}

int GameWorldBase::LUA_ClearResources(lua_State *L)
{
    if (lua_gettop(L) > 0)
    {
        unsigned p = (unsigned) luaL_checknumber(L, 1);

        if (p >= GAMECLIENT.GetPlayerCount())
        {
            lua_pushstring(L, "player number invalid!");
            lua_error(L);
            return(0);
        }

        const std::list<nobBaseWarehouse*> warehouses = GAMECLIENT.GetPlayer(p).GetStorehouses();
        
        for (std::list<nobBaseWarehouse*>::const_iterator wh = warehouses.begin(); wh != warehouses.end(); ++wh)
        {
            (*wh)->Clear();
        }
    } else
    {
        for (unsigned p = 0; p < GAMECLIENT.GetPlayerCount(); p++)
        {
            const std::list<nobBaseWarehouse*> warehouses = GAMECLIENT.GetPlayer(p).GetStorehouses();
            
            for (std::list<nobBaseWarehouse*>::const_iterator wh = warehouses.begin(); wh != warehouses.end(); ++wh)
            {
                (*wh)->Clear();
            }
        }
    }
    
    return(0);
}

int GameWorldBase::LUA_AddWares(lua_State* L)
{
//  GameWorldBase *gw = static_cast<GameWorldBase*>(lua_touserdata(L, lua_upvalueindex(1)));
    int argc = lua_gettop(L) - 1;

    if ((argc < 0) || (argc % 2 == 1))
    {
        lua_pushstring(L, "wrong arguments: player, ware1, count1, ware2, count2, ...");
        lua_error(L);
        return(0);
    }

    // player
    unsigned pnr = (unsigned) luaL_checknumber(L, 1);

    if (pnr >= GAMECLIENT.GetPlayerCount())
    {
        lua_pushstring(L, "player number invalid!");
        lua_error(L);
        return(0);
    }

    GameClientPlayer& player = GAMECLIENT.GetPlayer(pnr);

    nobBaseWarehouse* warehouse = player.GetFirstWH();

    if (!warehouse)
    {
        lua_pushnumber(L, 0);
        return(1);
    }

    Inventory goods;

    unsigned cnt = 2;
    for (argc >>= 1; argc > 0; --argc)
    {
        unsigned type = (unsigned) luaL_checknumber(L, cnt++);
        unsigned count = (unsigned) luaL_checknumber(L, cnt++);

        if (type < WARE_TYPES_COUNT)
        {
            goods.Add(GoodType(type), count);
            player.IncreaseInventoryWare(GoodType(type), count);
        }
    }

    warehouse->AddGoods(goods);

    lua_pushnumber(L, 1);
    return(1);
}

int GameWorldBase::LUA_AddPeople(lua_State* L)
{
//  GameWorldBase *gw = static_cast<GameWorldBase*>(lua_touserdata(L, lua_upvalueindex(1)));
    int argc = lua_gettop(L) - 1;

    if ((argc < 0) || (argc % 2 == 1))
    {
        lua_pushstring(L, "wrong arguments: player, ware1, count1, ware2, count2, ...");
        lua_error(L);
        return(0);
    }

    // player
    unsigned pnr = (unsigned) luaL_checknumber(L, 1);

    if (pnr >= GAMECLIENT.GetPlayerCount())
    {
        lua_pushstring(L, "player number invalid!");
        lua_error(L);
        return(0);
    }

    GameClientPlayer& player = GAMECLIENT.GetPlayer(pnr);

    nobBaseWarehouse* warehouse = player.GetFirstWH();

    if (!warehouse)
    {
        lua_pushnumber(L, 0);
        return(1);
    }

    Inventory goods;

    unsigned cnt = 2;
    for (argc >>= 1; argc > 0; --argc)
    {
        unsigned type = (unsigned) luaL_checknumber(L, cnt++);
        unsigned count = (unsigned) luaL_checknumber(L, cnt++);

        if (type < JOB_TYPES_COUNT)
        {
            goods.Add(Job(type), count);
            player.IncreaseInventoryJob(Job(type), count);
        }
    }

    warehouse->AddGoods(goods);

    lua_pushnumber(L, 1);
    
    return(1);
}

int GameWorldBase::LUA_GetGF(lua_State *L)
{
    lua_pushnumber(L, GAMECLIENT.GetGFNumber());
    return(1);
}

int GameWorldBase::LUA_GetPlayerCount(lua_State *L)
{
    lua_pushnumber(L, GAMECLIENT.GetPlayerCount());
    return(1);
}

int GameWorldBase::LUA_GetBuildingCount(lua_State *L)
{
    if (lua_gettop(L) < 2)
    {
        lua_pushstring(L, "need player number and building type!");
        lua_error(L);
        return(0);
    }
    
    unsigned pnr = (unsigned) luaL_checknumber(L, 1);
    
    if (pnr >= GAMECLIENT.GetPlayerCount())
    {
        lua_pushstring(L, "player number invalid!");
        lua_error(L);
        return(0);
    }
    
    unsigned building_type = (unsigned) luaL_checknumber(L, 2);
    
    if (building_type >= BUILDING_TYPES_COUNT)
    {
        lua_pushstring(L, "invalid building type!");
        lua_error(L);
        return(0);
    }
    
    BuildingCount bc;
    
    GAMECLIENT.GetPlayer(pnr).GetBuildingCount(bc);
    
    lua_pushnumber(L, bc.building_counts[building_type]);
    
    return(1);
}

int GameWorldBase::LUA_GetWareCount(lua_State *L)
{
    if (lua_gettop(L) < 2)
    {
        lua_pushstring(L, "need player number and ware type!");
        lua_error(L);
        return(0);
    }
    
    unsigned pnr = (unsigned) luaL_checknumber(L, 1);
    
    if (pnr >= GAMECLIENT.GetPlayerCount())
    {
        lua_pushstring(L, "player number invalid!");
        lua_error(L);
        return(0);
    }
    
    unsigned type = (unsigned) luaL_checknumber(L, 2);
    
    if (type >= WARE_TYPES_COUNT)
    {
        lua_pushstring(L, "invalid ware type!");
        lua_error(L);
        return(0);
    }

    const Inventory& goods = GAMECLIENT.GetPlayer(pnr).GetInventory();
    
    lua_pushnumber(L, goods.goods[type]);
    
    return(1);
}

int GameWorldBase::LUA_GetPeopleCount(lua_State *L)
{
    if (lua_gettop(L) < 2)
    {
        lua_pushstring(L, "need player number and job type!");
        lua_error(L);
        return(0);
    }
    
    unsigned pnr = (unsigned) luaL_checknumber(L, 1);
    
    if (pnr >= GAMECLIENT.GetPlayerCount())
    {
        lua_pushstring(L, "player number invalid!");
        lua_error(L);
        return(0);
    }
    
    unsigned type = (unsigned) luaL_checknumber(L, 2);
    
    if (type >= JOB_TYPES_COUNT)
    {
        lua_pushstring(L, "invalid job type!");
        lua_error(L);
        return(0);
    }

    const Inventory& goods = GAMECLIENT.GetPlayer(pnr).GetInventory();
    
    lua_pushnumber(L, goods.people[type]);
    
    return(1);
}

int GameWorldBase::LUA_Log(lua_State *L)
{
    int argc = lua_gettop(L);
    
    std::string message;
    
    for (int n = 1; n <= argc; n++)
    {
        message.append(luaL_checklstring(L, n, NULL));
    }
    
    LOG.lprintf("%s\n", message.c_str());
    
    return(0);
}

int GameWorldBase::LUA_Chat(lua_State *L)
{
    int argc = lua_gettop(L);
    
    if (argc < 2)
    {
        lua_pushstring(L, "Too few arguments!");
        lua_error(L);
        return(0);
    }
    
    unsigned player = (unsigned) luaL_checknumber(L, 1);
    
    if ((player != 0xFFFFFFFF) && (unsigned) GAMECLIENT.GetPlayerID() != player)
    {
        return(0);
    }
    
    std::string message;
    
    for (int n = 2; n <= argc; n++)
    {
        message.append(luaL_checklstring(L, n, NULL));
    }
    
    GAMECLIENT.SystemChat(message);
    
    return(0);
}

int GameWorldBase::LUA_MissionStatement(lua_State *L)
{
    int argc = lua_gettop(L);
    
    if (argc < 3)
    {
        lua_pushstring(L, "Too few arguments!");
        lua_error(L);
        return(0);
    }
    
    unsigned player = (unsigned) luaL_checknumber(L, 1);
    
    if ((player != 0xFFFFFFFF) && (unsigned) GAMECLIENT.GetPlayerID() != player)
    {
        return(0);
    }
    
    std::string message;
    
    for (int n = 3; n <= argc; n++)
    {
        message.append(luaL_checklstring(L, n, NULL));
    }
    
    WINDOWMANAGER.Show(new iwMissionStatement(luaL_checklstring(L, 2, NULL), message));
    
    return(0);
}

int GameWorldBase::LUA_PostMessage(lua_State *L)
{
    int argc = lua_gettop(L);
    
    if (argc < 2)
    {
        lua_pushstring(L, "Too few arguments!");
        lua_error(L);
        return(0);
    }
    
    if ((unsigned) GAMECLIENT.GetPlayerID() != (unsigned) luaL_checknumber(L, 1))
    {
        return(0);
    }
    
    std::string message;
    
    for (int n = 2; n <= argc; n++)
    {
        message.append(luaL_checklstring(L, n, NULL));
    }
    
    GAMECLIENT.SendPostMessage(new PostMsg(message, PMC_OTHER));
    
    return(0);
}

int GameWorldBase::LUA_PostMessageWithLocation(lua_State *L)
{
    int argc = lua_gettop(L);
    
    if (argc < 4)
    {
        lua_pushstring(L, "Too few arguments!");
        lua_error(L);
        return(0);
    }
    
    if ((unsigned) GAMECLIENT.GetPlayerID() != (unsigned) luaL_checknumber(L, 1))
    {
        return(0);
    }
    
    MapCoord x = (MapCoord) luaL_checknumber(L, 2);
    MapCoord y = (MapCoord) luaL_checknumber(L, 3);
    
    std::string message;
    
    for (int n = 4; n <= argc; n++)
    {
        message.append(luaL_checklstring(L, n, NULL));
    }
    
    GAMECLIENT.SendPostMessage(new PostMsgWithLocation(message, PMC_OTHER, MapPoint(x, y)));
    
    return(0);
}

int GameWorldBase::LUA_PostNewBuildings(lua_State *L)
{
    int argc = lua_gettop(L);
    
    if (argc < 2)
    {
        lua_pushstring(L, "Too few arguments!");
        lua_error(L);
        return(0);
    }
    
    if ((unsigned) GAMECLIENT.GetPlayerID() != (unsigned) luaL_checknumber(L, 1))
    {
        return(0);
    }
    
    unsigned pnr = (unsigned) luaL_checknumber(L, 1);
    
    for (int n = 2; n <= argc; n++)
    {
        unsigned building_type = (unsigned) luaL_checknumber(L, n);
        
        if (building_type < BUILDING_TYPES_COUNT)
        {
            GAMECLIENT.SendPostMessage(new ImagePostMsgWithLocation(_(BUILDING_NAMES[building_type]), PMC_GENERAL, GAMECLIENT.GetPlayer(pnr).hqPos, (BuildingType) building_type, (Nation) GAMECLIENT.GetPlayer(pnr).nation));
        }
    }
    
    return(0);
}

int GameWorldBase::LUA_AddStaticObject(lua_State *L)
{
    GameWorldGame *gwg = dynamic_cast<GameWorldGame*>((GameWorldBase*) lua_touserdata(L, lua_upvalueindex(1)));
    
    if (!gwg)
    {
        return(0);
    }
    
    int argc = lua_gettop(L);
    
    if (argc < 3)
    {
        lua_pushstring(L, "Too few arguments!");
        lua_error(L);
        return(0);
    }
    
    MapCoord x = (MapCoord) luaL_checknumber(L, 1);
    MapCoord y = (MapCoord) luaL_checknumber(L, 2);
    unsigned id = (unsigned) luaL_checknumber(L, 3);
    MapPoint pt(x, y);
    
    unsigned file = 0xFFFF;
    unsigned size = 0;
    
    if (argc > 3)
    {
        file = (unsigned) luaL_checknumber(L, 4);
        
        if (argc > 4)
        {
            size = (unsigned) luaL_checknumber(L, 5);
            
            if (size > 2)
            {
                lua_pushstring(L, "Invalid size!");
                lua_error(L);
                return(0);
            }
        }
    }
    
    const MapNode& node = gwg->GetNode(pt);
    if (node.obj && (node.obj->GetGOT() != GOT_NOTHING) && (node.obj->GetGOT() != GOT_STATICOBJECT) && (node.obj->GetGOT() != GOT_ENVOBJECT))
    {
        lua_pushnumber(L, 0);
        return(1);
    }
    
    gwg->DestroyNO(pt, false);
    gwg->SetNO(pt, new noStaticObject(pt, id, file, size));
    gwg->RecalcBQAroundPoint(pt);
       
    lua_pushnumber(L, 1);
    return(1);
}

int GameWorldBase::LUA_AddEnvObject(lua_State *L)
{
    GameWorldGame *gwg = dynamic_cast<GameWorldGame*>((GameWorldBase*) lua_touserdata(L, lua_upvalueindex(1)));
    
    if (!gwg)
    {
        return(0);
    }
    
    int argc = lua_gettop(L);
    
    if (argc < 3)
    {
        lua_pushstring(L, "Too few arguments!");
        lua_error(L);
        return(0);
    }
    
    MapCoord x = (MapCoord) luaL_checknumber(L, 1);
    MapCoord y = (MapCoord) luaL_checknumber(L, 2);
    unsigned id = (unsigned) luaL_checknumber(L, 3);
    MapPoint pt(x, y);
    
    unsigned file = 0xFFFF;
    
    if (argc > 3)
    {
        file = (unsigned) luaL_checknumber(L, 4);
    }
    
    const MapNode& node = gwg->GetNode(pt);
    if (node.obj && (node.obj->GetGOT() != GOT_NOTHING) && (node.obj->GetGOT() != GOT_STATICOBJECT) && (node.obj->GetGOT() != GOT_ENVOBJECT))
    {
        lua_pushnumber(L, 0);
        return(1);
    }
    
    gwg->DestroyNO(pt, false);
    gwg->SetNO(pt, new noEnvObject(pt, id, file));
    gwg->RecalcBQAroundPoint(pt);   
    
    lua_pushnumber(L, 1);
    return(1);
}

int GameWorldBase::LUA_AIConstructionOrder(lua_State *L)
{
    GameWorldGame *gwg = dynamic_cast<GameWorldGame*>((GameWorldBase*) lua_touserdata(L, lua_upvalueindex(1)));
    
    if (!gwg)
    {
        return(0);
    }
    
    int argc = lua_gettop(L);
    
    if (argc < 4)//player, x, y, buildingtype
    {
        lua_pushstring(L, "Too few arguments!");
        lua_error(L);
        return(0);
    }
    
    unsigned pn = (unsigned) luaL_checknumber(L, 1);
    MapCoord x = (MapCoord) luaL_checknumber(L, 2);
    MapCoord y = (MapCoord) luaL_checknumber(L, 3);
    unsigned id = (unsigned) luaL_checknumber(L, 4);
	BuildingType bt=static_cast<BuildingType>(id);    
    
    if(!GAMECLIENT.SendAIEvent(new AIEvent::Building(AIEvent::LuaConstructionOrder, MapPoint(x, y), bt), pn))
        LOG.lprintf("Sending AIConstructionOrder to player %u failed", pn);
    
    lua_pushnumber(L, 1);
    return(1);
}

void GameWorldBase::LUA_EventExplored(unsigned player, const MapPoint pt)
{
    lua_getglobal(lua, "onExplored");

    if (lua_isfunction(lua, -1))
    {
        lua_pushnumber(lua, player);
        lua_pushnumber(lua, pt.x);
        lua_pushnumber(lua, pt.y);

        // 3 arguments, 0 return values, no error handler
        if (lua_pcall(lua, 3, 0, 0))
        {
            fprintf(stderr, "ERROR: '%s'!\n", lua_tostring(lua, -1));
            lua_pop(lua, 1);
        }
    }
    else
    {
        lua_pop(lua, 1);
    }
}

void GameWorldBase::LUA_EventOccupied(unsigned player, const MapPoint pt)
{
    lua_getglobal(lua, "onOccupied");

    if (lua_isfunction(lua, -1))
    {
        lua_pushnumber(lua, player);
        lua_pushnumber(lua, pt.x);
        lua_pushnumber(lua, pt.y);

        // 3 arguments, 0 return values, no error handler
        if (lua_pcall(lua, 3, 0, 0))
        {
            fprintf(stderr, "ERROR: '%s'!\n", lua_tostring(lua, -1));
            lua_pop(lua, 1);
        }
    }
    else
    {
        lua_pop(lua, 1);
    }
}

void GameWorldBase::LUA_EventStart()
{
    lua_getglobal(lua, "onStart");

    if (lua_isfunction(lua, -1))
    {
        // 0 arguments, 0 return values, no error handler
        if (lua_pcall(lua, 0, 0, 0))
        {
            fprintf(stderr, "ERROR: '%s'!\n", lua_tostring(lua, -1));
            lua_pop(lua, 1);
        }
    }
    else
    {
        lua_pop(lua, 1);
    }
}

void GameWorldBase::LUA_EventGF(unsigned nr)
{
    lua_getglobal(lua, "onGameFrame");

    if (lua_isfunction(lua, -1))
    {
        lua_pushnumber(lua, nr);
        
        // 1 argument, 0 return values, no error handler
        if (lua_pcall(lua, 1, 0, 0))
        {
            fprintf(stderr, "ERROR: '%s'!\n", lua_tostring(lua, -1));
            lua_pop(lua, 1);
        }
    }
    else
    {
        lua_pop(lua, 1);
    }
}

void GameWorldBase::LUA_EventResourceFound(const unsigned char player, const MapPoint pt, const unsigned char type, const unsigned char quantity)
{
    lua_getglobal(lua, "onResourceFound");

    if (lua_isfunction(lua, -1))
    {
        lua_pushnumber(lua, player);
        lua_pushnumber(lua, pt.x);
        lua_pushnumber(lua, pt.y);
        lua_pushnumber(lua, type);
        lua_pushnumber(lua, quantity);
        
        // 5 arguments, 0 return values, no error handler
        if (lua_pcall(lua, 5, 0, 0))
        {
            fprintf(stderr, "ERROR: '%s'!\n", lua_tostring(lua, -1));
            lua_pop(lua, 1);
        }
    }
    else
    {
        lua_pop(lua, 1);
    }
}


