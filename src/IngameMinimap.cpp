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
#include "IngameMinimap.h"
#include "world/GameWorldViewer.h"
#include "GameClient.h"
#include "FOWObjects.h"
#include "gameData/MinimapConsts.h"
#include "gameData/TerrainData.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

IngameMinimap::IngameMinimap(const GameWorldViewer& gwv):
    Minimap(gwv.GetWidth(), gwv.GetHeight()), gwv(gwv), nodes_updated(gwv.GetWidth()*gwv.GetHeight(), false),
    dos(gwv.GetWidth()*gwv.GetHeight(), DO_INVALID), territory(true), houses(true), roads(true)
{
    CreateMapTexture();
}

///////////////////////////////////////////////////////////////////////////////
/**
*
*  @author OLiver
*/
unsigned IngameMinimap::CalcPixelColor(const MapPoint pt, const unsigned t)
{
    unsigned color = 0;

    // Beobeachtender Spieler
    unsigned char viewing_player = GAMECLIENT.GetPlayerID();

    Visibility visibility = gwv.GetVisibility(pt);

    if(visibility == VIS_INVISIBLE)
    {
        dos[GetMMIdx(pt)] = DO_INVISIBLE;
        // Man sieht nichts --> schwarz
        return 0xFF000000;
    } else
    {
        DrawnObject drawn_object = DO_INVALID;

        bool fow = (visibility == VIS_FOW);

        unsigned char owner;
        if(!fow)
            owner = gwv.GetNode(pt).owner;
        else
            owner = gwv.GetNode(pt).fow[GAMECLIENT.GetPlayerID()].owner;

        // Baum an dieser Stelle?
        if((!fow && gwv.GetNO(pt)->GetGOT() == GOT_TREE) || (fow && gwv.GetFOWObject(pt, viewing_player)->GetType() == FOW_TREE)) //-V807
        {
            color = VaryBrightness(TREE_COLOR, VARY_TREE_COLOR);
            drawn_object = DO_TERRAIN;
            // Ggf. mit Spielerfarbe
            if(owner)
            {
                drawn_object = DO_PLAYER;
                if(territory)
                    color = CombineWithPlayerColor(color, owner);
            }
        }
        // Granit an dieser Stelle?
        else if((!fow && gwv.GetNO(pt)->GetGOT() == GOT_GRANITE) || (fow && gwv.GetFOWObject(pt, viewing_player)->GetType() == FOW_GRANITE))
        {
            color = VaryBrightness(GRANITE_COLOR, VARY_GRANITE_COLOR);
            drawn_object = DO_TERRAIN;
            // Ggf. mit Spielerfarbe
            if(owner)
            {
                drawn_object = DO_PLAYER;
                if(territory)
                    color = CombineWithPlayerColor(color, owner);
            }
        }
        // Ansonsten die jeweilige Terrainfarbe nehmen
        else
        {
            // Ggf. Spielerfarbe mit einberechnen, falls das von einem Spieler ein Territorium ist
            if(owner)
            {
                // Gebäude?
                GO_Type got = gwv.GetNO(pt)->GetGOT();
                FOW_Type fot = gwv.GetFOWObject(pt, viewing_player)->GetType();

                if(((!fow && (got == GOT_NOB_USUAL || got == GOT_NOB_MILITARY ||
                    got == GOT_NOB_STOREHOUSE || got == GOT_NOB_SHIPYARD || got == GOT_NOB_HARBORBUILDING ||
                    got == GOT_NOB_HQ || got == GOT_BUILDINGSITE)) || (fow && (fot == FOW_BUILDING || fot == FOW_BUILDINGSITE))))
                    drawn_object = DO_BUILDING;
                /// Straßen?
                else if(IsRoad(pt, visibility))
                    drawn_object = DO_ROAD;
                // ansonsten normales Territorium?
                else
                    drawn_object = DO_PLAYER;

                if(drawn_object == DO_BUILDING && houses)
                    color = BUILDING_COLOR;
                /// Straßen?
                else if(drawn_object == DO_ROAD && roads)
                    color = ROAD_COLOR;
                // ansonsten normales Territorium?
                else if(territory)
                    // Normales Terrain und Spielerfarbe berechnen
                    color = CombineWithPlayerColor(CalcTerrainColor(pt, t), owner);
                else
                    // Normales Terrain berechnen
                    color = CalcTerrainColor(pt, t);
            } else
            {
                // Normales Terrain berechnen
                color = CalcTerrainColor(pt, t);
                drawn_object = DO_TERRAIN;
            }
        }

        // Bei FOW die Farben abdunkeln
        if(fow)
            color = MakeColor(0xFF, GetRed(color) / 2, GetGreen(color) / 2, GetBlue(color) / 2);

        dos[GetMMIdx(pt)] = drawn_object;
    }

    return color;
}

///////////////////////////////////////////////////////////////////////////////
/**
*  Berechnet für einen bestimmten Punkt und ein Dreieck die normale Terrainfarbe
*
*  @author OLiver
*/
unsigned IngameMinimap::CalcTerrainColor(const MapPoint pt, const unsigned t)
{
    unsigned color = TerrainData::GetColor(gwv.GetLandscapeType(), (t == 0) ? gwv.GetNode(pt).t1 : gwv.GetNode(pt).t2); //-V807

    // Schattierung
    int shadow = gwv.GetNode(pt).shadow;
    int r = GetRed(color) + shadow - 0x40;
    int g = GetGreen(color) + shadow - 0x40;
    int b = GetBlue(color) + shadow - 0x40;

    if(r < 0) r = 0;
    else if(r > 255) r = 255;
    if(g < 0) g = 0;
    else if(g > 255) g = 255;
    if(b < 0) b = 0;
    else if(b > 255) b = 255;

    return MakeColor(0xFF, unsigned(r), unsigned(g), unsigned(b));
}

