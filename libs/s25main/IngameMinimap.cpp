// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "IngameMinimap.h"
#include "FOWObjects.h"
#include "GamePlayer.h"
#include "RttrForeachPt.h"
#include "world/GameWorldBase.h"
#include "world/GameWorldViewer.h"
#include "gameData/MinimapConsts.h"
#include "gameData/TerrainDesc.h"
#include "libsiedler2/ColorBGRA.h"

IngameMinimap::IngameMinimap(const GameWorldViewer& gwv)
    : Minimap(gwv.GetWorld().GetSize()), gwv(gwv), nodes_updated(GetMapSize().x * GetMapSize().y, false),
      dos(GetMapSize().x * GetMapSize().y, DrawnObject::Invalid), territory(true), houses(true), roads(true)
{
    CreateMapTexture();
}

unsigned IngameMinimap::CalcPixelColor(const MapPoint pt, const unsigned t)
{
    unsigned color = 0;

    Visibility visibility = gwv.GetVisibility(pt);

    if(visibility == Visibility::Invisible)
    {
        dos[GetMMIdx(pt)] = DrawnObject::Invisible;
        // Man sieht nichts --> schwarz
        return 0xFF000000;
    } else
    {
        DrawnObject drawn_object = DrawnObject::Invalid;

        const bool fow = (visibility == Visibility::FogOfWar);

        unsigned char owner;
        NodalObjectType noType = NodalObjectType::Nothing;
        FoW_Type fot = FoW_Type::Nothing;
        if(!fow)
        {
            const MapNode& node = gwv.GetNode(pt);
            owner = node.owner;
            if(node.obj)
                noType = node.obj->GetType();
        } else
        {
            const FoWNode& node = gwv.GetYoungestFOWNode(pt);
            owner = node.owner;
            if(node.object)
                fot = node.object->GetType();
        }

        // Baum an dieser Stelle?
        if((!fow && noType == NodalObjectType::Tree) || (fow && fot == FoW_Type::Tree)) //-V807
        {
            color = VaryBrightness(TREE_COLOR, VARY_TREE_COLOR);
            drawn_object = DrawnObject::Terrain;
            // Ggf. mit Spielerfarbe
            if(owner)
            {
                drawn_object = DrawnObject::Player;
                if(territory)
                    color = CombineWithPlayerColor(color, owner);
            }
        }
        // Granit an dieser Stelle?
        else if((!fow && noType == NodalObjectType::Granite) || (fow && fot == FoW_Type::Granite))
        {
            color = VaryBrightness(GRANITE_COLOR, VARY_GRANITE_COLOR);
            drawn_object = DrawnObject::Terrain;
            // Ggf. mit Spielerfarbe
            if(owner)
            {
                drawn_object = DrawnObject::Player;
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
                // Building?
                if(((!fow && (noType == NodalObjectType::Building || noType == NodalObjectType::Buildingsite))
                    || (fow && (fot == FoW_Type::Building || fot == FoW_Type::Buildingsite))))
                    drawn_object = DrawnObject::Buidling;
                /// Roads?
                else if(IsRoad(pt, visibility))
                    drawn_object = DrawnObject::Road;
                // ansonsten normales Territorium?
                else
                    drawn_object = DrawnObject::Player;

                if(drawn_object == DrawnObject::Buidling && houses)
                    color = BUILDING_COLOR;
                /// Roads?
                else if(drawn_object == DrawnObject::Road && roads)
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
                drawn_object = DrawnObject::Terrain;
            }
        }

        // Bei FOW die Farben abdunkeln
        if(fow)
            color = MakeColor(0xFF, GetRed(color) / 2, GetGreen(color) / 2, GetBlue(color) / 2);

        dos[GetMMIdx(pt)] = drawn_object;
    }

    return color;
}

/**
 *  Calculate the normal terrain color for a given point and triangle
 */
unsigned IngameMinimap::CalcTerrainColor(const MapPoint pt, const unsigned t)
{
    unsigned color =
      gwv.GetWorld().GetDescription().get((t == 0) ? gwv.GetNode(pt).t1 : gwv.GetNode(pt).t2).minimapColor; //-V807

    // Schattierung
    int shadow = gwv.GetNode(pt).shadow;
    int r = GetRed(color) + shadow - 0x40;
    int g = GetGreen(color) + shadow - 0x40;
    int b = GetBlue(color) + shadow - 0x40;

    if(r < 0)
        r = 0;
    else if(r > 255)
        r = 255;
    if(g < 0)
        g = 0;
    else if(g > 255)
        g = 255;
    if(b < 0)
        b = 0;
    else if(b > 255)
        b = 255;

    return MakeColor(0xFF, unsigned(r), unsigned(g), unsigned(b));
}

