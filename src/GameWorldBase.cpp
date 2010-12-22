// $Id: GameWorldBase.cpp 6911 2010-12-20 20:24:31Z OLiver $
//
// Copyright (c) 2005 - 2010 Settlers Freaks (sf-team at siedler25.org)
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
#include "main.h"
#include "GameWorld.h"
#include "GameObject.h"
#include "noFlag.h"
#include "RoadSegment.h"
#include "noTree.h"
#include "noBaseBuilding.h"
#include "noStaticObject.h"
#include "GameClient.h"
#include "TerrainRenderer.h"
#include "nobBaseMilitary.h"
#include "MapGeometry.h"
#include "noMovable.h"
#include "nofPassiveSoldier.h"
#include "nobHarborBuilding.h"
#include "nobMilitary.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
	#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
#endif



GameWorldBase::GameWorldBase() : gi(NULL), width(0), height(0), lt(LT_GREENLAND), handled_nodes(NULL),
									nodes(NULL)
{
	noTree::ResetInstanceCounter();
	GameObject::ResetCounter();
}

GameWorldBase::~GameWorldBase()
{
	Unload();
}

void GameWorldBase::Init()
{
	map_size = width*height;

	// Map-Knoten erzeugen
	nodes = new MapNode[map_size];
	handled_nodes = new unsigned short[map_size];
	military_squares = new list<nobBaseMilitary*>[ (width/MILITARY_SQUARE_SIZE+1) * (height/MILITARY_SQUARE_SIZE+1)];
}

MapNode::MapNode()
{

}

void GameWorldBase::Unload()
{
	// Straßen sammeln und alle dann vernichten
	list<RoadSegment*> roadsegments;
	for(unsigned i = 0;i<map_size;++i)
	{
		if(nodes[i].obj)
		{
			if(nodes[i].obj->GetGOT() == GOT_FLAG)
			{
				for(unsigned r = 0;r<6;++r)
				{
					if(static_cast<noFlag*>(nodes[i].obj)->routes[r])
					{
						if(!((roadsegments.search(static_cast<noFlag*>(nodes[i].obj)->routes[r])).valid()))
							roadsegments.push_back(static_cast<noFlag*>(nodes[i].obj)->routes[r]);
					}
				}
			}
		}
	}

	for(list<RoadSegment*>::iterator it = roadsegments.begin();it.valid();++it)
		delete (*it);


	// Objekte vernichten
	for(unsigned i = 0;i < map_size; ++i)
	{
		if(nodes[i].obj)
		{
			delete nodes[i].obj;
			nodes[i].obj = NULL;
		}

		for(unsigned z = 0;z<GameClient::inst().GetPlayerCount();++z)
		{
			if(nodes[i].fow[z].object)
			{
				delete nodes[i].fow[z].object;
				nodes[i].fow[z].object = NULL;
			}
		}
	}

	// Figuren vernichten
	for(unsigned i = 0;i<map_size;++i)
	{
		if(nodes[i].figures.size())
		{
			for(list<noBase*>::iterator it = nodes[i].figures.begin();it.valid();++it)
				delete (*it);

			nodes[i].figures.clear();
		}
	}

	catapult_stones.clear();

	delete [] nodes;
	delete [] handled_nodes;
	delete [] military_squares;

	nodes = NULL;
	handled_nodes = NULL;
	military_squares = NULL;

	map_size = 0;
}

noBase * GameWorldBase::GetNO(const MapCoord x, const MapCoord y)
{
	if(GetNode(x,y).obj)
		return GetNode(x,y).obj;
	else
		return &nothing;
}



const noBase * GameWorldBase::GetNO(const MapCoord x, const MapCoord y) const
{
	if(GetNode(x,y).obj)
		return GetNode(x,y).obj;
	else
		return &nothing;
}

const FOWObject * GameWorldBase::GetFOWObject(const MapCoord x, const MapCoord y, const unsigned spectator_player) const
{
	if(GetNode(x,y).fow[spectator_player].object)
		return GetNode(x,y).fow[spectator_player].object;
	else
		return &::nothing;
}

/// Gibt den GOT des an diesem Punkt befindlichen Objekts zurück bzw. GOT_NOTHING, wenn keins existiert
GO_Type GameWorldBase::GetGOT(const MapCoord x, const MapCoord y) const
{
	noBase * obj = GetNode(x,y).obj;
	if(obj)
		return obj->GetGOT();
	else
		return GOT_NOTHING;
}

void GameWorldBase::ConvertCoords(int x, int y, unsigned short * x_out, unsigned short * y_out) const
{
	while(x < 0)
		x += width;

	while(y < 0)
		y += height;


	x %= width;
	y %= height;

	*x_out = static_cast<unsigned short>(x);
	*y_out = static_cast<unsigned short>(y);
}

MapCoord GameWorldBase::CalcDistanceAroundBorderX(const MapCoord x1, const MapCoord x2) const
{
	int diff = int(x2) - int(x1);
	
	if(diff >= 0)
		// Differenz positiv --> nicht über den Rand, d.h. normale Distanz
		return MapCoord(diff);
	else
	{
		// Ansonten Stück bis zum Rand und das Stück vom Rand bis zu Punkt 2
		return (width-x1) + x2;
	}

}

MapCoord GameWorldBase::CalcDistanceAroundBorderY(const MapCoord y1, const MapCoord y2) const
{
	int diff = int(y2) - int(y1);
	
	if(diff >= 0)
		// Differenz positiv --> nicht über den Rand, d.h. normale Distanz
		return MapCoord(diff);
	else
	{
		// Ansonten Stück bis zum Rand und das Stück vom Rand bis zu Punkt 2
		return (width-y1) + y2;
	}
}


