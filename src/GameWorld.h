// $Id: GameWorld.h 7326 2011-08-02 10:32:54Z FloSoft $
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

#ifndef OBJMANAGER_H_
#define OBJMANAGER_H_


#include "GameConsts.h"
#include "MapConsts.h"
#include "noNothing.h"
#include "RoadSegment.h"
#include "MilitaryConsts.h"
#include "EventManager.h"
#include "TerrainRenderer.h"
#include "main.h"
#include <vector>
#include "GamePlayerList.h"

class noEnvObject;
class noGranite;
class noTree;
class noFlag;
class noFigure;
class nobHQ;
class nobFarm;
class nobCatapult;
class ncFlag;
class noRoadNode;
class noCoordBase;
class nobBaseMilitary;
class SerializedGameData;
class CatapultStone;
class nofAttacker;
class MouseCoords;
class noBuilding;
class GameInterface;
class GameWorldBase;
class glArchivItem_Map;
class noShip;
class nofActiveSoldier;

struct RoadsBuilding;
class FOWObject;

typedef bool (*FP_Node_OK_Callback)(const GameWorldBase& gwb, const MapCoord x, const MapCoord y, const unsigned char dir, const void *param);

/// Eigenschaften von einem Punkt auf der Map
struct MapNode
{
	/// StraÃŸen
	unsigned char roads[3];
	bool roads_real[3];
	/// HÃ¶he
	unsigned char altitude;
	/// Schattierung
	unsigned char shadow;
	/// Terrain
	unsigned char t1,t2;
	/// Ressourcen
	unsigned char resources;
	/// Reservierungen
	bool reserved;
	/// Eigentümer (Spieler)
	unsigned char owner;
	/// Grenzsteine (der Punkt, und dann jeweils nach rechts, unten-links und unten-rechts die Zwischensteine)
	unsigned char boundary_stones[4];
	/// Bauqualität
	BuildingQuality bq;
	/// Visuelle Sachen für alle Spieler, die in Zusammenhang mit dem FoW stehen
	struct
	{
		/// Zeit (GF-Zeitpunkt), zu der, der Punkt zuletzt aktualisiert wurde
		unsigned last_update_time;
		/// Sichtbarkeit des Punktes
		Visibility visibility;
		/// FOW-Objekt
		FOWObject * object;
		/// StraÃŸen im FoW
		unsigned char roads[3];
		/// Grenzsteine (der Punkt, und dann jeweils nach rechts, unten-links und unten-rechts die Zwischensteine)
		unsigned char owner;
		/// Grenzsteine (der Punkt, und dann jeweils nach rechts, unten-links und unten-rechts die Zwischensteine)
		unsigned char boundary_stones[4];
	} fow[MAX_PLAYERS];

	/// Meeres-ID, d.h. zu welchem Meer gehÃ¶rt dieser Punkt (0 = kein Meer)
	unsigned short sea_id;
	/// Hafenpunkt-ID (0 = kein Hafenpunkt)
	unsigned harbor_id;

	/// Objekt, welches sich dort befindet
	noBase * obj;
	/// Figuren, Kämpfe, die sich dort befinden
	list<noBase*> figures;

	MapNode();
	
};

enum LandscapeType
{
	LT_GREENLAND = 0,
	LT_WASTELAND,
	LT_WINTERWORLD
};

/// Grundlegende Klasse, die die Gamewelt darstellt, enthält nur deren Daten
class GameWorldBase
{
protected:

	/// Interface zum GUI
	GameInterface * gi;

	/// Breite und HÃ¶he der Karte in Kontenpunkten
	unsigned short width,height;
	/// Landschafts-Typ
	LandscapeType lt;

	unsigned short * handled_nodes; /// Array von abgeklapperten Knoten und deren Punktzahlen
	list<CatapultStone*> catapult_stones; /// Liste von allen umherfliegenden Katapultsteinen.

	/// Eigenschaften von einem Punkt auf der Map
	MapNode *nodes;

	/// Rendert das Terrain
	TerrainRenderer tr;

	/// Informationen über die Weltmeere
	struct Sea
	{
		/// Anzahl der Knoten, welches sich in diesem Meer befinden
		unsigned nodes_count;

		Sea() : nodes_count(0) {}
		Sea(const unsigned nodes_count) : nodes_count(nodes_count) {}
	};
	std::vector<Sea> seas;

	/// Alle Hafenpositionen
	struct HarborPos
	{
		MapCoord x,y;

		struct CoastalPoint
		{
			unsigned short sea_id;
		} cps[6];

		struct Neighbor
		{
			unsigned id;
			unsigned distance;
			
			Neighbor() {}
			Neighbor(const unsigned id, const unsigned distance)
				: id(id), distance(distance) {}
			
			bool operator<(const Neighbor& two) const
			{ return distance < two.distance; }
		};

		std::vector<Neighbor> neighbors[6];
	};

	std::vector< HarborPos > harbor_pos;

	/// Baustellen von Häfen, die vom Schiff aus errichtet wurden
	std::list<noBuildingSite*> harbor_building_sites_from_sea;


public:
	unsigned int map_size;

	noNothing nothing; // nur Platzhalter bei der Rckgabe von GetNO
	/// Liste von Militärgebäuden (auch HQ und Haufengebäude, daher normale Gebäude) pro "Militärquadrat"
	list<nobBaseMilitary*> * military_squares;

public:

