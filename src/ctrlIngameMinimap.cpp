// $Id: ctrlIngameMinimap.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "ctrlIngameMinimap.h"
#include "Minimap.h"
#include "GameWorld.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
ctrlIngameMinimap::ctrlIngameMinimap( Window* parent,
                                      const unsigned int id,
                                      const unsigned short x,
                                      const unsigned short y,
                                      const unsigned short width,
                                      const unsigned short height,
                                      const unsigned short padding_x,
                                      const unsigned short padding_y,
                                      IngameMinimap* minimap,
                                      GameWorldViewer& gwv)
    : ctrlMinimap(parent, id, x, y, width, height, padding_x, padding_y, minimap->GetMapWidth(),
                  minimap->GetMapHeight()), minimap(minimap), gwv(gwv)
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
    DrawMap(*minimap);

    // Mittleren Punkt berechnen und dort hinscrollen
    int middle_x = (gwv.GetLastX() + gwv.GetFirstX()) / 2;
    int middle_y = (gwv.GetLastY() + gwv.GetFirstY()) / 2;

    // Koordinaten korrigieren
    MapCoord middle_corrected_x, middle_corrected_y;
    gwv.ConvertCoords(middle_x, middle_y, &middle_corrected_x, &middle_corrected_y);

    // Scroll-Auswahl-Bild holen
    glArchivItem_Bitmap* image = LOADER.GetMapImageN(23);

    // Position (relativ zum angezeigten Anfang der Karte) berechnen
    short xpos = middle_corrected_x * width_show / minimap->GetMapWidth() + 2;
    short ypos = middle_corrected_y * height_show / minimap->GetMapHeight() + 2;

    // Scroll-Auswahl-Bild an den Rändern verkleinern, damit es nicht über die Karte "überlappt"
    short src_x = 0, src_y = 0;
    short draw_width = image->getWidth();
    short draw_height = image->getHeight();

    // Überlappung am linken Rand?
    if(xpos - image->getNx() < 0)
    {
        src_x = -(xpos - image->getNx());
        draw_width += (xpos - image->getNx());
        xpos = image->getNx();
    }
    // Überlappung am oberen Rand?
    if(ypos - image->getNy() < 0)
    {
        src_y = -(ypos - image->getNy());
        draw_height += (ypos - image->getNy());
        ypos = image->getNy();
    }
    // Überlappung am rechten Rand?
    if(xpos - image->getNx() + image->getWidth() >= width_show)
        draw_width -= (xpos - image->getNx() + image->getWidth() - width_show);
    // Überlappung am unteren Rand?
    if(ypos - image->getNy() + image->getHeight() >= height_show)
        draw_height -= (ypos - image->getNy() + image->getHeight() - height_show);

    // Zeichnen
    image->Draw(GetX() + GetLeft() + xpos, GetY() + GetTop() + ypos, 0, 0, src_x, src_y, draw_width, draw_height);

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
bool ctrlIngameMinimap::Msg_LeftDown(const MouseCoords& mc)
{
    return Msg_MouseMove(mc);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
bool ctrlIngameMinimap::Msg_MouseMove(const MouseCoords& mc)
{
    if(mc.ldown)
    {
        // Mauszeiger auf der Karte?
        if(Coll(mc.x, mc.y, GetX() + GetLeft(), GetY() + GetTop(), width_show, height_show))
        {
            // Koordinate feststellen
            unsigned short map_x = (mc.x - (GetX() + GetLeft())) * minimap->GetMapWidth() / width_show;
            unsigned short map_y = (mc.y - (GetY() + GetTop())) * minimap->GetMapHeight() / height_show;

            gwv.MoveToMapObject(map_x, map_y);

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
    ctrlMinimap::SetDisplaySize(width, height, minimap->GetMapWidth(), minimap->GetMapHeight());
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void ctrlIngameMinimap::ToggleTerritory()
{
    minimap->ToggleTerritory();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void ctrlIngameMinimap::ToggleHouses()
{
    minimap->ToggleHouses();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void ctrlIngameMinimap::ToggleRoads()
{
    minimap->ToggleRoads();
}
