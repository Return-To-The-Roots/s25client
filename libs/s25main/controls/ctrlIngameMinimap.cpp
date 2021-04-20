// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlIngameMinimap.h"
#include "CollisionDetection.h"
#include "IngameMinimap.h"
#include "Loader.h"
#include "driver/MouseCoords.h"
#include "world/GameWorldView.h"
#include "world/MapGeometry.h"
class Window;

ctrlIngameMinimap::ctrlIngameMinimap(Window* parent, const unsigned id, const DrawPoint& pos, const Extent& size,
                                     const Extent& padding, IngameMinimap& minimap, GameWorldView& gwv)
    : ctrlMinimap(parent, id, pos, size, padding, Extent(minimap.GetMapSize())), minimap(minimap), gwv(gwv)
{}

/**
 *  Zeichnet die MapPreview
 */
void ctrlIngameMinimap::Draw_()
{
    DrawMap(minimap);

    // Mittleren Punkt berechnen und dort hinscrollen
    Position middlePt = (gwv.GetLastPt() + gwv.GetFirstPt()) / 2;

    // Koordinaten korrigieren
    DrawPoint middle_corrected(MakeMapPoint(middlePt, minimap.GetMapSize()));

    // Scroll-Auswahl-Bild holen
    glArchivItem_Bitmap* image = LOADER.GetMapImageN(23);

    // Position (relativ zum angezeigten Anfang der Karte) berechnen
    DrawPoint pos = middle_corrected * DrawPoint(GetCurMapSize()) / DrawPoint(minimap.GetMapSize()) + DrawPoint(2, 2);

    // Scroll-Auswahl-Bild an den Rändern verkleinern, damit es nicht über die Karte "überlappt"
    DrawPoint srcOffset(0, 0);
    Extent drawSize(image->getWidth(), image->getHeight());

    DrawPoint originOffset = pos - image->GetOrigin();

    // überlappung am linken Rand?
    if(originOffset.x < 0)
    {
        srcOffset.x = -originOffset.x;
        drawSize.x += originOffset.x;
        pos.x = image->getNx();
    }
    // überlappung am oberen Rand?
    if(originOffset.y < 0)
    {
        srcOffset.y = -originOffset.y;
        drawSize.y += originOffset.y;
        pos.y = image->getNy();
    }
    // überlappung am rechten Rand?
    DrawPoint overDrawSize = pos - image->GetOrigin() + DrawPoint(image->GetSize()) - DrawPoint(GetCurMapSize());
    if(overDrawSize.x >= 0)
        drawSize.x -= overDrawSize.x;
    // überlappung am unteren Rand?
    if(overDrawSize.y >= 0)
        drawSize.y -= overDrawSize.y;

    // Zeichnen
    image->DrawPart(Rect(GetMapDrawArea().getOrigin() + pos, drawSize), srcOffset);
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
        if(IsPointInRect(mc.GetPos(), GetMapDrawArea()))
        {
            // Koordinate feststellen
            DrawPoint mapCoord = (mc.GetPos() - DrawPoint(GetMapDrawArea().getOrigin()))
                                 * DrawPoint(minimap.GetMapSize()) / DrawPoint(GetCurMapSize());

            gwv.MoveToMapPt(MapPoint(mapCoord));

            return true;
        }
    }

    return false;
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