	GameWorldBase();
	virtual ~GameWorldBase();

	// Grundlegende Initialisierungen
	void Init();
	/// Aufräumen
	void Unload();

	/// Setzt GameInterface
	void SetGameInterface(GameInterface * const gi) { this->gi = gi; }

	/// GrÃ¶ÃŸe der Map abfragen
	unsigned short GetWidth() const { return width; }
	unsigned short GetHeight() const { return height; }

	/// Landschaftstyp abfragen
	LandscapeType GetLandscapeType() const { return lt; }

	/// Gibt Punkt um diesen Punkt (X-Koordinate) direkt zurück in einer Richtung von 0-5
	MapCoord GetXA(const MapCoord x, const MapCoord y, unsigned dir) const;
	/// Gibt Punkt um diesen Punkt (Y-Koordinate)  direkt zurück in einer Richtung von 0-5
	MapCoord GetYA(const MapCoord x, const MapCoord y, unsigned dir) const;
	/// Wie GetXA, bloÃŸ 2. AuÃŸenschale (dir zwischen 0 bis 11)
	MapCoord GetXA2(const MapCoord x, const MapCoord y, unsigned dir) const;
	/// Wie GetYA, bloÃŸ 2. AuÃŸenschale (dir zwischen 0 bis 11)
	MapCoord GetYA2(const MapCoord x, const MapCoord y, unsigned dir) const;
	/// Wandelt einen Punkt in einen Nachbarpunkt um
	void GetPointA(MapCoord& x, MapCoord& y, unsigned dir) const;
	/// Berechnet die Differenz zweier Koordinaten von x1 zu x2, wenn man berücksichtigt, dass man über den 
	/// Rand weitergehen kann
	MapCoord CalcDistanceAroundBorderX(const MapCoord x1, const MapCoord x2) const;
	MapCoord CalcDistanceAroundBorderY(const MapCoord y1, const MapCoord y2) const;

	/// Ermittelt Abstand zwischen 2 Punkten auf der Map unter Berücksichtigung der Kartengrenzüberquerung
	unsigned CalcDistance(const int x1, const int y1,
					  const int x2, const int y2) const;

	// Erzeugt eindeutige ID aus gegebenen X und Y-Werten
	unsigned MakeCoordID(const MapCoord x, const MapCoord y) const
	{ return y*width+x; }

	/// Gibt Map-Knotenpunkt zurück
	const MapNode& GetNode(const MapCoord x, const MapCoord y) const { assert(x<width && y<height);  return nodes[y*width+x]; }
	MapNode& GetNode(const MapCoord x, const MapCoord y) { assert(x<width && y<height); return nodes[y*width+x]; }
	/// Gibt MapKnotenpunkt darum zurück
	const MapNode& GetNodeAround(const MapCoord x, const MapCoord y, const unsigned i) const
	{ return GetNode(GetXA(x,y,i),GetYA(x,y,i));  }
	MapNode& GetNodeAround(const MapCoord x, const MapCoord y, const unsigned i)
	{ return GetNode(GetXA(x,y,i),GetYA(x,y,i));  }

	// Gibt ein NO zurück, falls keins existiert, wird ein "Nothing-Objekt" zurückgegeben
	noBase * GetNO(const MapCoord x, const MapCoord y);
	// Gibt ein NO zurück, falls keins existiert, wird ein "Nothing-Objekt" zurückgegeben
	const noBase * GetNO(const MapCoord x, const MapCoord y) const;
	/// Gibt ein FOW-Objekt zurück, falls keins existiert, wird ein "Nothing-Objekt" zurückgegeben
	const FOWObject * GetFOWObject(const MapCoord x, const MapCoord y, const unsigned spectator_player) const;
	/// Gibt den GOT des an diesem Punkt befindlichen Objekts zurück bzw. GOT_NOTHING, wenn keins existiert
	GO_Type GetGOT(const MapCoord x, const MapCoord y) const;

	/// Gibt Figuren, die sich auf einem bestimmten Punkt befinden, zurück
	/// nicht bei laufenden Figuren oder
	list<noBase*>& GetFigures(const MapCoord x, const MapCoord y) const { return nodes[y*width+x].figures; }
	/// Gibt Dynamische Objekte, die von einem bestimmten Punkt aus laufen oder dort stehen sowie andere Objekte,
	/// die sich dort befinden, zurück
	void GetDynamicObjectsFrom(const MapCoord x, const MapCoord y,list<noBase*>& objects) const;

	// Gibt ein spezifisches Objekt zurück
	template<typename T> T * GetSpecObj(MapCoord x, MapCoord y) { return dynamic_cast<T*>( GetNode(x,y).obj ); }
		// Gibt ein spezifisches Objekt zurück
	template<typename T> const T * GetSpecObj(MapCoord x, MapCoord y) const { return dynamic_cast<const T*>( GetNode(x,y).obj ); }

	/// Gibt ein Terrain-Dreieck um einen Punkt herum zurück.
	unsigned char GetTerrainAround(int x, int y, unsigned char dir) const;
	/// Gibt das Terrain zurück, über das ein Mensch/Tier laufen müsste, von X,Y in Richtung DIR (Vorwärts).
	unsigned char GetWalkingTerrain1(MapCoord x, MapCoord y, unsigned char dir) const;
	/// Gibt das Terrain zurück, über das ein Mensch/Tier laufen müsste, von X,Y in Richtung DIR (Rückwärts).
	unsigned char GetWalkingTerrain2(MapCoord x, MapCoord y, unsigned char dir) const;
	/// Gibt zurück, ob ein Punkt vollständig von Wasser umgeben ist
	bool IsSeaPoint(MapCoord x, MapCoord y) const;

