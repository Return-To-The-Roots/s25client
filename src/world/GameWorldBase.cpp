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
#include "world/GameWorldBase.h"
#include "GameClient.h"
#include "GamePlayer.h"
#include "buildings/nobHarborBuilding.h"
#include "buildings/nobMilitary.h"
#include "figures/nofPassiveSoldier.h"
#include "nodeObjs/noMovable.h"
#include "nodeObjs/noFlag.h"
#include "BQCalculator.h"
#include "notifications/NodeNote.h"
#include "notifications/PlayerNodeNote.h"
#include "lua/LuaInterfaceGame.h"
#include "pathfinding/RoadPathFinder.h"
#include "pathfinding/FreePathFinder.h"
#include "addons/const_addons.h"
#include "gameData/TerrainData.h"
#include "gameData/MapConsts.h"
#include "gameData/GameConsts.h"
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>

GameWorldBase::GameWorldBase(const std::vector<GamePlayer>& players, const GlobalGameSettings& gameSettings, EventManager& em):
    roadPathFinder(new RoadPathFinder(*this)),
    freePathFinder(new FreePathFinder(*this)),
    players(players), gameSettings(gameSettings), em(em),
    gi(NULL)
{}

GameWorldBase::~GameWorldBase()
{}

void GameWorldBase::Init(const unsigned short width, const unsigned short height, LandscapeType lt)
{
    World::Init(width, height, lt);
    freePathFinder->Init(GetWidth(), GetHeight());
}

void GameWorldBase::InitAfterLoad()
{
    for(unsigned y = 0; y < GetHeight(); ++y)
    {
        for(unsigned x = 0; x < GetWidth(); ++x)
        {
            RecalcBQ(MapPoint(x, y));
        }
    }
}

GamePlayer& GameWorldBase::GetPlayer(const unsigned id)
{
    RTTR_Assert(id < GetPlayerCount());
    return players[id];
}

const GamePlayer& GameWorldBase::GetPlayer(const unsigned id) const
{
    RTTR_Assert(id < GetPlayerCount());
    return players[id];
}