/**
 *  Check if there is a road at that point
 */
bool IngameMinimap::IsRoad(const MapPoint pt, const Visibility visibility)
{
    for(const auto dir : helpers::EnumRange<RoadDir>{})
    {
        if(gwv.GetVisibleRoad(pt, dir, visibility) != PointRoad::None)
            return true;
    }

    return false;
}

/**
 *  Berechnet Spielerfarbe mit in eine gegebene Farbe mit ein
 *  (player muss mit +1 gegeben sein!)
 */
unsigned IngameMinimap::CombineWithPlayerColor(const unsigned color, const unsigned char player) const
{
    // Spielerfarbe mit einberechnen
    unsigned player_color = gwv.GetWorld().GetPlayer(player - 1).color;

    return MakeColor(0xFF, (GetRed(color) + GetRed(player_color)) / 2, (GetGreen(color) + GetGreen(player_color)) / 2,
                     (GetBlue(color) + GetBlue(player_color)) / 2);
}

void IngameMinimap::UpdateNode(const MapPoint pt)
{
    if(!nodes_updated[GetMMIdx(pt)])
    {
        nodes_updated[GetMMIdx(pt)] = true;
        nodesToUpdate.push_back(pt);
    }
}

/**
 *  Additional stuff we need to do before drawing (in this case: update the map)
 */
void IngameMinimap::BeforeDrawing()
{
    // Ab welcher Knotenanzahl (Teil der Gesamtknotenanzahl) die Textur komplett neu erstellt werden soll
    static const unsigned MAX_NODES_UPDATE_DENOMINATOR = 2; // (2 = 1/2, 3 = 1/3 usw.)

    if(!nodesToUpdate.empty())
    {
        // Komplette Textur neu erzeugen, weil es zu viele Knoten sind?
        if(nodesToUpdate.size() >= nodes_updated.size() / MAX_NODES_UPDATE_DENOMINATOR)
        {
            // Ja, alles neu erzeugen
            UpdateAll();
            std::fill(nodes_updated.begin(), nodes_updated.end(), false);
        } else
        {
            map.beginUpdate();
            // Entsprechende Pixel updaten
            for(auto& it : nodesToUpdate)
            {
                for(unsigned t = 0; t < 2; ++t)
                {
                    unsigned color = CalcPixelColor(it, t);
                    DrawPoint texPos((it.x * 2 + t + (it.y & 1)) % (GetMapSize().x * 2), it.y);
                    map.updatePixel(texPos, libsiedler2::ColorBGRA(color));
                }
                nodes_updated[GetMMIdx(it)] = false;
            }
            map.endUpdate();
        }

        this->nodesToUpdate.clear();
    }
}

/**
 *  Updatet die gesamte Minimap
 */
void IngameMinimap::UpdateAll()
{
    map.DeleteTexture();
    CreateMapTexture();
}

/**
 *  Update all nodes with the given drawn object
 */
void IngameMinimap::UpdateAll(const DrawnObject drawn_object)
{
    map.beginUpdate();
    // Gesamte Karte neu berechnen
    RTTR_FOREACH_PT(MapPoint, GetMapSize())
    {
        for(unsigned t = 0; t < 2; ++t)
        {
            if(dos[GetMMIdx(pt)] == drawn_object
               || (drawn_object == DrawnObject::Player && // for DrawnObject::Player check for not drawn buildings or
                                                          // roads as there is only the player territory visible
                   ((dos[GetMMIdx(pt)] == DrawnObject::Buidling && !houses)
                    || (dos[GetMMIdx(pt)] == DrawnObject::Road && !roads))))
            {
                unsigned color = CalcPixelColor(pt, t);
                DrawPoint texPos((pt.x * 2 + t + (pt.y & 1)) % (GetMapSize().x * 2), pt.y);
                map.updatePixel(texPos, libsiedler2::ColorBGRA(color));
            }
        }
    }
    map.endUpdate();
}

void IngameMinimap::ToggleTerritory()
{
    territory = !territory;
    UpdateAll(DrawnObject::Player);
}

void IngameMinimap::ToggleHouses()
{
    houses = !houses;
    UpdateAll(DrawnObject::Buidling);
}

void IngameMinimap::ToggleRoads()
{
    roads = !roads;
    UpdateAll(DrawnObject::Road);
}