	/// liefert den StraÃŸen-Wert an der Stelle X,Y
	unsigned char GetRoad(const MapCoord x, const MapCoord y, unsigned char dir, bool all = false) const;
	/// liefert den StraÃŸen-Wert um den Punkt X,Y.
	unsigned char GetPointRoad(const MapCoord x, const MapCoord y, unsigned char dir, bool all = false) const;
	/// liefert FOW-StraÃŸen-Wert um den punkt X,Y
	unsigned char GetPointFOWRoad(MapCoord x, MapCoord y, unsigned char dir, const unsigned char viewing_player) const;

	/// Kann dorthin eine StraÃŸe gebaut werden?
	bool RoadAvailable(const bool boat_road,const int x, const int y,unsigned char to_dir,const bool visual = true) const;
	/// Prüft ob exakt die gleiche StraÃŸe schon gebaut wurde
	bool RoadAlreadyBuilt(const bool boat_road, unsigned short start_x, unsigned short start_y, const std::vector<unsigned char>& route);
	/// Bauqualitäten berechnen, bei flagonly gibt er nur 1 zurück, wenn eine Flagge mÃ¶glich ist
	BuildingQuality CalcBQ(const MapCoord x, const MapCoord y,const unsigned char player,const bool flagonly = false,const bool visual = true, const bool ignore_player = false) const;
	/// Setzt die errechnete BQ gleich mit
	void SetBQ(const MapCoord x, const MapCoord y,const unsigned char player,const bool flagonly = false,const bool visual = true)
	{ GetNode(x,y).bq = CalcBQ(x,y,player,flagonly,visual); }

	/// Prüft, ob der Pkut zu dem Spieler gehÃ¶rt (wenn er der Besitzer ist und es false zurückliefert, ist es Grenzgebiet)
	bool IsPlayerTerritory(const MapCoord x, const MapCoord y) const;
	/// Berechnet BQ bei einer gebauten StraÃŸe
	void CalcRoad(const MapCoord x, const MapCoord y,const unsigned char player);
	/// Ist eine Flagge irgendwo um x,y ?
	bool FlagNear(const int x, const int y) const;
	/// Prüft, ob sich in unmittelbarer Nähe (im Radius von 4) Militärgebäude befinden
	bool IsMilitaryBuildingNearNode(const MapCoord nx, const MapCoord ny) const;

	/// setzt den virtuellen StraÃŸen-Wert an der Stelle X,Y (berichtigt).
	void SetVirtualRoad(const MapCoord x, const MapCoord y, unsigned char dir, unsigned char type);
	/// setzt den virtuellen StraÃŸen-Wert um den Punkt X,Y.
	void SetPointVirtualRoad(const MapCoord x, const MapCoord y, unsigned char dir, unsigned char type);

	/// Test, ob auf dem besagten Punkt ein Militärgebäude steht
	bool IsMilitaryBuilding(const MapCoord x, const MapCoord y) const;

	/// Erstellt eine Liste mit allen Militärgebäuden in der Umgebung, radius bestimmt wie viele Kästchen nach einer Richtung im Umkreis
	void LookForMilitaryBuildings(std::list<nobBaseMilitary*>& buildings,const MapCoord x, const MapCoord y, unsigned short radius) const;

	/// Prüft, ob von einem bestimmten Punkt aus der Untergrund für Figuren zugänglich ist (kein Wasser,Lava,Sumpf)
	bool IsNodeToNodeForFigure(const MapCoord x, const MapCoord y, const unsigned dir) const;

	/// Informationen, ob Grenzen überquert wurden
	struct CrossBorders
	{
		CrossBorders(void) : left(false), top(false), right(false), bottom(false) { }
		bool left, top, right, bottom;
	};


	/// Wegfindung in freiem Terrain - Basisroutine
	bool FindFreePath(const MapCoord x_start,const MapCoord y_start,
				  const MapCoord x_dest, const MapCoord y_dest, const bool random_route, 
				  const unsigned max_route, std::vector<unsigned char> * route, unsigned *length, unsigned char * first_dir, 
				  FP_Node_OK_Callback IsNodeOK, FP_Node_OK_Callback IsNodeToDestOk, const void * param, const bool record) const;
	/// Ermittelt, ob eine freie Route noch passierbar ist und gibt den Endpunkt der Route zurück
	bool CheckFreeRoute(const MapCoord x_start,const MapCoord y_start, const std::vector<unsigned char>& route,
		const unsigned pos, FP_Node_OK_Callback IsNodeOK, FP_Node_OK_Callback IsNodeToDestOk,
		 MapCoord* x_dest,  MapCoord* y_dest, const void * const param = NULL);
	/// Wegfindung auf StraÃŸen - Basisroutine
	bool FindPathOnRoads(const noRoadNode * const start, const noRoadNode * const goal,
									const bool ware_mode, unsigned * length,
									unsigned char * first_dir, Point<MapCoord> * next_harbor, const RoadSegment * const forbidden, const bool record = true) const;
	/// Findet einen Weg für Figuren
	unsigned char FindHumanPath(const MapCoord x_start,const MapCoord y_start,
		  const MapCoord x_dest, const MapCoord y_dest, const unsigned max_route = 0xFFFFFFFF, const bool random_route = false, unsigned *length = NULL) const;
	/// Wegfindung für Schiffe auf dem Wasser
	bool FindShipPath(const MapCoord x_start,const MapCoord y_start, const MapCoord x_dest, const MapCoord y_dest, std::vector<unsigned char> * route, unsigned * length, const unsigned max_length = 200,
								 CrossBorders * cb = NULL);