unsigned GameWorldBase::GetPlayerCount() const
{
    return players.size();
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

    //dont build on the border
    if(GetNode(pt).boundary_stones[0])
        return false;

    for(unsigned char z = 0; z < 6; ++z)
    {
        // Roads around charburner piles are not possible
        if(GetNO(GetNeighbour(pt, z))->GetBM() == BlockingManner::NothingAround)
            return false;

        // Other roads at this point?
        if(GetPointRoad(pt, z))
            return false;
    }

    // Terrain (unterscheiden, ob Wasser und Landweg)
    if(!boat_road)
    {
        bool flagPossible = false;

        for(unsigned char i = 0; i < 6; ++i)
        {
            TerrainBQ bq = TerrainData::GetBuildingQuality(GetTerrainAround(pt, i));
            if(bq == TerrainBQ::DANGER)
                return false;
            else if(bq != TerrainBQ::NOTHING)
                flagPossible = true;
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

bool GameWorldBase::IsOnRoad(const MapPoint& pt) const
{
    for(unsigned roadDir = 0; roadDir < 3; ++roadDir)
        if(GetRoad(pt, roadDir))
            return true;
    for(unsigned roadDir = 0; roadDir < 3; ++roadDir)
        if(GetRoad(GetNeighbour(pt, Direction::fromInt(roadDir)), roadDir))
            return true;
    return false;
}

bool GameWorldBase::IsFlagAround(const MapPoint& pt) const
{
    for(unsigned i = 0; i < 6; ++i)
    {
        if(GetNO(GetNeighbour(pt, i))->GetBM() == BlockingManner::Flag)
            return true;
    }
    return false;
}

void GameWorldBase::RecalcBQForRoad(const MapPoint pt)
{
    RecalcBQ(pt);

    for(unsigned i = 3; i < 6; ++i)
        RecalcBQ(GetNeighbour(pt, i));
}

namespace{
    bool IsMilBldOfOwner(const GameWorldBase& gwb, MapPoint pt, unsigned char owner)
    {
        return gwb.IsMilitaryBuilding(pt) && (gwb.GetNode(pt).owner == owner);
    }
}

bool GameWorldBase::IsMilitaryBuildingNearNode(const MapPoint nPt, const unsigned char player) const
{
    using boost::lambda::_1;
    // Im Umkreis von 4 Punkten ein Militärgebäude suchen
    return CheckPointsInRadius(nPt, 4, boost::lambda::bind(IsMilBldOfOwner, boost::lambda::constant_ref(*this), _1, player + 1), false);
}

bool GameWorldBase::IsMilitaryBuilding(const MapPoint pt) const
{
    const noBase* obj = GetNO(pt);
    if(obj->GetType() == NOP_BUILDING || obj->GetType() == NOP_BUILDINGSITE)
    {
        BuildingType buildingType = static_cast<const noBaseBuilding*>(obj)->GetBuildingType();
        if( (buildingType >= BLD_BARRACKS && buildingType <= BLD_FORTRESS) ||
             buildingType == BLD_HEADQUARTERS ||
             buildingType == BLD_HARBORBUILDING)
            return true;
    }

    return false;
}

sortedMilitaryBlds GameWorldBase::LookForMilitaryBuildings(const MapPoint pt, unsigned short radius) const
{
    return militarySquares.GetBuildingsInRange(pt, radius);
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

noFlag* GameWorldBase::GetRoadFlag(MapPoint pt, unsigned char& dir, unsigned prevDir)
{
    // Getting a flag is const
    const noFlag* flag = const_cast<const GameWorldBase*>(this)->GetRoadFlag(pt, dir, prevDir);
    // However we self are not const, so we allow returning a non-const flag pointer
    return const_cast<noFlag*>(flag);
}

const noFlag* GameWorldBase::GetRoadFlag(MapPoint pt, unsigned char& dir, unsigned prevDir) const
{
    unsigned i = 0;

    while(true)
    {
        // suchen, wo der Weg weitergeht
        for(i = 0; i < 6; ++i)
        {
            if(i != prevDir && GetPointRoad(pt, i))
                break;
        }

        if(i == 6)
            return NULL;

        pt = GetNeighbour(pt, i);

        // endlich am Ende des Weges und an einer Flagge angekommen?
        if(GetNO(pt)->GetType() == NOP_FLAG)
        {
            dir = (i + 3) % 6;
            return GetSpecObj<noFlag>(pt);
        }
        prevDir = (i + 3) % 6;
    }
}

Point<int> GameWorldBase::GetNodePos(const MapPoint pt) const
{
    Point<int> result;
    result.x = pt.x * TR_W;
    if(pt.y & 1)
        result.x += TR_W / 2;
    result.y = pt.y * TR_H - HEIGHT_FACTOR * (GetNode(pt).altitude + 0x0A);
    return result;
}

void GameWorldBase::VisibilityChanged(const MapPoint pt, unsigned player)
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
    for(unsigned char i = 0; i < 6; ++i)
        RecalcBQ(GetNeighbour(pt, i));
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
    Visibility best_visibility = GetNode(pt).fow[player].visibility;

    if (best_visibility == VIS_VISIBLE)
        return best_visibility;

    /// Teamsicht aktiviert?
    if(GetGGS().teamView)
    {
        // Dann prüfen, ob Teammitglieder evtl. eine bessere Sicht auf diesen Punkt haben
        for(unsigned i = 0; i < GetPlayerCount(); ++i)
        {
            if(i != player && GetPlayer(i).IsAlly(player))
            {
                if(GetNode(pt).fow[i].visibility > best_visibility)
                    best_visibility = GetNode(pt).fow[i].visibility;
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
        const unsigned numHarborPts = GetHarborPointCount();
        for(unsigned i = 1; i <= numHarborPts; i++)
        {
            if(IsHarborAtSea(i, sea))
                return true;
        }
    }
    return false;
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
        if(figures.empty())
            continue;
        for(std::list<noBase*>::const_iterator it = figures.begin(); it != figures.end(); ++it)
        {
            // Ist es auch ein Figur und befindet sie sich an diesem Punkt?
            const noMovable* movable = dynamic_cast<noMovable*>(*it);
            if(movable)
            {
                if(movable->GetPos() == pt)
                    objects.push_back(*it);
            }
            else if(i == 0)
                // Den Rest nur bei den richtigen Koordinaten aufnehmen
                objects.push_back(*it);
        }
    }
    return objects;
}

template<typename T_IsHarborOk>
unsigned GameWorldBase::GetHarborInDir(const MapPoint pt, 
        const unsigned origin_harborId, const ShipDirection& dir,
        const unsigned char player, T_IsHarborOk isHarborOk) const
{
    RTTR_Assert(origin_harborId);

    // Herausfinden, in welcher Richtung sich dieser Punkt vom Ausgangspuknt unterscheidet
    unsigned char coastal_point_dir = 0xFF;
    const MapPoint hbPt = GetHarborPoint(origin_harborId);

    for(unsigned char i = 0; i < 6; ++i)
    {
        if(GetNeighbour(hbPt, i) == pt)
        {
            coastal_point_dir = i;
            break;
        }
    }

    RTTR_Assert(coastal_point_dir != 0xff);

    unsigned short seaId = GetSeaId(origin_harborId, Direction::fromInt(coastal_point_dir));
    const std::vector<HarborPos::Neighbor>& neighbors = GetHarborNeighbor(origin_harborId, dir);


    for(unsigned i = 0; i < neighbors.size(); ++i)
    {
        if(IsHarborAtSea(neighbors[i].id, seaId) && isHarborOk(neighbors[i].id))
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
bool GameWorldBase::IsHarborPointFree(const unsigned harborId, const unsigned char player) const
{
    MapPoint hbPos(GetHarborPoint(harborId));

    // Überprüfen, ob das Gebiet in einem bestimmten Radius entweder vom Spieler oder gar nicht besetzt ist außer wenn der Hafen und die Flagge im Spielergebiet liegen
    MapPoint flagPos = GetNeighbour(hbPos, 4);
    if(GetNode(hbPos).owner != player + 1 || GetNode(flagPos).owner != player + 1)
    {
        if(CheckPointsInRadius(hbPos, 4, IsPointOwnerDifferent(*this, player), false))
            return false;
    }

    return GetNode(hbPos).bq == BQ_HARBOR;
}

/// Sucht freie Hafenpunkte, also wo noch ein Hafen gebaut werden kann
unsigned GameWorldBase::GetNextFreeHarborPoint(const MapPoint pt, const unsigned origin_harborId, const ShipDirection& dir,
        const unsigned char player) const
{
    using boost::lambda::_1;
    using boost::lambda::bind;
    return GetHarborInDir(pt, origin_harborId, dir, player, bind(&GameWorldBase::IsHarborPointFree, this, _1, player));
}

/// Berechnet die Entfernung zwischen 2 Hafenpunkten
unsigned GameWorldBase::CalcHarborDistance(const unsigned habor_id1, const unsigned harborId2) const
{
    if (habor_id1 == harborId2) //special case: distance to self
        return 0;
    for(unsigned i = 0; i < 6; ++i)
    {
        const std::vector<HarborPos::Neighbor>& neighbors = GetHarborNeighbor(habor_id1, ShipDirection::fromInt(i));
        for(unsigned z = 0; z < neighbors.size(); ++z)
        {
            const HarborPos::Neighbor& n = neighbors[z];
            if(n.id == harborId2)
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
        min_distance = std::min(min_distance, this->CalcDistance(pos, GetHarborPoint(i)));

    return min_distance;
}

/// returns true when a harborpoint is in SEAATTACK_DISTANCE for figures!
bool GameWorldBase::IsAHarborInSeaAttackDistance(const MapPoint pos) const
{
    for(unsigned i = 1; i <= GetHarborPointCount(); ++i)
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
    // Erst nach Rang, an zweiter Stelle nach Entfernung sortieren
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

std::vector<unsigned> GameWorldBase::GetUsableTargetHarborsForAttack(const MapPoint targetPt, std::vector<bool>& use_seas, const unsigned char player_attacker) const
{
    // Walk to the flag of the bld/harbor. Important to check because in some locations where the coast is north of the harbor this might be blocked
    const MapPoint flagPt = GetNeighbour(targetPt, Direction::SOUTHEAST);
    std::vector<unsigned> harbor_points;
    // Check each possible harbor
    for(unsigned curHbId = 1; curHbId <= GetHarborPointCount(); ++curHbId)
    {
        const MapPoint harborPt = GetHarborPoint(curHbId);

        if(CalcDistance(harborPt, targetPt) > SEAATTACK_DISTANCE)
            continue;

        // Not attacking this harbor and harbors block?
        if(targetPt != harborPt && GetGGS().getSelection(AddonId::SEA_ATTACK) == 1)
        {
            // Does an enemy harbor exist at current harbor spot? -> Can't attack through this harbor spot
            const nobHarborBuilding* hb = GetSpecObj<nobHarborBuilding>(harborPt);
            if(hb && GetPlayer(player_attacker).IsAttackable(hb->GetPlayer()))
                continue;
        }

        // add seaIds from which we can actually attack the harbor
        bool harborinlist = false;
        for(unsigned z = 0; z < 6; ++z)
        {
            const unsigned short seaId = GetSeaId(curHbId, Direction::fromInt(z));
            if(!seaId)
                continue;
            //checks previously tested sea ids to skip pathfinding
            bool previouslytested = false;
            for(unsigned k = 0; k < z; k++)
            {
                if(seaId == GetSeaId(curHbId, Direction::fromInt(k)))
                {
                    previouslytested = true;
                    break;
                }
            }
            if(previouslytested)
                continue;

            // Can figures reach flag from coast
            const MapPoint coastalPt = GetCoastalPoint(curHbId, seaId);
            if((flagPt == coastalPt) || FindHumanPath(flagPt, coastalPt, SEAATTACK_DISTANCE) != INVALID_DIR)
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

std::vector<unsigned short> GameWorldBase::GetFilteredSeaIDsForAttack(const MapPoint targetPt, const std::vector<unsigned short>& usableSeas, const unsigned char player_attacker)const
{
    // Walk to the flag of the bld/harbor. Important to check because in some locations where the coast is north of the harbor this might be blocked
    const MapPoint flagPt = GetNeighbour(targetPt, Direction::SOUTHEAST);
    std::vector<unsigned short> confirmedSeaIds;
    // Check each possible harbor
    for(unsigned curHbId = 1; curHbId <= GetHarborPointCount(); ++curHbId)
    {
        const MapPoint harborPt = GetHarborPoint(curHbId);

        if(CalcDistance(harborPt, targetPt) > SEAATTACK_DISTANCE)
            continue;

        // Not attacking this harbor and harbors block?
        if(targetPt != harborPt && GetGGS().getSelection(AddonId::SEA_ATTACK) == 1)
        {
            // Does an enemy harbor exist at current harbor spot? -> Can't attack through this harbor spot
            const nobHarborBuilding* hb = GetSpecObj<nobHarborBuilding>(harborPt);
            if(hb && GetPlayer(player_attacker).IsAttackable(hb->GetPlayer()))
                continue;
        }

        for(unsigned z = 0; z < 6; ++z)
        {
            const unsigned short seaId = GetSeaId(curHbId, Direction::fromInt(z));
            if(!seaId)
                continue;
            // sea id is not in compare list or already confirmed? -> skip rest
            if(!helpers::contains(usableSeas, seaId) || helpers::contains(confirmedSeaIds, seaId))
                continue;

            //checks previously tested sea ids to skip pathfinding
            bool previouslytested = false;
            for(unsigned k = 0; k < z; k++)
            {
                if(seaId == GetSeaId(curHbId, Direction::fromInt(k)))
                {
                    previouslytested = true;
                    break;
                }
            }
            if(previouslytested)
                continue;

            // Can figures reach flag from coast
            MapPoint coastalPt = GetCoastalPoint(curHbId, seaId);
            if((flagPt == coastalPt) || FindHumanPath(flagPt, coastalPt, SEAATTACK_DISTANCE) != INVALID_DIR)
            {
                confirmedSeaIds.push_back(seaId);
                //all sea ids confirmed? return without changes
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
unsigned int GameWorldBase::GetNumSoldiersForSeaAttackAtSea(const unsigned char player_attacker, unsigned short seaid, bool returnCount)const
{
    // Liste alle Militärgebäude des Angreifers, die Soldaten liefern
    std::vector<nobHarborBuilding::SeaAttackerBuilding> buildings;
    unsigned int attackercount = 0;
    // Angrenzende Häfen des Angreifers an den entsprechenden Meeren herausfinden
    const std::list<nobHarborBuilding*>& harbors = GetPlayer(player_attacker).GetHarbors();
    for(std::list<nobHarborBuilding*>::const_iterator it = harbors.begin(); it != harbors.end(); ++it)
    {
        // Bestimmen, ob Hafen an einem der Meere liegt, über die sich auch die gegnerischen
        // Hafenpunkte erreichen lassen
        if(!IsHarborAtSea((*it)->GetHarborPosID(), seaid))
            continue;

        std::vector<nobHarborBuilding::SeaAttackerBuilding> tmp = (*it)->GetAttackerBuildingsForSeaIdAttack();
        buildings.insert(buildings.begin(), tmp.begin(), tmp.end());
    }

    // Die Soldaten aus allen Militärgebäuden sammeln
    for(unsigned i = 0; i < buildings.size(); ++i)
    {
        // Soldaten holen
        std::vector<nofPassiveSoldier*> tmp_soldiers = buildings[i].building->GetSoldiersForAttack(buildings[i].harbor->GetPos());

        // Überhaupt welche gefunden?
        if(tmp_soldiers.empty())
            continue;

        // Soldaten hinzufügen
        for(unsigned j = 0; j < tmp_soldiers.size(); ++j)
        {
            if(returnCount)
                attackercount++;
            else
                attackercount += (tmp_soldiers[j]->GetJobType() - 20); //private is type 21 so this increases soldiercount by 1-5 depending on rank
        }
    }
    return attackercount;
}

/// Sucht verfügbare Soldaten, um dieses Militärgebäude mit einem Seeangriff anzugreifen
std::vector<GameWorldBase::PotentialSeaAttacker> GameWorldBase::GetSoldiersForSeaAttack(const unsigned char player_attacker, const MapPoint pt) const
{
    std::vector<GameWorldBase::PotentialSeaAttacker> attackers;
    //sea attack abgeschaltet per addon?
    if(GetGGS().getSelection(AddonId::SEA_ATTACK) == 2)
        return attackers;
    // Do we have an attackble military building?
    const nobBaseMilitary* milBld = GetSpecObj<nobBaseMilitary>(pt);
    if(!milBld || !milBld->IsAttackable(player_attacker))
        return attackers;
    // Prüfen, ob der angreifende Spieler das Gebäude überhaupt sieht (Cheatvorsorge)
    if(CalcVisiblityWithAllies(pt, player_attacker) != VIS_VISIBLE)
        return attackers;
    std::vector<bool> use_seas(GetNumSeas());

    // Mögliche Hafenpunkte in der Nähe des Gebäudes
    std::vector<unsigned> defender_harbors = GetUsableTargetHarborsForAttack(pt, use_seas, player_attacker);

    // Liste alle Militärgebäude des Angreifers, die Soldaten liefern
    std::vector<nobHarborBuilding::SeaAttackerBuilding> buildings;

    // Angrenzende Häfen des Angreifers an den entsprechenden Meeren herausfinden
    const std::list<nobHarborBuilding*>& harbors = GetPlayer(player_attacker).GetHarbors();
    for(std::list<nobHarborBuilding*>::const_iterator it = harbors.begin(); it != harbors.end(); ++it)
    {
        // Bestimmen, ob Hafen an einem der Meere liegt, über die sich auch die gegnerischen
        // Hafenpunkte erreichen lassen
        bool is_at_sea = false;
        for(unsigned i = 0; i < 6; ++i)
        {
            const unsigned short seaId = GetSeaId((*it)->GetHarborPosID(), Direction::fromInt(i));
            if(seaId && use_seas[seaId - 1])
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
        std::vector<nofPassiveSoldier*> tmp_soldiers = it->building->GetSoldiersForAttack(it->harbor->GetPos());

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

void GameWorldBase::RecalcBQ(const MapPoint pt)
{
    BQCalculator calcBQ(*this);
    if(SetBQ(pt, calcBQ(pt, boost::lambda::bind(&GameWorldBase::IsOnRoad, this, boost::lambda::_1))))
        GetNotifications().publish(NodeNote(NodeNote::BQ, pt));
}
