// $Id: Minimap.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "main.h"
#include "Minimap.h"
#include "MinimapConsts.h"
#include "GameWorld.h"
#include "GameClient.h"

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *  @author OLiver
 */
Minimap::Minimap(const unsigned short map_width, const unsigned short map_height)
    : map_width(map_width), map_height(map_height)
{

}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *  @author OLiver
 */
void Minimap::CreateMapTexture(const void* param)
{
    map.DeleteTexture();

    if(!param)
        return;

    /// Buffer für die Daten erzeugen
    unsigned char* buffer = new unsigned char[map_width * 2 * map_height * 4];

    for(MapCoord y = 0; y < map_height; ++y)
    {
        for(MapCoord x = 0; x < map_width; ++x)
        {
            // Die 2. Terraindreiecke durchgehen
            for(unsigned t = 0; t < 2; ++t)
            {
                unsigned color = CalcPixelColor(param, x, y, t);

                unsigned pos  = y * map_width * 4 * 2 + (x * 4 * 2 + t * 4 + (y & 1) * 4) % (map_width * 4 * 2);
                buffer[pos + 2] = GetRed(color);
                buffer[pos + 1] = GetGreen(color);
                buffer[pos]   = GetBlue(color);
                buffer[pos + 3] = GetAlpha(color);
            }
        }
    }

    map.setFilter(GL_LINEAR);
    map.create(map_width * 2, map_height, buffer, map_width * 2, map_height,
               libsiedler2::FORMAT_RGBA, LOADER.GetPaletteN("pal5"));

    delete [] buffer;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *  @author OLiver
 */
void Minimap::Draw(const unsigned short x, const unsigned short y, const unsigned short width, const unsigned short height)
{
    BeforeDrawing();

    // Map ansich zeichnen
    map.Draw(x, y, width, height, 0, 0, 0, 0, COLOR_WHITE);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *  @author OLiver
 */
void Minimap::BeforeDrawing()
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Variiert die übergebene Farbe zufällig in der Helligkeit
 *
 *  @author OLiver
 */
unsigned Minimap::VaryBrightness(const unsigned color, const int range) const
{
    int add = 100 - rand() % (2 * range);

    int red = GetRed(color) * add / 100;
    if(red < 0) red = 0;
    else if(red > 0xFF) red = 0xFF;
    int green = GetGreen(color) * add / 100;
    if(green < 0) green = 0;
    else if(green > 0xFF) green = 0xFF;
    int blue = GetBlue(color) * add / 100;
    if(blue < 0) blue = 0;
    else if(blue > 0xFF) blue = 0xFF;

    return MakeColor(GetAlpha(color), red, green, blue);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *  @author OLiver
 */
PreviewMinimap::PreviewMinimap(glArchivItem_Map* s2map)
{
    SetMap(s2map);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *  @author OLiver
 */
void Minimap::SetMap(glArchivItem_Map* s2map)
{
    if(s2map)
    {
        map_width = s2map->getHeader().getWidth();
        map_height = s2map->getHeader().getHeight();
        CreateMapTexture(s2map);
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *  @author OLiver
 */
unsigned PreviewMinimap::CalcPixelColor(const void* param, const MapCoord x, const MapCoord y, const unsigned t)
{
    const glArchivItem_Map& s2map = *static_cast<const glArchivItem_Map*>(param);
    unsigned color = 0;
    // Baum an dieser Stelle?
    unsigned char landscape_obj = s2map.GetMapDataAt(MAP_TYPE, x, y);
    if(landscape_obj >= 0xC4 && landscape_obj <= 0xC6)
        color = VaryBrightness(TREE_COLOR, VARY_TREE_COLOR);
    // Granit an dieser Stelle?
    else if(landscape_obj == 0xCC || landscape_obj == 0xCD)
        color = VaryBrightness(GRANITE_COLOR, VARY_GRANITE_COLOR);
    // Ansonsten die jeweilige Terrainfarbe nehmen
    else
    {
        color = TERRAIN_COLORS[s2map.getHeader().getGfxSet()]
                [TERRAIN_INDIZES[s2map.GetMapDataAt(MapLayer(MAP_TERRAIN1 + t), x, y)]];

        // Schattierung
        int r = GetRed(color) + s2map.GetMapDataAt(MAP_SHADOWS, x, y) - 0x40;
        int g = GetGreen(color) + s2map.GetMapDataAt(MAP_SHADOWS, x, y) - 0x40;
        int b = GetBlue(color) + s2map.GetMapDataAt(MAP_SHADOWS, x, y) - 0x40;

        if(r < 0) r = 0;
        if(r > 255) r = 255;
        if(g < 0) g = 0;
        if(g > 255) g = 255;
        if(b < 0) b = 0;
        if(b > 255) b = 255;

        color = MakeColor(0xFF, unsigned(r), unsigned(g), unsigned(b));
    }

    return color;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *  @author OLiver
 */
IngameMinimap::IngameMinimap(const GameWorldViewer& gwv) :
    Minimap(gwv.GetWidth(), gwv.GetHeight()), gwv(gwv), nodes_updated(gwv.GetWidth()*gwv.GetHeight(), false),
    dos(gwv.GetWidth()*gwv.GetHeight(), DO_INVALID), territory(true), houses(true), roads(true)
{
    CreateMapTexture(&gwv);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *  @author OLiver
 */
unsigned IngameMinimap::CalcPixelColor(const void* param, const MapCoord x, const MapCoord y, const unsigned t)
{
    const GameWorldViewer& gwv = *static_cast<const GameWorldViewer*>(param);

    unsigned color = 0;

    // Beobeachtender Spieler
    unsigned char viewing_player = GameClient::inst().GetPlayerID();

    Visibility visibility = gwv.GetVisibility(x, y);

    if(visibility == VIS_INVISIBLE)
    {
        dos[y * map_width + x] = DO_INVISIBLE;
        // Man sieht nichts --> schwarz
        return 0xFF000000;
    }
    else
    {
        DrawnObject drawn_object = DO_INVALID;

        bool fow = (visibility == VIS_FOW);

        unsigned char owner;
        if(!fow)
            owner = gwv.GetNode(x, y).owner;
        else
            owner = gwv.GetNode(x, y).fow[GameClient::inst().GetPlayerID()].owner;

        // Baum an dieser Stelle?
        if((!fow && gwv.GetNO(x, y)->GetGOT() == GOT_TREE) || (fow && gwv.GetFOWObject(x, y, viewing_player)->GetType() == FOW_TREE))
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
        else if((!fow && gwv.GetNO(x, y)->GetGOT() == GOT_GRANITE) || (fow && gwv.GetFOWObject(x, y, viewing_player)->GetType() == FOW_GRANITE))
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
                GO_Type got = gwv.GetNO(x, y)->GetGOT();
                FOW_Type fot = gwv.GetFOWObject(x, y, viewing_player)->GetType();

                if(((!fow && (got == GOT_NOB_USUAL || got == GOT_NOB_MILITARY ||
                              got == GOT_NOB_STOREHOUSE || got == GOT_NOB_USUAL ||
                              got == GOT_NOB_HQ || got == GOT_BUILDINGSITE)) || (fow && (fot == FOW_BUILDING || fot == FOW_BUILDINGSITE))))
                    drawn_object = DO_BUILDING;
                /// Straßen?
                else if(IsRoad(x, y, visibility))
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
                    color = CombineWithPlayerColor(CalcTerrainColor(x, y, t), owner);
                else
                    // Normales Terrain berechnen
                    color = CalcTerrainColor(x, y, t);
            }
            else
            {
                // Normales Terrain berechnen
                color = CalcTerrainColor(x, y, t);
                drawn_object = DO_TERRAIN;
            }
        }

        // Bei FOW die Farben abdunkeln
        if(fow)
            color = MakeColor(0xFF, GetRed(color) / 2, GetGreen(color) / 2, GetBlue(color) / 2);

        dos[y * map_width + x] = drawn_object;
    }

    return color;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Berechnet für einen bestimmten Punkt und ein Dreieck die normale Terrainfarbe
 *
 *  @author OLiver
 */
unsigned IngameMinimap::CalcTerrainColor(const MapCoord x, const MapCoord y, const unsigned t)
{
    unsigned color = TERRAIN_COLORS[gwv.GetLandscapeType()][ (t == 0) ? gwv.GetNode(x, y).t1 : gwv.GetNode(x, y).t2];

    // Schattierung
    int shadow = gwv.GetNode(x, y).shadow;
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
bool IngameMinimap::IsRoad(const MapCoord x, const MapCoord y, const Visibility visibility)
{
    for(unsigned i = 0; i < 3; ++i)
    {
        if(gwv.GetVisibleRoad(x, y, i, visibility))
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
    unsigned player_color = COLORS[GameClient::inst().GetPlayer(player - 1)->color];

    return MakeColor(0xFF, (GetRed(color) + GetRed(player_color)) / 2,
                     (GetGreen(color) + GetGreen(player_color)) / 2,
                     (GetBlue(color) + GetBlue(player_color)) / 2);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *  @author OLiver
 */
void IngameMinimap::UpdateNode(const MapCoord x, const MapCoord y)
{
    if(!nodes_updated[y * map_width + x])
    {
        nodes_updated[y * map_width + x] = true;
        Node node = { x, y };
        nodes_updated_list.push_back(node);
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

    // Überhaupt Änderungen nötig?
    if(nodes_updated_list.size())
    {
        // Komplette Textur neu erzeugen, weil es zu viele Knoten sind?
        if(nodes_updated_list.size() >= map_width * map_height / MAX_NODES_UPDATE_DENOMINATOR)
        {
            // Ja, alles neu erzeugen
            UpdateAll();
            // Alles Aktualisierungen wieder zurücksetzen
            for(MapCoord y = 0; y < map_height; ++y)
            {
                for(MapCoord x = 0; x < map_width; ++x)
                    nodes_updated[y * map_width + x] = false;
            }
        }
        else
        {
            // Entsprechende Pixel updaten
            for(list<Node>::iterator it = nodes_updated_list.begin(); it.valid(); ++it)
            {
                for(unsigned t = 0; t < 2; ++t)
                {
                    unsigned color = CalcPixelColor(&gwv, it->x, it->y, t);
                    map.tex_setPixel((it->x * 2 + t + (it->y & 1)) % (map_width * 2), it->y, GetRed(color), GetGreen(color),
                                     GetBlue(color), GetAlpha(color));
                }
                // Jetzt muss er nicht mehr geändert werden
                nodes_updated[it->y * map_width + it->x] = false;
            }
        }

        this->nodes_updated_list.clear();
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
    CreateMapTexture(&gwv);
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
            for(unsigned t = 0; t < 2; ++t)
            {
                if(dos[y * map_width + x] == drawn_object || // das gewünschte Objekt
                        (drawn_object == DO_PLAYER && // bei DO_PLAYER auf evtl. nicht gezeichnete Häuser und Straßen
                         ((dos[y * map_width + x] == DO_BUILDING && !houses) || // achten, da dort auch nur das Player-
                          (dos[y * map_width + x] == DO_ROAD && !roads)))) // Territorium zu sehen ist!
                {
                    unsigned color = CalcPixelColor(&gwv, x, y, t);
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