	/// Baut eine (bisher noch visuell gebaute) StraÃŸe wieder zurück
	void RemoveVisualRoad(unsigned short start_x, unsigned short start_y, const std::vector<unsigned char>& route);

	/// x,y ist ein Punkt auf irgendeinem Wegstck, gibt die Flagge zurück
	noFlag * GetRoadFlag(int x, int y,unsigned char& dir,unsigned last_i=255);

	/// Konvertiert die Koordinaten.
	void ConvertCoords(int x, int y, unsigned short * x_out, unsigned short * y_out) const;

	/// Erzeugt eine GUI-ID für die Fenster von Map-Objekten
	unsigned CreateGUIID(const MapCoord x, const MapCoord y) const
	{ return 1000 + width*y+x; }
	/// Gibt Terrainkoordinaten zurück
	float GetTerrainX(const MapCoord x, const MapCoord y)
	{ return tr.GetTerrainX(x,y); }
	float GetTerrainY(const MapCoord x, const MapCoord y)
	{ return tr.GetTerrainY(x,y); }

	/// Verändert die HÃ¶he eines Punktes und die damit verbundenen Schatten
	void ChangeAltitude(const MapCoord x, const MapCoord y, const unsigned char altitude);
	
	/// Ermittelt Sichtbarkeit eines Punktes auch unter Einbeziehung der Verbündeten des jeweiligen Spielers
	Visibility CalcWithAllyVisiblity(const MapCoord x, const MapCoord y, const unsigned char player) const; 

	/// Gibt die Anzahl an Hafenpunkten zurück
	unsigned GetHarborPointCount() const 
	{ return harbor_pos.size()-1; }
	/// Ist es an dieser Stelle für einen Spieler mÃ¶glich einen Hafen zu bauen
	bool IsHarborPointFree(const unsigned harbor_id, const unsigned char player, 
		const unsigned short sea_id) const;
	/// Gibt die Koordinaten eines bestimmten Hafenpunktes zurück
	Point<MapCoord> GetHarborPoint(const unsigned harbor_id) const;
	/// Gibt die ID eines Hafenpunktes zurück
	unsigned GetHarborPointID(const MapCoord x, const MapCoord y) const
	{ return GetNode(x,y).harbor_id; }
	/// Ermittelt, ob ein Punkt Küstenpunkt ist, d.h. Zugang zu einem schiffbaren Meer hat 
	/// und gibt ggf. die Meeres-ID zurück, ansonsten 0
	unsigned short IsCoastalPoint(const MapCoord x, const MapCoord y) const;
		/// Grenzt der Hafen an ein bestimmtes Meer an?
	bool IsAtThisSea(const unsigned harbor_id, const unsigned short sea_id) const;
	/// Gibt den Punkt eines bestimmtes Meeres um den Hafen herum an, sodass Schiffe diesen anfahren kÃ¶nnen
	void GetCoastalPoint(const unsigned harbor_id, MapCoord * px, MapCoord * py, const unsigned short sea_id) const;
	/// Sucht freie Hafenpunkte, also wo noch ein Hafen gebaut werden kann
	unsigned GetNextFreeHarborPoint(const MapCoord x, const MapCoord y, const unsigned origin_harbor_id, const unsigned char dir,
										   const unsigned char player) const;
	/// Gibt die angrenzenden Sea-IDs eines Hafenpunktes zurück
	void GetSeaIDs(const unsigned harbor_id, unsigned short * sea_ids) const;
	/// Berechnet die Entfernung zwischen 2 Hafenpunkten
	unsigned CalcHarborDistance(const unsigned habor_id1, const unsigned harbor_id2) const;
	/// Bestimmt für einen beliebigen Punkt auf der Karte die Entfernung zum nächsten Hafenpunkt
	unsigned CalcDistanceToNearestHarbor(const Point<MapCoord> pos) const;
	
	

	void SetPlayers(GameClientPlayerList *pls) { players = pls; }
	/// Liefert einen Player zurück
	inline GameClientPlayer * GetPlayer(const unsigned int id) const { return players->getElement(id); }
	
	struct PotentialSeaAttacker
	{
		/// Soldat, der als Angreifer in Frage kommt
		nofPassiveSoldier * soldier;
		/// Hafen, den der Soldat zuerst ansteuern soll
		nobHarborBuilding * harbor;
		/// Entfernung Hafen-Hafen (entscheidende)
		unsigned distance;
		
		/// Komperator zum Sortieren
		bool operator<(const PotentialSeaAttacker& pa) const;
	};
	
	
	/// Liefert Hafenpunkte im Umkreis von einem bestimmten Militärgebäude
	void GetHarborPointsAroundMilitaryBuilding(const MapCoord x, const MapCoord y, std::vector<unsigned> * harbor_points) const;