/// Ermittelt Abstand zwischen 2 Punkten auf der Map unter Berücksichtigung der Kartengrenzüberquerung
unsigned GameWorldBase::CalcDistance(const int x1, const int y1,
					  const int x2, const int y2) const
{
	// Jeweils kürzeste gemessene Distanz
	unsigned best = 0xffffffff;

	// In Erwägung ziehen, alle Kartenränder jeweils zu überqueren und kürzeste Distanz merken
	for(unsigned plusx = 0;plusx<3;++plusx)
	{
		for(unsigned plusy = 0;plusy<3;++plusy)
		{
			int tx1 = x1, ty1 = y1, tx2 = x2, ty2 = y2;
			if(plusx == 1)
				tx1 += width;
			else if(plusx == 2)
				tx2 += width;
			if(plusy == 1)
				ty1 += height;
			else if(plusy == 2)
				ty2 += height;

			unsigned tmp_dist = CalcRawDistance(tx1,ty1,tx2,ty2);
			if(tmp_dist < best)
				best = tmp_dist;

		}
	}

	return best;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  liefert den Straßen-Wert an der Stelle X,Y (berichtigt).
 *
 *
 *  @author OLiver
 */
unsigned char GameWorldBase::GetRoad(const MapCoord x, const MapCoord y, unsigned char dir, bool all) const
{
	assert(dir < 3);

	assert(x < width && y < height);

	unsigned pos = width * unsigned(y) + unsigned(x);

	// Entweder muss es eine richtige Straße sein oder es müssen auch visuelle Straßen erlaubt sein
	if(dir < 3)
	{
		if(nodes[pos].roads_real[(unsigned)dir] || all)
			return nodes[pos].roads[(unsigned)dir];
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  liefert den Straßen-Wert um den Punkt X,Y.
 *
 *  @author OLiver
 */
unsigned char GameWorldBase::GetPointRoad(const MapCoord x, const MapCoord y, unsigned char dir, bool all) const
{
	assert(dir < 6);

	if(dir >= 3)
		return GetRoad(x, y, dir % 3, all);
	else
		return GetRoad(GetXA(x, y, dir), GetYA(x, y, dir), dir, all);
}

unsigned char GameWorldBase::GetPointFOWRoad(MapCoord x, MapCoord y, unsigned char dir, const unsigned char viewing_player) const
{
	if(dir >= 3)
		dir = dir - 3;
	else
	{
		x = GetXA(x,y,dir);
		y = GetYA(x,y,dir);
	}

	return GetNode(x,y).fow[viewing_player].roads[dir];
}

bool GameWorldBase::IsPlayerTerritory(const MapCoord x, const MapCoord y) const
{
	unsigned char owner = GetNode(x,y).owner;

	// Umliegende Punkte dürfen keinem anderen gehören
	for(unsigned i = 0;i<6;++i)
	{
		if(GetNodeAround(x,y,i).owner != owner)
			return false;
	}

	return true;
}

bool GameWorldBase::RoadAvailable(const bool boat_road,const int x, const int y,unsigned char to_dir,const bool visual) const
{
	// Hindernisse
	if(GetNode(x,y).obj)
	{
		noBase::BlockingManner bm = GetNode(x,y).obj->GetBM();
		if(bm != noBase::BM_NOTBLOCKING)
			 return 0;
	}

	

	for(unsigned char z = 0;z<6;++z)
	{
		// Roads around charburner piles are not possible
		if(GetNO(GetXA(x,y,z),GetYA(x,y,z))->GetBM() == noBase::BM_CHARBURNERPILE)
			return 0;

		// Other roads at this point?
		if(GetPointRoad(x,y, z, visual))
		{
			unsigned r;
			r = GetPointRoad(x,y, z, visual);
			return 0;
		}
	}


	for(unsigned char i = 3;i<6;++i)
	{
		if(GetNO(GetXA(x,y,i),GetYA(x,y,i))->GetBM() == noBase::BM_CASTLE)		
			return 0;
	}

	// Terrain (unterscheidne, ob Wasser und Landweg)
	if(!boat_road)
	{
		unsigned flag_hits = 0;
		unsigned char t;

		for(unsigned char i = 0;i<6;++i)
		{
			t = GetTerrainAround(x,y,i);
			if(TERRAIN_BQ[t] == BQ_CASTLE || TERRAIN_BQ[t] == BQ_CASTLE || TERRAIN_BQ[t] == BQ_MINE || TERRAIN_BQ[t] == BQ_FLAG) ++flag_hits;
			else if(TERRAIN_BQ[t] == BQ_DANGER)
				return 0;
		}

		if(!flag_hits)
			return 0;

		// Richtung übergeben? Dann auch das zwischen den beiden Punkten beachten, damit
		// man nicht über ein Wasser oder so hüpft
		if(to_dir != 0xFF)
		{
			// Richtung genau entgegengesetzt, da das ja hier der Zielpunkt ist, wir müssen wieder zurück zum Quellpunkt
			to_dir = (to_dir+3)%6;

			//// Nicht über Wasser, Lava, Sümpfe gehen
			//if(!IsNodeToNodeForFigure(x,y,to_dir,boat_road))
			//	return false;
		}

		return true;
	}
	else
	{
		// Beim Wasserweg muss um den Punkt herum Wasser sein
		for(unsigned i = 0;i<6;++i)
			if(GetTerrainAround(x,y,i) != 14)
				return false;
	}

	return true;
}

bool GameWorldBase::RoadAlreadyBuilt(const bool boat_road, unsigned short start_x, unsigned short start_y, const std::vector<unsigned char>& route)
{
	int tx = start_x;
	int ty = start_y;
	for(unsigned i = 0;i<route.size()-1;++i)
	{
		// Richtiger Weg auf diesem Punkt?
		if(!GetPointRoad(tx,ty, route[i]))
			return false;

		int tmpx = tx;
		tx = GetXA(tx,ty,route[i]);
		ty = GetYA(tmpx,ty,route[i]);
	}
	return true;
}


bool GameWorldBase::FlagNear(const int x, const int y) const
{
	for(unsigned char i = 0;i<6;++i)
	{
		int ya = GetYA(x,y,i);
		int xa = GetXA(x,y,i);

		if(GetNO(xa,ya)->GetType() == NOP_FLAG)
			return 1;
	}
	return 0;
}

void GameWorldBase::CalcRoad(const MapCoord x, const MapCoord y,const unsigned char player)
{
	SetBQ(x,y,GAMECLIENT.GetPlayerID());
	
	for(unsigned i = 3;i<6;++i)
		SetBQ(GetXA(x,y,i),GetYA(x,y,i),GAMECLIENT.GetPlayerID());
}

bool GameWorldBase::IsMilitaryBuildingNearNode(const MapCoord nx, const MapCoord ny) const
{
	// Im Umkreis von 4 Punkten ein Militärgebäude suchen
	MapCoord x = nx,y = ny;

	for(int r = 1;r<=4;++r)
	{
		// Eins weiter nach links gehen
		this->GetPointA(x,y,0);
		
		for(unsigned dir = 0;dir<6;++dir)
		{
			for(unsigned short i = 0;i<r;++i)
			{
				if(IsMilitaryBuilding(x,y))
					return true;
				// Nach rechts oben anfangen
				this->GetPointA(x,y,(2+dir)%6);
			}
		}
	}

	// Keins gefunden
	return false;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  setzt den virtuellen Straßen-Wert an der Stelle X,Y (berichtigt).
 *
 *  @author OLiver
 */
void GameWorldBase::SetVirtualRoad(const MapCoord x, const MapCoord y, unsigned char dir, unsigned char type)
{
	assert(dir < 3);

	unsigned pos = width * unsigned(y) + unsigned(x);

	nodes[pos].roads[dir] = type;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  setzt den virtuellen Straßen-Wert um den Punkt X,Y.
 *
 *  @author OLiver
 */
void GameWorldBase::SetPointVirtualRoad(const MapCoord x, const MapCoord y, unsigned char dir, unsigned char type)
{
	assert(dir < 6);

	if(dir >= 3)
		SetVirtualRoad(x, y, dir - 3, type);
	else
		SetVirtualRoad(GetXA(x, y, dir), GetYA(x, y, dir), dir, type);
}




bool GameWorldBase::IsMilitaryBuilding(const MapCoord x, const MapCoord y) const
{
	if(GetNO(x,y)->GetType() == NOP_BUILDING || GetNO(x,y)->GetType() == NOP_BUILDINGSITE)
	{
		if( (GetSpecObj<noBaseBuilding>(x,y)->GetBuildingType() >= BLD_BARRACKS &&
			GetSpecObj<noBaseBuilding>(x,y)->GetBuildingType() <= BLD_FORTRESS) ||
			GetSpecObj<noBaseBuilding>(x,y)->GetBuildingType() == BLD_HEADQUARTERS ||
			GetSpecObj<noBaseBuilding>(x,y)->GetBuildingType() == BLD_HARBORBUILDING)
			return true;
	}


	return false;
}

void GameWorldBase::LookForMilitaryBuildings(std::list<nobBaseMilitary*>& buildings,const MapCoord x, const MapCoord y, unsigned short radius) const
{
	// Radius auf Anzahl der Militärquadrate begrenzen, sonst gibt es Überlappungen
	radius = min<MapCoord>(width/MILITARY_SQUARE_SIZE+1,radius);
	
	// in Militärquadrat-Koordinaten umwandeln-
	int first_x = x/MILITARY_SQUARE_SIZE;
	int first_y = y/MILITARY_SQUARE_SIZE;

	// linkes, oberes Quadrat ermitteln, dabei aufpassen dass wir nicht unter 0 geraden
	first_x -= radius;
	first_y -= radius; 

	// in Militärquadrat-Koordinaten umwandeln
	unsigned short last_x = x/MILITARY_SQUARE_SIZE;
	unsigned short last_y = y/MILITARY_SQUARE_SIZE;

	// rechtes unteres Quadrat ermitteln, dabei nicht über die Karte hinausgehen
	last_x += radius;
	last_y += radius;

	// Liste erzeugen
	for(int cy = first_y;cy<=last_y;++cy)
	{
		MapCoord ty;
		if(cy < 0) ty = (cy + 2*(height/MILITARY_SQUARE_SIZE+1)) % (height/MILITARY_SQUARE_SIZE + 1);
		else if(cy >= height/MILITARY_SQUARE_SIZE+1) ty = cy % (height/MILITARY_SQUARE_SIZE + 1);
		else ty = cy;
		for(int cx = first_x;cx<=last_x;++cx)
		{
			MapCoord tx;
			if(cx < 0) tx = cx + width/MILITARY_SQUARE_SIZE+1;
			else if(cx >= width/MILITARY_SQUARE_SIZE+1) tx = cx - width/MILITARY_SQUARE_SIZE - 1;
			else tx = cx;

			for(list<nobBaseMilitary*>::iterator it = military_squares[ty*(width/MILITARY_SQUARE_SIZE+1)+tx].begin();it.valid();++it)
			{
				// Jedes Militärgebäude nur einmal hinzufügen
				if(std::find(buildings.begin(),buildings.end(),*it) == buildings.end())
					buildings.push_back(*it);
			}
		}
	}
}


/// Baut eine (bisher noch visuell gebaute) Straße wieder zurück
void GameWorldBase::RemoveVisualRoad(unsigned short start_x, unsigned short start_y, const std::vector<unsigned char>& route)
{
	// Wieder zurückbauen
	for(unsigned z = 0;z<route.size();++z)
	{
		SetPointVirtualRoad(start_x,start_y, route[z],0);
		CalcRoad(start_x,start_y,GAMECLIENT.GetPlayerID());
		GetPointA(start_x,start_y,route[z]);
		
	}
}

BuildingQuality GameWorldBase::CalcBQ(const MapCoord x, const MapCoord y,const unsigned char player,const bool flagonly,const bool visual, const bool ignore_player) const
{

	///////////////////////
	// 1. nach Terrain

	// Unser Land?
	if(!ignore_player && (GetNode(x,y).owner-1 != player  || !IsPlayerTerritory(x,y)))
		return BQ_NOTHING;

	unsigned building_hits = 0;
	unsigned mine_hits = 0;
	unsigned flag_hits = 0;
	BuildingQuality val = BQ_CASTLE;
	unsigned char t;

	// bebaubar?
	for(unsigned char i = 0;i<6;++i)
	{
		t = GetTerrainAround(x,y,i);
		if(TERRAIN_BQ[t] == BQ_CASTLE) ++building_hits;
		else if(TERRAIN_BQ[t] == BQ_MINE) ++mine_hits;
		else if(TERRAIN_BQ[t] == BQ_FLAG) ++flag_hits;
		else if(TERRAIN_BQ[t] == BQ_DANGER) return BQ_NOTHING;
	}

	if(flag_hits)
		val = BQ_FLAG;
	else if(mine_hits == 6)
		val = BQ_MINE;
	else if(mine_hits)
		val = BQ_FLAG;
	else if(building_hits == 6)
		val = BQ_CASTLE;
	else if(building_hits)
		val = BQ_FLAG;
	else
		return BQ_NOTHING;


	//////////////////////////////////////
	// 2. nach Terrain

	unsigned char ph = GetNode(x,y).altitude, th;

	// Bergwerke anders handhaben
	if(val == BQ_CASTLE && val != BQ_FLAG)
	{

		if((th=GetNodeAround(x,y,4).altitude) > ph)
		{
			if(th - ph > 1)
				val =	BQ_FLAG;
		}

		// 2. Außenschale prüfen ( keine Hütten werden ab Steigung 3 )
		for(unsigned i = 0;i<12;++i)
		{
			if( (th = GetNode(GetXA2(x,y,i),GetYA2(x,y,i)).altitude ) > ph)
			{
				if(th - ph > 2)
				{
					val = BQ_HUT;
					break;
				}
			}

			if( (th = GetNode(GetXA2(x,y,i), GetYA2(x,y,i)).altitude ) < ph)
			{
				if(ph - th > 2)
				{
					val = BQ_HUT;
					break;
				}
			}
		}

		// 1. Auäcnschale ( käcnen Flaggen werden ab Steigung 4)
		for(unsigned i = 0;i<6;++i)
		{
			if((th=GetNodeAround(x,y,i).altitude) > ph)
			{
				if(th - ph > 3)
					val = BQ_FLAG;
			}

			if((th=GetNodeAround(x,y,i).altitude) < ph)
			{
				if(ph - th > 3)
					val = BQ_FLAG;
			}
		}
	}
	else
	{
		for(unsigned i = 0;i<6;++i)
		{
			if(i > 3 && i!=5 && (th=GetNodeAround(x,y,i).altitude) > ph)
			{
				if(th - ph > 3)
					val = BQ_FLAG;
			}
		}
	}

	//////////////////////////////////////////
	// 3. nach Objekten

	 if(flagonly) if(FlagNear(x,y)) return BQ_NOTHING;


	// allgemein nix bauen auf folgenden Objekten:

	 if(GetNO(x,y)->GetBM() != noBase::BM_NOTBLOCKING)
		return BQ_NOTHING;

	// Don't build anything around charburner piles
	for(unsigned i = 0;i<6;++i)
	{
		if(GetNO(GetXA(x,y,i),GetYA(x,y,i))->GetBM() == noBase::BM_CHARBURNERPILE)
			return BQ_NOTHING;
	}

	if(val > 2 && val != BQ_MINE)
	{
		for(unsigned i = 0;i<6;++i)
		{
			// Baum --> rundrum Hütte
			if(GetNO(GetXA(x,y,i),GetYA(x,y,i))->GetType() == NOP_TREE)
			{
				val = BQ_HUT;
				break;
			}

			/*// StaticObject --> rundrum Flagge/Hütte
			else if(GetNO(GetXA(x,y,i),GetYA(x,y,i))->GetType() == NOP_OBJECT)
			{
				const noStaticObject *obj = GetSpecObj<noStaticObject>(GetXA(x,y,i),GetYA(x,y,i));
				if(obj->GetSize() == 2)
					val = BQ_FLAG;
				else
					val = BQ_HUT;

				break;
			}*/
		}
	}

	// Stein, Feuer und Getreidefeld --> rundrum Flagge
	for(unsigned i = 0;i<6;++i)
	{
		const noBase * nob = GetNO(GetXA(x,y,i),GetYA(x,y,i));
		if(nob->GetBM() == noBase::BM_GRANITE)
		{
			val = BQ_FLAG;
			break;
		}
	}

	// Flagge
	if(val == BQ_CASTLE)
	{
		for(unsigned char i = 0;i<3;++i)
		{
			if(GetNodeAround(x,y,i).obj)
			{
				if(GetNodeAround(x,y,i).obj->GetBM() == noBase::BM_FLAG)
					val = BQ_HOUSE;
			}
		}
	}

	if(GetNO(GetXA(x,y,3),GetYA(x,y,3))->GetBM() == noBase::BM_FLAG)
		return BQ_NOTHING;
	if(GetNO(GetXA(x,y,5),GetYA(x,y,5))->GetBM() == noBase::BM_FLAG)
		return BQ_NOTHING;


	if(val != BQ_FLAG)
	{
		if(GetNO(GetXA(x,y,5),GetYA(x,y,5))->GetBM() == noBase::BM_FLAG)
			val = BQ_FLAG;
	}

	// Gebäude
	if(val == BQ_CASTLE)
	{
		for(unsigned i = 0;i<12;++i)
		{
			noBase::BlockingManner bm = GetNO(GetXA2(x,y,i),GetYA2(x,y,i))->GetBM();

			if(bm >= noBase::BM_HUT && bm <= noBase::BM_MINE)
				val = BQ_HOUSE;
		}
	}

	for(unsigned i = 0;i<3;++i)
	{
		if(val == BQ_CASTLE)
		{
			for(unsigned char c = 0;c<6;++c)
			{
				if(GetPointRoad(GetXA(x,y,i),GetYA(x,y,i), c, visual))
				{
					val = BQ_HOUSE;
					break;
				}
			}
		}
	}

	for(unsigned char c = 0;c<6;++c)
	{
		if(GetPointRoad(x,y, c, visual))
		{
			val = BQ_FLAG;
			break;
		}
	}

	if(val == BQ_FLAG)
	{
		for(unsigned char i = 0;i<6;++i)
		{
			if(GetNO(GetXA(x,y,i),GetYA(x,y,i))->GetBM() == noBase::BM_FLAG)
				return BQ_NOTHING;
		}
	}


	if(flagonly)
		return BQ_FLAG;

	if(val == BQ_FLAG)
	{
		for(unsigned char i = 0;i<3;++i)
			if(GetNO(GetXA(x,y,i),GetYA(x,y,i))->GetBM() == noBase::BM_FLAG)
				return BQ_NOTHING;
	}


	// Schloss bis hierhin und ist hier ein Hafenplatz?
	if(val == BQ_CASTLE && GetNode(x,y).harbor_id)
		// Dann machen wir einen Hafen draus
		val = BQ_HARBOR;

	if(val >= BQ_HUT && val <= BQ_HARBOR)
	{
		if(GetNO(GetXA(x,y,4),GetYA(x,y,4))->GetBM() == noBase::BM_FLAG)
			return val;

		if(CalcBQ(GetXA(x,y,4),GetYA(x,y,4),player,true,visual,ignore_player))
		{
			return val;
		}
		else
		{

			for(unsigned char i = 0;i<3;++i)
				if(GetNO(GetXA(x,y,i),GetYA(x,y,i))->GetBM() == noBase::BM_FLAG)
					return BQ_NOTHING;
			return BQ_FLAG;
		}
	}


	return val;
}

bool GameWorldBase::IsNodeToNodeForFigure(const MapCoord x, const MapCoord y, const unsigned dir) const
{
	// Nicht über Wasser, Lava, Sümpfe gehen
	// Als Boot dürfen wir das natürlich
	unsigned char t1 = GetWalkingTerrain1(x,y,dir),
		t2 = GetWalkingTerrain2(x,y,dir);

	// Wenn ein Weg da drüber geht, dürfen wir das sowieso, aber kein Wasserweg!
	unsigned char road = GetPointRoad(x,y,dir);
	if(road && road != RoadSegment::RT_BOAT+1)
		return true;

	if((t1 == TT_SNOW || t1 == TT_SWAMPLAND || t1 == TT_LAVA || (t1 == TT_WATER)) && 
	   (t2 == TT_SNOW || t2 == TT_SWAMPLAND || t2 == TT_LAVA || (t2 == TT_WATER )))
		return false;
	else
		return true;
}

noFlag * GameWorldBase::GetRoadFlag(int x, int y,unsigned char& dir,unsigned last_i)
{
	unsigned char i = 0;

	while(true)
	{
		// suchen, wo der Weg weitergeht
		for(i = 0;i<6;++i)
		{
			if(GetPointRoad(x,y,i) && i != last_i)
				break;
		}

		if(i == 6)
			return 0;

		int tx = x,ty = y;
		x = GetXA(tx,ty,i);
		y = GetYA(tx,ty,i);

		// endlich am Ende des Weges und an einer Flagge angekommen?
		if(GetNO(x,y)->GetType() == NOP_FLAG)
		{
			dir = (i+3)%6;
			return GetSpecObj<noFlag>(x,y);
		}
		last_i = (i+3)%6;
	}
}




///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
unsigned short GameWorldBase::GetXA(const MapCoord x, const MapCoord y, unsigned dir) const
{
	assert(dir < 6);

	int tx = x;

	switch(dir)
	{
	case 0: tx = x - 1; break;
	case 1: tx = x - !(y&1); break;
	case 2: tx = x + (y&1); break;
	case 3: tx = x + 1; break;
	case 4: tx = x + (y&1); break;
	case 5: tx = x - !(y&1); break;
	}

	unsigned short rx,ry;
	ConvertCoords(tx,int(y),&rx,&ry);

	return rx;
}

void GameWorldBase::GetPointA(MapCoord& x, MapCoord& y, unsigned dir) const
{
	// Kopie speichern
	MapCoord tx = x, ty = y;

	// Jeweils umwandeln, Kopie benutzen, damit das ganze nicht durch vorher 
	// umgwandelte x-Koordinate verfälscht wird bei Y
	x = GetXA(tx,ty,dir);
	y = GetYA(tx,ty,dir);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
unsigned short GameWorldBase::GetYA(const MapCoord x, const MapCoord y, unsigned dir) const
{
	assert(dir < 6);

	int ty=y;

	switch(dir)
	{
	default: break;
	case 1:
	case 2: --ty; break;
	case 4:
	case 5: ++ty; break;
	}

	unsigned short rx,ry;
	ConvertCoords(int(x),ty,&rx,&ry);

	return ry;
}

/// Wie GetXA, bloß 2. Außenschale (dir zwischen 0 bis 11)
MapCoord GameWorldBase::GetXA2(const MapCoord x, const MapCoord y, unsigned dir) const
{
	int tx; 

	switch(dir)
	{
	default: assert(false); tx = 0xFFFF;
	case 0: tx = x-2; break;
	case 1: tx = x-2+(y&1); break;
	case 2: tx = x-1; break;
	case 3: tx = x; break;
	case 4: tx = x+1; break;
	case 5: tx = x+2-!(y&1); break;
	case 6: tx = x+2; break;
	case 7: tx = x-2+(y&1); break;
	case 8: tx = x-1; break;
	case 9: tx = x; break;
	case 10: tx = x+1; break;
	case 11: tx = x+2-!(y&1);
	}


	unsigned short rx,ry;
	ConvertCoords(tx,int(y),&rx,&ry);

	return rx;
}

/// Wie GetYA, bloß 2. Außenschale (dir zwischen 0 bis 11)
MapCoord GameWorldBase::GetYA2(const MapCoord x, const MapCoord y, unsigned dir) const
{
	assert(dir < 12);

	static const int ADD_Y[12] =
	{ 0, -1, -2, -2, -2, -1, 0, 1, 2, 2, 2, 1 };


	unsigned short rx,ry;
	ConvertCoords(int(x),int(y)+ADD_Y[dir],&rx,&ry);

	return ry;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  liefert das Terrain um den Punkt X,Y.
 *
 *  @author OLiver
 *  @author FloSoft
 */
unsigned char GameWorldBase::GetTerrainAround(int x, int y, unsigned char dir)  const
{
	assert(dir < 6);

	switch(dir)
	{
	case 0: return GetNodeAround(x,y,1).t1;
	case 1: return GetNodeAround(x,y,1).t2;
	case 2: return GetNodeAround(x,y,2).t1;
	case 3: return GetNode(x,y).t2;
	case 4: return GetNode(x,y).t1;
	case 5: return GetNodeAround(x,y,0).t2;
	}

	return 0xFF;
}


///////////////////////////////////////////////////////////////////////////////
/**
 *  Gibt das Terrain zurück, über das ein Mensch/Tier laufen müsste, von X,Y
 *  in Richtung DIR (Vorwärts).
 *
 *  @author OLiver
 */
unsigned char GameWorldBase::GetWalkingTerrain1(MapCoord x, MapCoord y, unsigned char dir)  const
{
	assert(dir < 6);

	switch(dir)
	{
	case 0: return GetTerrainAround(x,y,5);
	case 1: return GetTerrainAround(x,y,0);
	case 2: return GetTerrainAround(x,y,1);
	case 3: return GetTerrainAround(x,y,2);
	case 4: return GetTerrainAround(x,y,3);
	case 5: return GetTerrainAround(x,y,4);
	}

	return 0xFF;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Gibt das Terrain zurück, über das ein Mensch/Tier laufen müsste, von X,Y
 *  in Richtung DIR (Rückwärts).
 *
 *  @author OLiver
 */
unsigned char GameWorldBase::GetWalkingTerrain2(MapCoord x, MapCoord y, unsigned char dir)  const
{
	assert(dir < 6);

	switch(dir)
	{
	case 0: return GetTerrainAround(x,y,0);
	case 1: return GetTerrainAround(x,y,1);
	case 2: return GetTerrainAround(x,y,2);
	case 3: return GetTerrainAround(x,y,3);
	case 4: return GetTerrainAround(x,y,4);
	case 5: return GetTerrainAround(x,y,5);
	}

	return 0xFF;
}

/// Gibt zurück, ob ein Punkt vollständig von Wasser umgeben ist
bool GameWorldBase::IsSeaPoint(MapCoord x, MapCoord y) const
{
	for(unsigned i = 0;i<6;++i)
	{
		if(GetTerrainAround(x,y,i) != TT_WATER)
			return false;
	}
	
	return true;
}

/// Verändert die Höhe eines Punktes und die damit verbundenen Schatten
void GameWorldBase::ChangeAltitude(const MapCoord x, const MapCoord y, const unsigned char altitude)
{
	// Höhe verändern
	GetNode(x,y).altitude = altitude;

	// Schattierung neu berechnen von diesem Punkt und den Punkten drumherum
	RecalcShadow(x,y);
	for(unsigned i = 0;i<6;++i)
		RecalcShadow(GetXA(x,y,i),GetYA(x,y,i));

	// Baumöglichkeiten neu berechnen
	// Direkt drumherum
	for(unsigned i = 0;i<6;++i)
		SetBQ(GetXA(x,y,i),GetYA(x,y,i),GameClient::inst().GetPlayerID());
	// noch eine Schale weiter außen
	for(unsigned i = 0;i<12;++i)
		SetBQ(GetXA2(x,y,i),GetYA2(x,y,i),GameClient::inst().GetPlayerID());

	// Abgeleiteter Klasse Bescheid sagen
	AltitudeChanged(x,y);
}

void GameWorldBase::RecalcShadow(const MapCoord x, const MapCoord y)
{
	const int SHADOW_COEFFICIENT = 6;

	// Normale Ausleuchtung
	int shadow = 0x40;

	// Höhendifferenz zu den Punkten darum betrachten, auf der einen Seite entsprechend heller, wenn höher, sonst dunkler
	shadow+=(SHADOW_COEFFICIENT*(GetNode(x,y).altitude-GetNodeAround(x,y,0).altitude));
	shadow+=(SHADOW_COEFFICIENT*(GetNode(x,y).altitude-GetNodeAround(x,y,5).altitude));
	shadow+=(SHADOW_COEFFICIENT*(GetNode(x,y).altitude-GetNodeAround(x,y,4).altitude));

	// und hier genau umgekehrt
	shadow-=(SHADOW_COEFFICIENT*(GetNode(x,y).altitude-GetNodeAround(x,y,1).altitude));
	shadow-=(SHADOW_COEFFICIENT*(GetNode(x,y).altitude-GetNodeAround(x,y,3).altitude));
	shadow-=(SHADOW_COEFFICIENT*(GetNode(x,y).altitude-GetNodeAround(x,y,3).altitude));

	// Zu niedrig? Zu hoch? --> extreme Werte korrigieren
	if(shadow < 0x00)
		shadow = 0x00;
	else if(shadow > 0x60)
		shadow = 0x60;

	GetNode(x,y).shadow = static_cast<unsigned char>(shadow);
}

Visibility GameWorldBase::CalcWithAllyVisiblity(const MapCoord x, const MapCoord y, const unsigned char player) const
{
	Visibility best_visibility = GetNode(x,y).fow[player].visibility;

	if (best_visibility == VIS_VISIBLE)
		return best_visibility;

	/// Teamsicht aktiviert?
	if(GameClient::inst().GetGGS().team_view)
	{
		// Dann prüfen, ob Teammitglieder evtl. eine bessere Sicht auf diesen Punkt haben
		for(unsigned i = 0;i<GameClient::inst().GetPlayerCount();++i)
		{
			if(GameClient::inst().GetPlayer(i)->IsAlly(player))
			{
				if(GetNode(x,y).fow[i].visibility > best_visibility)
					best_visibility = GetNode(x,y).fow[i].visibility;
			}
		}
	}

	return best_visibility;
}


/// Ermittelt, ob ein Punkt Küstenpunkt ist, d.h. Zugang zu einem schiffbaren Meer hat
unsigned short GameWorldBase::IsCoastalPoint(const MapCoord x, const MapCoord y) const
{
	// Punkt muss selbst zu keinem Meer gehören
	if(GetNode(x,y).sea_id)
		return 0;

	// Um den Punkt herum muss ein gültiger Meeres Punkt sein
	for(unsigned i = 0;i<6;++i)
	{
		if(unsigned short sea_id = GetNodeAround(x,y,i).sea_id)
		{
			// Dieses Meer schiffbar (todo: andere Kritierien wie Hafenplätze etc.)?
			if(seas[GetNodeAround(x,y,i).sea_id].nodes_count > 20)
				return sea_id;
		}
	}

	return false;
}

/// Gibt Dynamische Objekte, die von einem bestimmten Punkt aus laufen oder dort stehen sowie andere Objekte,
/// die sich dort befinden, zurück
void GameWorldBase::GetDynamicObjectsFrom(const MapCoord x, const MapCoord y,list<noBase*>& objects) const
{
	// Auch über und unter dem Punkt gucken, da dort auch die Figuren hängen können!
	const unsigned short coords[6] =
	{
		x,y,
		GetXA(x,y,1),GetYA(x,y,1),
		GetXA(x,y,2),GetYA(x,y,2)
	};

	for(unsigned i = 0;i<3;++i)
	{
		for(list<noBase*>::iterator it = GetFigures(coords[i*2],coords[i*2+1]).begin();
			it.valid();++it)
		{
			// Ist es auch ein Figur und befindet sie sich an diesem Punkt?
			if((*it)->GetType() == NOP_FIGURE || (*it)->GetGOT() == GOT_ANIMAL || (*it)->GetGOT() == GOT_SHIP)
			{
				if(static_cast<noMovable*>(*it)->GetX() == x && static_cast<noMovable*>(*it)->GetY() == y)
					objects.push_back(*it);
			}
			else if(i == 0)
				// Den Rest nur bei den richtigen Koordinaten aufnehmen
				objects.push_back(*it);

		}
	}
}


/// Grenzt der Hafen an ein bestimmtes Meer an?
bool GameWorldBase::IsAtThisSea(const unsigned harbor_id, const unsigned short sea_id) const
{
	for(unsigned i = 0;i<6;++i)
	{
		if(sea_id == harbor_pos[harbor_id].cps[i].sea_id)
			return true;
	}
	return false;
}

/// Gibt die Koordinaten eines bestimmten Hafenpunktes zurück
Point<MapCoord> GameWorldBase::GetHarborPoint(const unsigned harbor_id) const
{
	assert(harbor_id);

	return Point<MapCoord>(harbor_pos[harbor_id].x,harbor_pos[harbor_id].y);
}

/// Gibt den Punkt eines bestimmtes Meeres um den Hafen herum an, sodass Schiffe diesen anfahren können
void GameWorldBase::GetCoastalPoint(const unsigned harbor_id, MapCoord * px, MapCoord * py, const unsigned short sea_id) const
{
	assert(harbor_id);

	for(unsigned i = 0;i<6;++i)
	{
		if(harbor_pos[harbor_id].cps[i].sea_id == sea_id)
		{
			*px = GetXA(harbor_pos[harbor_id].x,harbor_pos[harbor_id].y,i);
			*py = GetYA(harbor_pos[harbor_id].x,harbor_pos[harbor_id].y,i);
			return;
		}
	}

	// Keinen Punkt gefunden
	*px = 0xFFFF;
	*py = 0xFFFF;
}


/// Gibt nächsten Hafenpunkt in einer bestimmten Richtung zurück, bzw. 0, jwenn es keinen gibt 
unsigned GameWorldBase::GetNextHarborPoint(const MapCoord x, const MapCoord y, 
											const unsigned origin_harbor_id, const unsigned char dir,  
											const unsigned char player, 
	bool (GameWorldBase::*IsPointOK)(const unsigned, const unsigned char, const unsigned short) const) const
{

	//unsigned char group_id = harbor_pos[origin_harbor_id-1].cps[

	// Herausfinden, in welcher Richtung sich dieser Punkt vom Ausgangspuknt unterscheidet
	unsigned char coastal_point_dir = 0xFF;

	for(unsigned char i = 0;i<6;++i)
	{
		if(GetXA(harbor_pos[origin_harbor_id].x,harbor_pos[origin_harbor_id].y,i) == x &&
			GetYA(harbor_pos[origin_harbor_id].x,harbor_pos[origin_harbor_id].y,i) == y)
		{
			coastal_point_dir = i;
			break;
		}
	}

	assert(coastal_point_dir != 0xff);

	unsigned short sea_id = harbor_pos[origin_harbor_id].cps[coastal_point_dir].sea_id;


	for(unsigned i = 0;i<harbor_pos[origin_harbor_id].neighbors[dir].size();++i)
	{
		// Entspricht der Punkt meinen Erwartungen?
		if((this->*IsPointOK)(harbor_pos[origin_harbor_id].neighbors[dir][i].id,player,sea_id))
		{
		
			// Dann nehmen wir den doch
			return harbor_pos[origin_harbor_id].neighbors[dir][i].id;
		}
	}

	// Nichts gefunden
	return 0;
}

/// Ist es an dieser Stelle für einen Spieler möglich einen Hafen zu bauen
bool GameWorldBase::IsHarborPointFree(const unsigned harbor_id, const unsigned char player, const unsigned short sea_id) const
{
	Point<MapCoord> coords(GetHarborPoint(harbor_id));

	// Befindet sich der Hafenpunkt auch an dem erforderlichen Meer?
	bool at_sea = false;
	for(unsigned i = 0;i<6;++i)
	{
		if(harbor_pos[harbor_id].cps[i].sea_id == sea_id)
		{
			at_sea = true;
			break;
		}
	}

	if(!at_sea)
		return false;

	// Überprüfen, ob das Gebiet in einem bestimmten Radius entweder vom Spieler oder gar nicht besetzt ist
	for(MapCoord tx=GetXA(coords.x,coords.y,0), r=1;r<=4;tx=GetXA(tx,coords.y,0),++r)
	{
		MapCoord tx2 = tx, ty2 = coords.y;
		for(unsigned i = 2;i<8;++i)
		{
			for(MapCoord r2=0;r2<r;GetPointA(tx2,ty2,i%6),++r2)
			{
				unsigned char owner = GetNode(tx2,ty2).owner;
				if(owner != 0 && owner != player+1)
					return false;
			}
		}
	}


	return (CalcBQ(coords.x,coords.y,0,false,false,true) == BQ_HARBOR);
}

/// Sucht freie Hafenpunkte, also wo noch ein Hafen gebaut werden kann
unsigned GameWorldBase::GetNextFreeHarborPoint(const MapCoord x, const MapCoord y, const unsigned origin_harbor_id, const unsigned char dir,
										   const unsigned char player) const
{
	return GetNextHarborPoint(x,y,origin_harbor_id,dir,player,&GameWorldBase::IsHarborPointFree);
}

/// Gibt die angrenzenden Sea-IDs eines Hafenpunktes zurück
void GameWorldBase::GetSeaIDs(const unsigned harbor_id, unsigned short * sea_ids) const
{
	for(unsigned i = 0;i<6;++i)
	{
		sea_ids[i] = harbor_pos[harbor_id].cps[i].sea_id;
	}
}

/// Berechnet die Entfernung zwischen 2 Hafenpunkten
unsigned GameWorldBase::CalcHarborDistance(const unsigned habor_id1, const unsigned harbor_id2) const
{
	const HarborPos& hp = harbor_pos[habor_id1];
	for(unsigned i = 0;i<6;++i)
	{
		for(unsigned z = 0;z<hp.neighbors[i].size();++z)
		{
			const HarborPos::Neighbor& n = hp.neighbors[i][z];
			if(n.id == harbor_id2)
				return n.distance;
		}
	}
	
	return 0xffffffff;
}

/// Bestimmt für einen beliebigen Punkt auf der Karte die Entfernung zum nächsten Hafenpunkt
unsigned GameWorldBase::CalcDistanceToNearestHarbor(const Point<MapCoord> pos) const
{
	unsigned min_distance = 0xffffffff;
	for(unsigned i = 0;i<harbor_pos.size();++i)
		min_distance = min(min_distance,this->CalcDistance(pos.x,pos.y,harbor_pos[i].x,harbor_pos[i].y));
		
	return min_distance;
}


/// Komperator zum Sortieren
bool GameWorldBase::PotentialSeaAttacker::operator<(const GameWorldBase::PotentialSeaAttacker& pa) const
{
	// Erst nach Rang, an zweiter Stelle nach Entfernung sortieren (
	if(soldier->GetRank() == pa.soldier->GetRank())
		return distance < pa.distance;
	else
		return soldier->GetRank() > pa.soldier->GetRank();
}

/// Liefert Hafenpunkte im Umkreis von einem bestimmten Militärgebäude
void GameWorldBase::GetHarborPointsAroundMilitaryBuilding(const MapCoord x, const MapCoord y, std::vector<unsigned> * harbor_points) const
{
	assert(harbor_points);
	

	// Nach Hafenpunkten in der Nähe des angegriffenen Gebäudes suchen
	// Alle unsere Häfen durchgehen
	for(unsigned i = 1;i<harbor_pos.size();++i)

	{
		MapCoord harbor_x = harbor_pos[i].x, harbor_y = harbor_pos[i].y;
		
		if(CalcDistance(harbor_x,harbor_y,x,y) <= SEAATTACK_DISTANCE)
		{
			// Wird ein Weg vom Militärgebäude zum Hafen gefunden bzw. Ziel = Hafen?
			if(x == harbor_x && y == harbor_y)
				harbor_points->push_back(i);
			else if(FindHumanPath(x,y,harbor_x,harbor_y,SEAATTACK_DISTANCE) != 0xff)
				harbor_points->push_back(i);
		}
	}
}

/// Sucht verfügbare Soldaten, um dieses Militärgebäude mit einem Seeangriff anzugreifen
void GameWorldBase::GetAvailableSoldiersForSeaAttack(const unsigned char player_attacker, const MapCoord x, const MapCoord y, 
	std::list<GameWorldBase::PotentialSeaAttacker> * attackers) const
{
	// Ist das Ziel auch ein richtiges Militärgebäude?
	if(GetNO(x,y)->GetGOT() != GOT_NOB_HARBORBUILDING && GetNO(x,y)->GetGOT() !=  GOT_NOB_HQ 
		&& GetNO(x,y)->GetGOT() !=  GOT_NOB_MILITARY)
		return;
		
	bool use_seas[512];
	memset(use_seas,0,512);
	
	// Mögliche Hafenpunkte in der Nähe des Gebäudes
	std::vector< unsigned > defender_harbors;
	
	GetHarborPointsAroundMilitaryBuilding(x,y,&defender_harbors);
	// Nach Hafenpunkten in der Nähe des angegriffenen Gebäudes suchen
	// Alle unsere Häfen durchgehen
	for(unsigned i = 0;i<defender_harbors.size();++i)
	{
		unsigned harbor_id = defender_harbors[i];

		// Steht an dieser Stelle ein Hafengeäude?
		Point<MapCoord> harbor_pos = GetHarborPoint(harbor_id);
		const noBase * hb = GetNO(harbor_pos.x,harbor_pos.y);
		if(hb->GetGOT() == GOT_NOB_HARBORBUILDING)
		{
			// Gehört dem Feind dieser Hafen und ist dieser Hafen nicht unser Ziel?
			if(players->getElement(player_attacker)->IsPlayerAttackable(static_cast<const nobHarborBuilding*>(hb)->GetPlayer())
				&& !(harbor_pos.x == x && harbor_pos.y == y))
			{
				// Dann können wir hier nicht landen
				continue;
			}
		}

		unsigned short sea_ids[6];
		GetSeaIDs(harbor_id,sea_ids);
		for(unsigned z = 0;z<6;++z)
		{
			if(sea_ids[z])
				use_seas[sea_ids[z]] = true;
		}
	}
	
	
	// Liste alle Militärgebäude des Angreifers, die Soldaten liefern
	std::vector<nobHarborBuilding::SeaAttackerBuilding> buildings;
	
	// Angrenzende Häfen des Angreifers an den entsprechenden Meeren herausfinden
	for(std::list<nobHarborBuilding*>::const_iterator it = players->getElement(player_attacker)->GetHarbors()
	.begin();it!=players->getElement(player_attacker)->GetHarbors().end();++it)
	{
		// Bestimmen, ob Hafen an einem der Meere liegt, über die sich auch die gegnerischen 
		// Hafenpunkte erreichen lassen
		bool is_at_sea = false;
		unsigned short sea_ids[6];
		GetSeaIDs((*it)->GetHarborPosID(),sea_ids);
		for(unsigned i = 0;i<6;++i)
		{
			if(sea_ids[i] && use_seas[sea_ids[i]])
			{
				is_at_sea = true;
				break;
			}
		}
		
		if(!is_at_sea)
			continue;
			
		(*it)->GetAttackerBuildingsForSeaAttack(&buildings,defender_harbors);
	}
	
	// Die Soldaten aus allen Militärgebäuden sammeln 
	for(unsigned i = 0;i<buildings.size();++i)
	{
		// Soldaten holen
		std::vector<nofPassiveSoldier*> tmp_soldiers;
		buildings[i].building->GetSoldiersForAttack(buildings[i].harbor->GetX(),buildings[i].harbor->GetY(),
		player_attacker,&tmp_soldiers);
		
		// Überhaupt welche gefunden?
		if(!tmp_soldiers.size())
			continue;
			
		// Soldaten hinzufügen
		for(unsigned j = 0;j<tmp_soldiers.size();++j)
		{
			PotentialSeaAttacker pa = { tmp_soldiers[j], buildings[i].harbor, buildings[i].distance };
			attackers->push_back(pa);
		}
	}
	
	// Entsprechend nach Rang sortieren
	attackers->sort();
	
	
	
}


