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
    : Minimap(gwv.getWorld().getSize()), gwv(gwv), nodes_updated(getMapSize().x * getMapSize().y, false),
      dos(getMapSize().x * getMapSize().y, DrawnObject::Invalid), territory(true), houses(true), roads(true)
{
    createMapTexture();
}

unsigned IngameMinimap::calcPixelColor(const MapPoint pt, const unsigned t)
{
    unsigned color = 0;

    Visibility visibility = gwv.getVisibility(pt);

    if(visibility == Visibility::Invisible)
    {
        dos[getMMIdx(pt)] = DrawnObject::Invisible;
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
            const MapNode& node = gwv.getNode(pt);
            owner = node.owner;
            if(node.obj)
                noType = node.obj->getType();
        } else
        {
            const FoWNode& node = gwv.getYoungestFOWNode(pt);
            owner = node.owner;
            if(node.object)
                fot = node.object->getType();
        }

        // Baum an dieser Stelle?
        if((!fow && noType == NodalObjectType::Tree) || (fow && fot == FoW_Type::Tree)) //-V807
        {
            color = varyBrightness(TREE_COLOR, VARY_TREE_COLOR);
            drawn_object = DrawnObject::Terrain;
            // Ggf. mit Spielerfarbe
            if(owner)
            {
                drawn_object = DrawnObject::Player;
                if(territory)
                    color = combineWithPlayerColor(color, owner);
            }
        }
        // Granit an dieser Stelle?
        else if((!fow && noType == NodalObjectType::Granite) || (fow && fot == FoW_Type::Granite))
        {
            color = varyBrightness(GRANITE_COLOR, VARY_GRANITE_COLOR);
            drawn_object = DrawnObject::Terrain;
            // Ggf. mit Spielerfarbe
            if(owner)
            {
                drawn_object = DrawnObject::Player;
                if(territory)
                    color = combineWithPlayerColor(color, owner);
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
                else if(isRoad(pt, visibility))
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
                    color = combineWithPlayerColor(calcTerrainColor(pt, t), owner);
                else
                    // Normales Terrain berechnen
                    color = calcTerrainColor(pt, t);
            } else
            {
                // Normales Terrain berechnen
                color = calcTerrainColor(pt, t);
                drawn_object = DrawnObject::Terrain;
            }
        }

        // Bei FOW die Farben abdunkeln
        if(fow)
            color = makeColor(0xFF, getRed(color) / 2, getGreen(color) / 2, getBlue(color) / 2);

        dos[getMMIdx(pt)] = drawn_object;
    }

    return color;
}

/**
 *  Calculate the normal terrain color for a given point and triangle
 */
unsigned IngameMinimap::calcTerrainColor(const MapPoint pt, const unsigned t)
{
    unsigned color =
      gwv.getWorld().getDescription().get((t == 0) ? gwv.getNode(pt).t1 : gwv.getNode(pt).t2).minimapColor; //-V807

    // Schattierung
    int shadow = gwv.getNode(pt).shadow;
    int r = getRed(color) + shadow - 0x40;
    int g = getGreen(color) + shadow - 0x40;
    int b = getBlue(color) + shadow - 0x40;

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

    return makeColor(0xFF, unsigned(r), unsigned(g), unsigned(b));
}

/**
 *  Check if there is a road at that point
 */
bool IngameMinimap::isRoad(const MapPoint pt, const Visibility visibility)
{
    for(const auto dir : helpers::EnumRange<RoadDir>{})
    {
        if(gwv.getVisibleRoad(pt, dir, visibility) != PointRoad::None)
            return true;
    }

    return false;
}

/**
 *  Berechnet Spielerfarbe mit in eine gegebene Farbe mit ein
 *  (player muss mit +1 gegeben sein!)
 */
unsigned IngameMinimap::combineWithPlayerColor(const unsigned color, const unsigned char player) const
{
    // Spielerfarbe mit einberechnen
    unsigned player_color = gwv.getWorld().getPlayer(player - 1).color;

    return makeColor(0xFF, (getRed(color) + getRed(player_color)) / 2, (getGreen(color) + getGreen(player_color)) / 2,
                     (getBlue(color) + getBlue(player_color)) / 2);
}

void IngameMinimap::updateNode(const MapPoint pt)
{
    if(!nodes_updated[getMMIdx(pt)])
    {
        nodes_updated[getMMIdx(pt)] = true;
        nodesToUpdate.push_back(pt);
    }
}

/**
 *  Additional stuff we need to do before drawing (in this case: update the map)
 */
void IngameMinimap::beforeDrawing()
{
    // Ab welcher Knotenanzahl (Teil der Gesamtknotenanzahl) die Textur komplett neu erstellt werden soll
    static const unsigned MAX_NODES_UPDATE_DENOMINATOR = 2; // (2 = 1/2, 3 = 1/3 usw.)

    if(!nodesToUpdate.empty())
    {
        // Komplette Textur neu erzeugen, weil es zu viele Knoten sind?
        if(nodesToUpdate.size() >= nodes_updated.size() / MAX_NODES_UPDATE_DENOMINATOR)
        {
            // Ja, alles neu erzeugen
            updateAll();
            std::fill(nodes_updated.begin(), nodes_updated.end(), false);
        } else
        {
            map.beginUpdate();
            // Entsprechende Pixel updaten
            for(auto& it : nodesToUpdate)
            {
                for(unsigned t = 0; t < 2; ++t)
                {
                    unsigned color = calcPixelColor(it, t);
                    DrawPoint texPos((it.x * 2 + t + (it.y & 1)) % (getMapSize().x * 2), it.y);
                    map.updatePixel(texPos, libsiedler2::ColorBGRA(color));
                }
                nodes_updated[getMMIdx(it)] = false;
            }
            map.endUpdate();
        }

        this->nodesToUpdate.clear();
    }
}

/**
 *  Updatet die gesamte Minimap
 */
void IngameMinimap::updateAll()
{
    map.deleteTexture();
    createMapTexture();
}

/**
 *  Update all nodes with the given drawn object
 */
void IngameMinimap::updateAll(const DrawnObject drawn_object)
{
    map.beginUpdate();
    // Gesamte Karte neu berechnen
    RTTR_FOREACH_PT(MapPoint, getMapSize())
    {
        for(unsigned t = 0; t < 2; ++t)
        {
            if(dos[getMMIdx(pt)] == drawn_object
               || (drawn_object == DrawnObject::Player && // for DrawnObject::Player check for not drawn buildings or
                                                          // roads as there is only the player territory visible
                   ((dos[getMMIdx(pt)] == DrawnObject::Buidling && !houses)
                    || (dos[getMMIdx(pt)] == DrawnObject::Road && !roads))))
            {
                unsigned color = calcPixelColor(pt, t);
                DrawPoint texPos((pt.x * 2 + t + (pt.y & 1)) % (getMapSize().x * 2), pt.y);
                map.updatePixel(texPos, libsiedler2::ColorBGRA(color));
            }
        }
    }
    map.endUpdate();
}

void IngameMinimap::toggleTerritory()
{
    territory = !territory;
    updateAll(DrawnObject::Player);
}

void IngameMinimap::toggleHouses()
{
    houses = !houses;
    updateAll(DrawnObject::Buidling);
}

void IngameMinimap::toggleRoads()
{
    roads = !roads;
    updateAll(DrawnObject::Road);
}