	/// Sucht verfügbare Soldaten, um dieses Militärgebäude mit einem Seeangriff anzugreifen
	void GetAvailableSoldiersForSeaAttack(const unsigned char player_attacker, 
	const MapCoord x, const MapCoord y, std::list<PotentialSeaAttacker> * attackers) const;

protected:

	/// Berechnet die Schattierung eines Punktes neu
	void RecalcShadow(const MapCoord x, const MapCoord y);

	/// Für abgeleitete Klasse, die dann das Terrain entsprechend neu generieren kann
	virtual void AltitudeChanged(const MapCoord x, const MapCoord y) = 0;
	/// Für abgeleitete Klasse, die dann das Terrain entsprechend neu generieren kann
	virtual void VisibilityChanged(const MapCoord x, const MapCoord y) = 0;

	/// Gibt nächsten Hafenpunkt in einer bestimmten Richtung zurück, bzw. 0, wenn es keinen gibt 
	unsigned GetNextHarborPoint(const MapCoord x, const MapCoord y, const unsigned origin_harbor_id, const unsigned char dir,
										   const unsigned char player, bool (GameWorldBase::*IsPointOK)(const unsigned, const unsigned char, const unsigned short) const) const;

private:
	GameClientPlayerList *players;

};

/// "Interface-Klasse" für GameWorldBase, die die Daten grafisch anzeigt
class GameWorldViewer : public virtual GameWorldBase
{
	bool show_bq;    ///< Bauqualitäten-Anzeigen ein oder aus
	bool show_names; ///< Gebäudenamen-Anzeigen ein oder aus
	bool show_productivity; ///< Produktivität-Anzeigen ein oder aus

	/// Scrolling-Zeug
	int xoffset,yoffset;
	/// Erster gezeichneter Map-Punkt
	int fx,fy;
	/// Letzter gezeichneter Map-Punkt
	int lx,ly;
	/// Selektierter Punkt
	unsigned short selx,sely;
	int selxo,selyo;
	int sx,sy;
	/// Wird gerade gescrollt?
	bool scroll;
	/// Letzte Scrollposition, an der man war, bevor man weggesprungen ist
	int last_xoffset, last_yoffset;
	/// Koordinaten auf der Map anzeigen (zum Debuggen)?
	bool show_coordinates;

public:

	GameWorldViewer();

	// Wege und Grenzsteine zeichnen ( nur reingenommen, da die Position sowieso hier berechnet wird, da bietet es sich an )
	void DrawWays(const int x, const int y, const float xpos, const float ypos);
	void DrawBoundaryStone(const int x, const int y, const MapCoord tx, const MapCoord ty, const int xpos, const int ypos, Visibility vis);

	/// Bauqualitäten anzeigen oder nicht
	void ShowBQ() { show_bq = !show_bq; }
	/// Gebäudenamen zeigen oder nicht
	void ShowNames() { show_names = !show_names; }
	/// Produktivität zeigen oder nicht
	void ShowProductivity() { show_productivity = !show_productivity; };
	/// Schaltet Produktivitäten/Namen komplett aus oder an
	void ShowNamesAndProductivity();

	/// Wegfinden ( A* ) --> Wegfindung auf allgemeinen Terrain ( ohne Straäcn ) ( fr Wegebau oder frei herumlaufende )
	bool FindRoadPath(const MapCoord x_start,const MapCoord y_start, const MapCoord x_dest, const MapCoord y_dest,std::vector<unsigned char>& route, const bool boat_road);
	/// Sucht die Anzahl der verfügbaren Soldaten, um das Militärgebäude an diesem Punkt anzugreifen
	unsigned GetAvailableSoldiersForAttack(const unsigned char player_attacker,const MapCoord x, const MapCoord y);
	/// Zeichnet die Objekte
	void Draw(const unsigned char player, unsigned * water, const bool draw_selected, const MapCoord selected_x, const MapCoord selected_y,const RoadsBuilding& rb);

	/// Scrolling-Zeug
	void MouseMove(const MouseCoords& mc);
	void MouseDown(const MouseCoords& mc);
	/// Bewegt sich zu einer bestimmten Position in Pixeln auf der Karte
	void MoveTo(int x, int y, bool absolute = false);
	/// Zentriert den Bildschirm auf ein bestimmtes Map-Object
	void MoveToMapObject(const MapCoord x, const MapCoord y);
	/// Springt zur letzten Position, bevor man "weggesprungen" ist
	void MoveToLastPosition();

	void MoveToX(int x, bool absolute = false) { MoveTo( (absolute ? 0 : xoffset) + x, yoffset, true); }
	void MoveToY(int y, bool absolute = false) { MoveTo( xoffset, (absolute ? 0 : yoffset) + y, true); }
	void MouseUp();
	void DontScroll() { scroll = false; }
	void CalcFxLx();

	/// Koordinatenanzeige ein/aus
	void ShowCoordinates() { show_coordinates = !show_coordinates; }

	/// Gibt selektierten Punkt zurück
	unsigned short GetSelX() const { return selx; }
	unsigned short GetSelY() const { return sely; }

	int GetSelXo() const { return selxo; }
	int GetSelYo() const { return selyo; }

	/// Gibt Scrolling-Offset zurück
	int GetXOffset() const { return xoffset; }
	int GetYOffset() const { return yoffset; }
	/// Gibt ersten Punkt an, der beim Zeichnen angezeigt wird
	int GetFirstX() const { return fx; }
	int GetFirstY() const { return fy; }
	/// Gibt letzten Punkt an, der beim Zeichnen angezeigt wird
	int GetLastX() const { return lx; }
	int GetLastY() const { return ly; }

