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
#include "ctrlIngameMinimap.h"
#include "world/GameWorldView.h"
#include "world/MapGeometry.h"
#include "IngameMinimap.h"
#include "driver/src/MouseCoords.h"
#include "CollisionDetection.h"
#include "Loader.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep
class Window;

ctrlIngameMinimap::ctrlIngameMinimap( Window* parent,
                                      const unsigned int id,
                                      const unsigned short x,
                                      const unsigned short y,
                                      const unsigned short width,
                                      const unsigned short height,
                                      const unsigned short padding_x,
                                      const unsigned short padding_y,
                                      IngameMinimap& minimap,
                                      GameWorldView& gwv)
    : ctrlMinimap(parent, id, x, y, width, height, padding_x, padding_y, minimap.GetMapWidth(),
                  minimap.GetMapHeight()), minimap(minimap), gwv(gwv)
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Zeichnet die MapPreview
 *
 *  @author OLiver
 */
bool ctrlIngameMinimap::Draw_()
{
    DrawMap(minimap);

    // Mittleren Punkt berechnen und dort hinscrollen
    Point<int> middlePt = (gwv.GetLastPt() + gwv.GetFirstPt()) / 2;

    // Koordinaten korrigieren
    MapPoint middle_corrected = MakeMapPoint(middlePt, minimap.GetMapWidth(), minimap.GetMapHeight());

    // Scroll-Auswahl-Bild holen
    glArchivItem_Bitmap* image = LOADER.GetMapImageN(23);

    // Position (relativ zum angezeigten Anfang der Karte) berechnen
    short xpos = middle_corrected.x * width_show / minimap.GetMapWidth() + 2;
    short ypos = middle_corrected.y * height_show / minimap.GetMapHeight() + 2;

    // Scroll-Auswahl-Bild an den Rändern verkleinern, damit es nicht über die Karte "überlappt"
    short src_x = 0, src_y = 0;
    short draw_width = image->getWidth();
    short draw_height = image->getHeight();

    // überlappung am linken Rand?
    if(xpos - image->getNx() < 0)
    {
        src_x = -(xpos - image->getNx());
        draw_width += (xpos - image->getNx());
        xpos = image->getNx();
    }
    // überlappung am oberen Rand?
    if(ypos - image->getNy() < 0)
    {
        src_y = -(ypos - image->getNy());
        draw_height += (ypos - image->getNy());
        ypos = image->getNy();
    }
    // überlappung am rechten Rand?
    if(xpos - image->getNx() + image->getWidth() >= width_show)
        draw_width -= (xpos - image->getNx() + image->getWidth() - width_show);
    // überlappung am unteren Rand?
    if(ypos - image->getNy() + image->getHeight() >= height_show)
        draw_height -= (ypos - image->getNy() + image->getHeight() - height_show);

    // Zeichnen
    image->Draw(GetX() + GetLeft() + xpos, GetY() + GetTop() + ypos, 0, 0, src_x, src_y, draw_width, draw_height);

    return true;
}

bool ctrlIngameMinimap::Msg_LeftDown(const MouseCoords& mc)
{
    return Msg_MouseMove(mc);
}

bool ctrlIngameMinimap::Msg_MouseMove(const MouseCoords& mc)
{
    if(mc.ldown)
    {
        // Mauszeiger auf der Karte?
        if(Coll(mc.x, mc.y, GetX() + GetLeft(), GetY() + GetTop(), width_show, height_show))
        {
            // Koordinate feststellen
            unsigned short map_x = (mc.x - (GetX() + GetLeft())) * minimap.GetMapWidth() / width_show;
            unsigned short map_y = (mc.y - (GetY() + GetTop())) * minimap.GetMapHeight() / height_show;

            gwv.MoveToMapPt(MapPoint(map_x, map_y));

            return true;
        }
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Setzt Breite und Höhe des Controls
 *
 *  @author OLiver
 */
void ctrlIngameMinimap::SetDisplaySize(const unsigned short width, const unsigned short height)
{
    ctrlMinimap::SetDisplaySize(width, height, minimap.GetMapWidth(), minimap.GetMapHeight());
}

void ctrlIngameMinimap::ToggleTerritory()
{
    minimap.ToggleTerritory();
}

void ctrlIngameMinimap::ToggleHouses()
{
    minimap.ToggleHouses();
}

void ctrlIngameMinimap::ToggleRoads()
{
    minimap.ToggleRoads();
}