///////////////////////////////////////////////////////////////////////////////
/**
*  Prüft ob an einer Stelle eine Straße gezeichnet werden muss
*
*  @author OLiver
*/
bool IngameMinimap::IsRoad(const MapPoint pt, const Visibility visibility)
{
    for(unsigned i = 0; i < 3; ++i)
    {
        if(gwv.GetVisibleRoad(pt, i, visibility))
            return true;
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////
/**
*  Berechnet Spielerfarbe mit in eine gegebene Farbe mit ein
*  (player muss mit +1 gegeben sein!)
*
*  @author OLiver
*/
unsigned IngameMinimap::CombineWithPlayerColor(const unsigned color, const unsigned char player) const
{
    // Spielerfarbe mit einberechnen
    unsigned player_color = GAMECLIENT.GetPlayer(player - 1).color;

    return MakeColor(0xFF, (GetRed(color) + GetRed(player_color)) / 2,
        (GetGreen(color) + GetGreen(player_color)) / 2,
        (GetBlue(color) + GetBlue(player_color)) / 2);
}

///////////////////////////////////////////////////////////////////////////////
/**
*
*  @author OLiver
*/
void IngameMinimap::UpdateNode(const MapPoint pt)
{
    if(!nodes_updated[GetMMIdx(pt)])
    {
        nodes_updated[GetMMIdx(pt)] = true;
        nodesToUpdate.push_back(pt);
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
*  Zusätzliche Dinge, die die einzelnen Maps vor dem Zeichenvorgang zu tun haben
*  in dem Falle: Karte aktualisieren
*
*  @author OLiver
*/
void IngameMinimap::BeforeDrawing()
{
    // Ab welcher Knotenanzahl (Teil der Gesamtknotenanzahl) die Textur komplett neu erstellt werden soll
    static const unsigned MAX_NODES_UPDATE_DENOMINATOR = 2; // (2 = 1/2, 3 = 1/3 usw.)

                                                            // überhaupt änderungen nötig?
    if(!nodesToUpdate.empty())
    {
        // Komplette Textur neu erzeugen, weil es zu viele Knoten sind?
        if(nodesToUpdate.size() >= map_width * map_height / MAX_NODES_UPDATE_DENOMINATOR)
        {
            // Ja, alles neu erzeugen
            UpdateAll();
            // Alles Aktualisierungen wieder zurücksetzen
            for(MapPoint p(0, 0); p.y < map_height; p.y++)
                for(p.x = 0; p.x < map_width; p.x++)
                    nodes_updated[GetMMIdx(p)] = false;
        } else
        {
            // Entsprechende Pixel updaten
            for(std::vector<MapPoint>::iterator it = nodesToUpdate.begin(); it != nodesToUpdate.end(); ++it)
            {
                for(unsigned t = 0; t < 2; ++t)
                {
                    unsigned color = CalcPixelColor(*it, t);
                    map.tex_setPixel((it->x * 2 + t + (it->y & 1)) % (map_width * 2), it->y, GetRed(color), GetGreen(color),
                        GetBlue(color), GetAlpha(color));
                }
                // Jetzt muss er nicht mehr geändert werden
                nodes_updated[GetMMIdx(*it)] = false;
            }
        }

        this->nodesToUpdate.clear();
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
*  Updatet die gesamte Minimap
*
*  @author OLiver
*/
void IngameMinimap::UpdateAll()
{
    map.DeleteTexture();
    CreateMapTexture();
}

///////////////////////////////////////////////////////////////////////////////
/**
*  Alle Punkte Updaten, bei denen das DrawnObject
*  gleich dem übergebenen drawn_object ist
*
*  @author OLiver
*/
void IngameMinimap::UpdateAll(const DrawnObject drawn_object)
{
    // Gesamte Karte neu berechnen
    for(MapCoord y = 0; y < map_height; ++y)
    {
        for(MapCoord x = 0; x < map_width; ++x)
        {
            MapPoint pt(x, y);
            for(unsigned t = 0; t < 2; ++t)
            {
                if(dos[GetMMIdx(pt)] == drawn_object || // das gewünschte Objekt
                    (drawn_object == DO_PLAYER && // bei DO_PLAYER auf evtl. nicht gezeichnete Häuser und Straßen
                        ((dos[GetMMIdx(pt)] == DO_BUILDING && !houses) || // achten, da dort auch nur das Player-
                            (dos[GetMMIdx(pt)] == DO_ROAD && !roads)))) // Territorium zu sehen ist!
                {
                    unsigned color = CalcPixelColor(pt, t);
                    map.tex_setPixel((x * 2 + t + (y & 1)) % (map_width * 2), y, GetRed(color), GetGreen(color),
                        GetBlue(color), GetAlpha(color));
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
*  Die einzelnen Dinge umschalten
*
*  @author OLiver
*/
void IngameMinimap::ToggleTerritory()
{
    territory = !territory;
    UpdateAll(DO_PLAYER);
}

///////////////////////////////////////////////////////////////////////////////
/**
*
*  @author OLiver
*/
void IngameMinimap::ToggleHouses()
{
    houses = !houses;
    UpdateAll(DO_BUILDING);
}

///////////////////////////////////////////////////////////////////////////////
/**
*
*  @author OLiver
*/
void IngameMinimap::ToggleRoads()
{
    roads = !roads;
    UpdateAll(DO_ROAD);
}