	/// Ermittelt Sichtbarkeit eines Punktes für den lokalen Spieler, berücksichtigt ggf. Teamkameraden
	Visibility GetVisibility(const MapCoord x, const MapCoord y) const; 

	/// HÃ¶he wurde verändert: TerrainRenderer Bescheid sagen, damit es entsprechend verändert werden kann
	void AltitudeChanged(const MapCoord x, const MapCoord y);
	/// Sichtbarkeit wurde verändert: TerrainRenderer Bescheid sagen, damit es entsprechend verändert werden kann
	void VisibilityChanged(const MapCoord x, const MapCoord y);

	/// Get the "youngest" FOWObject of all players who share the view with the local player
	const FOWObject * GetYoungestFOWObject(const Point<MapCoord> pos) const;
	
	/// Gets the youngest fow node of all visible objects of all players who are connected
	/// with the local player via team view
	unsigned char GetYoungestFOWNodePlayer(const Point<MapCoord> pos) const;


	/// Schattierungen (vor allem FoW) neu berechnen
	void RecalcAllColors();

	/// liefert sichtbare StraÃŸe, im Nebel entsprechend die FoW-StraÃŸe
	unsigned char GetVisibleRoad(const MapCoord x, const MapCoord y, unsigned char dir, const Visibility visibility) const;

	/// Gibt das erste Schiff, was gefunden wird von diesem Spieler, zurück, ansonsten NULL, falls es nicht
	/// existiert
	noShip * GetShip(const MapCoord x, const MapCoord y, const unsigned char player) const;
	
	/// Gibt die verfügbar Anzahl der Angreifer für einen Seeangriff zurück
	unsigned GetAvailableSoldiersForSeaAttackCount(const unsigned char player_attacker, const MapCoord x, const MapCoord y) const;

	void Resize(unsigned short displayWidth, unsigned short displayHeight);

protected:
	unsigned short displayWidth, displayHeight;


	// debug ai
	private:
		unsigned d_what;
		unsigned d_player;
		bool d_active;
	public:
		void SetAIDebug(unsigned what, unsigned player, bool active) 
		{
			d_what = what; d_player = player; d_active = active; 
		}


};

/// "Interface-Klasse" für das Spiel
class GameWorldGame : public virtual GameWorldBase
{
private:

	/// vergleicht 2 Punkte, ob sie von unterschiedlichen Spielern sind und setzt
	/// Punkt ggf. zu gar keinem Spieler, 2. Funktion wird für Punkte im 2er Abstand verwendet, da es dort ein bisschen anders läuft!
	void AdjustNodes(const MapCoord x1, const MapCoord y1, const MapCoord x2, const MapCoord y2);
	void AdjustNodes2(const MapCoord x1, const MapCoord y1, const MapCoord x2, const MapCoord y2);
	/// ZerstÃ¶rt Spielerteile auf einem Punkt, wenn der Punkt dem Spieler nun nich mehr gehÃ¶rt
	void DestroyPlayerRests(const MapCoord x, const MapCoord y, const unsigned char new_player,const noBaseBuilding * exception);

	/// Prüft, ob auf diesem Punkt Deko-Objekte liegen, die für den Wegbau entfernt werden kÃ¶nnen
	bool IsObjectionableForRoad(const MapCoord x, const MapCoord y);

	
	/// Punkt vollständig sichtbar?
	bool IsPointCompletelyVisible(const MapCoord x, const MapCoord y, const unsigned char player, const noBaseBuilding * const exception) const;
	/// Prüft, ob auf angegebenen Punkt sich ein Späher von player befindet
	bool IsScoutingFigureOnNode(const MapCoord x, const MapCoord y, const unsigned player, const unsigned distance) const;
	/// Berechnet die Sichtbarkeit eines Punktes neu für den angegebenen Spieler
	/// exception ist ein Gebäude (Spähturm, Militärgebäude), was nicht mit in die Berechnugn einbezogen
	/// werden soll, z.b. weil es abgerissen wird
	void RecalcVisibility(const MapCoord x, const MapCoord y, const unsigned char player, const noBaseBuilding * const exception);
	/// Setzt Punkt auf jeden Fall auf sichtbar
	void SetVisibility(const MapCoord x, const MapCoord y,  const unsigned char player);
	
	/// Prüfen, ob zu einem bestimmten Küsenpunkt ein Hafenpunkt gehÃ¶rt und wenn ja, wird dieser zurückgegeben
	unsigned short GetHarborPosID(const MapCoord x, const MapCoord y);
	/// Bestimmt die Schifffahrtrichtung, in der ein Punkt relativ zu einem anderen liegt 
	unsigned char GetShipDir(Point<int> pos1, Point<int> pos2);
	

protected:

	/// Erzeugt FOW-Objekte, -StraÃŸen und -Grensteine von aktuellen Punkt für einen bestimmten Spieler
	void SaveFOWNode(const MapCoord x, const MapCoord y, const unsigned player);
	/// Berechnet für alle Hafenpunkt jeweils die Richtung und Entfernung zu allen anderen Hafenpunkten
	/// über die Kartenränder hinweg
	void CalcHarborPosNeighbors();


public:

	/// Stellt anderen Spielern/Spielobjekten das Game-GUI-Interface zur Verfüung
	GameInterface * GetGameInterface() const { return gi; }

