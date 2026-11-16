// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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
    : ctrlMinimap(parent, id, pos, size, padding, Extent(minimap.getMapSize())), minimap(minimap), gwv(gwv)
{}

/**
 *  Zeichnet die MapPreview
 */
void ctrlIngameMinimap::drawImpl()
{
    drawMap(minimap);

    // Mittleren Punkt berechnen und dort hinscrollen
    Position middlePt = (gwv.getLastPt() + gwv.getFirstPt()) / 2;

    // Koordinaten korrigieren
    DrawPoint middle_corrected(makeMapPoint(middlePt, minimap.getMapSize()));

    // Scroll-Auswahl-Bild holen
    glArchivItem_Bitmap* image = LOADER.getMapImage(23);

    // Position (relativ zum angezeigten Anfang der Karte) berechnen
    DrawPoint pos = middle_corrected * DrawPoint(getCurMapSize()) / DrawPoint(minimap.getMapSize()) + DrawPoint(2, 2);

    // Scroll-Auswahl-Bild an den Rändern verkleinern, damit es nicht über die Karte "überlappt"
    DrawPoint srcOffset(0, 0);
    Extent drawSize(image->getWidth(), image->getHeight());

    DrawPoint originOffset = pos - image->getOrigin();

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
    DrawPoint overDrawSize = pos - image->getOrigin() + DrawPoint(image->getSize()) - DrawPoint(getCurMapSize());
    if(overDrawSize.x >= 0)
        drawSize.x -= overDrawSize.x;
    // überlappung am unteren Rand?
    if(overDrawSize.y >= 0)
        drawSize.y -= overDrawSize.y;

    // Zeichnen
    image->drawPart(Rect(getMapDrawArea().getOrigin() + pos, drawSize), srcOffset);
}

bool ctrlIngameMinimap::msgLeftDown(const MouseCoords& mc)
{
    return msgMouseMove(mc);
}

bool ctrlIngameMinimap::msgMouseMove(const MouseCoords& mc)
{
    if(mc.ldown)
    {
        // Mauszeiger auf der Karte?
        if(IsPointInRect(mc.pos, getMapDrawArea()))
        {
            // Koordinate feststellen
            DrawPoint mapCoord = (mc.pos - DrawPoint(getMapDrawArea().getOrigin())) * DrawPoint(minimap.getMapSize())
                                 / DrawPoint(getCurMapSize());

            gwv.moveToMapPt(MapPoint(mapCoord));

            return true;
        }
    }

    return false;
}

void ctrlIngameMinimap::toggleTerritory()
{
    minimap.toggleTerritory();
}

void ctrlIngameMinimap::toggleHouses()
{
    minimap.toggleHouses();
}

void ctrlIngameMinimap::toggleRoads()
{
    minimap.toggleRoads();
}