	void SetNO(noBase * obj, const MapCoord x, const MapCoord y) { GetNode(x,y).obj = obj; }
	void AddFigure(noBase * fig,const MapCoord x, const MapCoord y);
	void RemoveFigure(const noBase * fig,const MapCoord x, const MapCoord y);

	/// Berechnet Bauqualitäten an Punkt x;y und den ersten Kreis darum neu
	void RecalcBQAroundPoint(const MapCoord x, const MapCoord y);
	/// Berechnet Bauqualitäten wie bei letzterer Funktion, bloÃŸ noch den 2. Kreis um x;y herum
	void RecalcBQAroundPointBig(const MapCoord x, const MapCoord y);

	/// Prüft, ob dieser Punkt von Menschen betreten werden kann
	bool IsNodeForFigures(const MapCoord x, const MapCoord y);
	/// Kann dieser Punkt von auf StraÃŸen laufenden Menschen betreten werden? (Kämpfe!)
	bool IsRoadNodeForFigures(const MapCoord x, const MapCoord y,const unsigned char dir);
	/// Lässt alle Figuren, die auf diesen Punkt  auf Wegen zulaufen, anhalten auf dem Weg (wegen einem Kampf)
	void StopOnRoads(const MapCoord x, const MapCoord y, const unsigned char dir = 0xff);

	/// Sagt Bescheid, dass der Punkt wieder freigeworden ist und lässt ggf. Figuren drumherum wieder weiterlaufen
	void RoadNodeAvailable(const MapCoord x, const MapCoord y);

	/// Flagge an x,y setzen, dis_dir ist der aus welche Richtung der Weg kommt, wenn man einen Weg mit Flagge baut
	/// kann ansonsten auf 255 gesetzt werden
	void SetFlag(const MapCoord x, const MapCoord y, const unsigned char player,const unsigned char dis_dir = 255);
	/// Flagge soll zerstrÃ¶rt werden
	void DestroyFlag(const MapCoord x, const MapCoord y);
	/// Baustelle setzen
	void SetBuildingSite(const BuildingType type,const MapCoord x, const MapCoord y, const unsigned char player);
	/// Gebäude bzw Baustelle abreiÃŸen
	void DestroyBuilding(const MapCoord x, const MapCoord y, const unsigned char playe);

	/// Wegfindung für Menschen im StraÃŸennetz
	unsigned char FindHumanPathOnRoads(const noRoadNode * const start, const noRoadNode * const goal,unsigned * length = NULL, Point<MapCoord> * next_harbor = NULL, const RoadSegment * const forbidden = NULL);
	/// Wegfindung für Waren im StraÃŸennetz
	unsigned char FindPathForWareOnRoads(const noRoadNode * const start, const noRoadNode * const goal,unsigned * length = NULL, Point<MapCoord> * next_harbor = NULL);
	/// Prüft, ob eine Schiffsroute noch Gültigkeit hat
	bool CheckShipRoute(const MapCoord x_start,const MapCoord y_start, const std::vector<unsigned char>& route, const unsigned pos, 
		 MapCoord* x_dest,  MapCoord* y_dest);

	/// setzt den StraÃŸen-Wert an der Stelle X,Y (berichtigt).
	void SetRoad(const MapCoord x, const MapCoord y, unsigned char dir, unsigned char type);

	/// setzt den StraÃŸen-Wert um den Punkt X,Y.
	void SetPointRoad(const MapCoord x, const MapCoord y, unsigned char dir, unsigned char type);

	/// Funktionen aus ehemaligen Game
	/// Baut eine StraÃŸe ( nicht nur visuell, sondern auch wirklich )
	void BuildRoad(const unsigned char playerid,const bool boat_road,
		unsigned short start_x,unsigned short start_y, const std::vector<unsigned char>& route);
	/// ReiÃŸt eine StraÃŸe ab
	void DestroyRoad(const MapCoord x, const MapCoord y, const unsigned char dir);
	/// baut eine StraÃŸe aus
	void UpgradeRoad(const MapCoord x, const MapCoord y, const unsigned char dir);

	/// Berechnet das Land in einem bestimmten Bereich (um ein neues, abgerissenes oder eingenommenes
	/// Militärgebäude rum) neu, destroyed gibt an, ob building abgerissen wurde und somit nicht einberechnet werden soll
	void RecalcTerritory(const noBaseBuilding * const building,const unsigned short radius, const bool destroyed, const bool newBuilt);
	/// Greift ein Militärgebäude auf x,y an (entsendet dafür die Soldaten etc.)
	void Attack(const unsigned char player_attacker, const MapCoord x, const MapCoord y, const unsigned short soldiers_count, const bool strong_soldiers);
	/// Greift ein Militäregebäude mit Schiffen an 
	void AttackViaSea(const unsigned char player_attacker, const MapCoord x, const MapCoord y, const unsigned short soldiers_count, const bool strong_soldiers);
	// Liefert das entsprechende Militärquadrat für einen bestimmten Punkt auf der Karte zurück (normale Koordinaten)
	list<nobBaseMilitary*>& GetMilitarySquare(const MapCoord x, const MapCoord y)
	{ return military_squares[(y/MILITARY_SQUARE_SIZE)*(width/MILITARY_SQUARE_SIZE+1)+x/MILITARY_SQUARE_SIZE]; }

	/// Fügt einen Katapultstein der Welt hinzu, der gezeichnt werden will
	void AddCatapultStone(CatapultStone * cs) {catapult_stones.push_back(cs); }
	void RemoveCatapultStone(CatapultStone * cs) {catapult_stones.erase(cs); }

	/// Lässt alles spielerische abbrennen, indem es alle Flaggen der Spieler zerstÃ¶rt
	void Armageddon();

  /// Lässt alles spielerische eines Spielers abbrennen, indem es alle Flaggen eines Spieler zerstÃ¶rt
	void Armageddon(const unsigned char player);

	/// Sagt der GW Bescheid, dass ein Objekt von Bedeutung an x,y vernichtet wurde, damit dieser
	/// dass ggf. an den WindowManager weiterleiten kann, damit auch ein Fenster wieder geschlossen wird
	virtual void ImportantObjectDestroyed(const MapCoord x, const MapCoord y) = 0;
	/// Sagt, dass ein Militärgebäude eingenommen wurde und ggf. ein entsprechender "Fanfarensound" abgespielt werden sollte
	virtual void MilitaryBuildingCaptured(const MapCoord x, const MapCoord y, const unsigned char player) = 0;

	/// Ist der Punkt ein geeigneter Platz zum Warten vor dem Militärgebäude
	bool ValidWaitingAroundBuildingPoint(const MapCoord x, const MapCoord y, nofAttacker * attacker);
	/// Geeigneter Punkt für Kämpfe?
	bool ValidPointForFighting(const MapCoord x, const MapCoord y, nofActiveSoldier *exception = NULL);

	/// Berechnet die Sichtbarkeiten neu um einen Punkt mit radius
	void RecalcVisibilitiesAroundPoint(const MapCoord x, const MapCoord y, const MapCoord radius, const unsigned char player, const noBaseBuilding * const exception);
	/// Setzt die Sichtbarkeiten um einen Punkt auf sichtbar (aus Performancegründen Alternative zu oberem)
	void SetVisibilitiesAroundPoint(const MapCoord x, const MapCoord y, const MapCoord radius, const unsigned char player);
	/// Berechet die ganzen Sichtbarkeiten der Karte neu
	void RecalcAllVisibilities();
	/// Bestimmt bei der Bewegung eines spähenden Objekts die Sichtbarkeiten an
	/// den Rändern neu
	void RecalcMovingVisibilities(const MapCoord x, const MapCoord y, const unsigned char player, const MapCoord radius, 
		const unsigned char moving_dir, Point<MapCoord> * enemy_territory);

	/// Stellt fest, ob auf diesem Punkt ein Grenzstein steht (ob das Grenzgebiet ist)
	bool IsBorderNode(const MapCoord x, const MapCoord y, const unsigned char player) const;
	
	// Konvertiert Ressourcen zwischen Typen hin und her oder lÃ¶scht sie.
	// Für Spiele ohne Gold.
	void ConvertMineResourceTypes(unsigned char from, unsigned char to);

	/// Gründet vom Schiff aus eine neue Kolonie, gibt true zurück bei Erfolg
	bool FoundColony(const unsigned harbor_point, const unsigned char player, const unsigned short sea_id);
	/// Registriert eine Baustelle eines Hafens, die vom Schiff aus gesetzt worden ist
	void AddHarborBuildingSiteFromSea(noBuildingSite * building_site)
	{ this->harbor_building_sites_from_sea.push_back(building_site); }
	/// Entfernt diese wieder
	void RemoveHarborBuildingSiteFromSea(noBuildingSite * building_site)
	{ this->harbor_building_sites_from_sea.remove(building_site); }
	/// Gibt zurück, ob eine bestimmte Baustellen eine Baustelle ist, die vom Schiff aus errichtet wurde
	bool IsHarborBuildingSiteFromSea(const noBuildingSite * building_site) const;
	/// Liefert eine Liste der Hafenpunkte, die von einem bestimmten Hafenpunkt erreichbar sind
	void GetHarborPointsWithinReach(const unsigned hp,std::vector<unsigned>& hps) const;
};


class GameWorld : public GameWorldViewer, public GameWorldGame
{
public:

	/// Lädt eine Karte
	bool LoadMap(const std::string& filename);

	/// Serialisiert den gesamten GameWorld
	void Serialize(SerializedGameData *sgd) const;
	void Deserialize(SerializedGameData *sgd);

	/// Sagt der GW Bescheid, dass ein Objekt von Bedeutung an x,y vernichtet wurde, damit dieser
	/// dass ggf. an den WindowManager weiterleiten kann, damit auch ein Fenster wieder geschlossen wird
	void ImportantObjectDestroyed(const MapCoord x, const MapCoord y);
	/// Sagt, dass ein Militärgebäude eingenommen wurde und ggf. ein entsprechender "Fanfarensound" abgespielt werden sollte
	void MilitaryBuildingCaptured(const MapCoord x, const MapCoord y, const unsigned char player);

private:
	/// Vermisst ein neues Weltmeer von einem Punkt aus, indem es alle mit diesem Punkt verbundenen
	/// Wasserpunkte mit der gleichen sea_id belegt und die Anzahl zurückgibt
	unsigned MeasureSea(const MapCoord x, const MapCoord y, const unsigned short sea_id);

	/// Erstellt Objekte anhand der ausgelesenen S2map
	void Scan(glArchivItem_Map *map);


};



#endif

