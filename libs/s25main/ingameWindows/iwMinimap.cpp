// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwMinimap.h"
#include "Loader.h"
#include "Settings.h"
#include "controls/ctrlImageButton.h"
#include "controls/ctrlIngameMinimap.h"
#include "helpers/containerUtils.h"
#include "gameData/const_gui_ids.h"
#include "s25util/error.h"

/// (maximum) size of the minimap
const Extent MINIMAP_SIZE = Extent::all(170);
/// (maximum) size of the zoomed minimap
const Extent MINIMAP_SIZE_BIG = Extent::all(370);

/// Abstand der Kartenränder zum Fensterrand
const unsigned short WINDOW_MAP_SPACE = 8;
/// Size of the lower buttons
const Extent BUTTON_SIZE(36, 36);
/// Abstand zwischen Buttons und Karte (Y)
const unsigned short BUTTON_MAP_SPACE = 3;
/// Abstand zwischen Buttons und unteren Fensterrand
const unsigned short BUTTON_WINDOW_SPACE = 5;

iwMinimap::iwMinimap(IngameMinimap& minimap, GameWorldView& gwv)
    : IngameWindow(CGI_MINIMAP, IngameWindow::posLastOrCenter, MINIMAP_SIZE, _("Outline map"),
                   LOADER.getImageN("resource", 41)),
      extended(SETTINGS.ingame.minimapExtended)
{
    AddCtrl(std::make_unique<ctrlIngameMinimap>(this, 0, DrawPoint(contentOffset), Extent::all(WINDOW_MAP_SPACE),
                                                Extent::all(WINDOW_MAP_SPACE), minimap, gwv));

    // Land, Häuser, Straßen an/aus
    DrawPoint curPos(contentOffset.x + WINDOW_MAP_SPACE, 0);
    for(unsigned i = 0; i < 3; ++i)
    {
        AddImageButton(i + 1, curPos, BUTTON_SIZE, TextureColor::Grey, LOADER.getImageN("io", 85 + i));
        curPos.x += BUTTON_SIZE.x;
    }

    // Fenster vergrößern/verkleinern
    AddImageButton(4, DrawPoint(0, 0), BUTTON_SIZE, TextureColor::Grey, LOADER.getImageN("io", 109));

    Resize(extended ? MINIMAP_SIZE_BIG : MINIMAP_SIZE);
}

/// Verändert die Größe des Fensters und positioniert alle Controls etc. neu
void iwMinimap::Resize(const Extent& newSize)
{
    IngameWindow::Resize(newSize);
    auto* im = getCtrl<ctrlIngameMinimap>(0);

    im->Resize(newSize - contentOffset - contentOffsetEnd);

    // Control kürzen in der Höhe
    im->RemoveBoundingBox(Extent(BUTTON_SIZE.x * 4 + WINDOW_MAP_SPACE * 2, 0));

    // Fensterbreite anpassen
    setIwSize(im->getSize() + Extent(0, WINDOW_MAP_SPACE + BUTTON_MAP_SPACE + BUTTON_SIZE.y + BUTTON_WINDOW_SPACE));

    // Buttonpositionen anpassen, nach unten verschieben
    for(unsigned i = 1; i < 4; ++i)
    {
        auto* ctrl = getCtrl<Window>(i);
        DrawPoint ctrlPos = ctrl->getPos();
        ctrlPos.y = getRightBottomBoundary().y - BUTTON_SIZE.y - BUTTON_WINDOW_SPACE;
        ctrl->setPos(ctrlPos);
    }

    // Vergrößern/Verkleinern-Button nach unten rechts verschieben
    getCtrl<Window>(4)->setPos(getRightBottomBoundary() - BUTTON_SIZE
                               - DrawPoint(WINDOW_MAP_SPACE, BUTTON_WINDOW_SPACE));

    // Bild vom Vergrößern/Verkleinern-Button anpassen
    getCtrl<ctrlImageButton>(4)->setImage(LOADER.getTextureN("io", extended ? 108 : 109));
}

void iwMinimap::msgButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 1: getCtrl<ctrlIngameMinimap>(0)->ToggleTerritory(); break;
        case 2: getCtrl<ctrlIngameMinimap>(0)->ToggleHouses(); break;
        case 3: getCtrl<ctrlIngameMinimap>(0)->ToggleRoads(); break;
        case 4:
        {
            // Fenster vergrößern/verkleinern
            this->extended = !extended;
            SETTINGS.ingame.minimapExtended = extended;

            Resize(extended ? MINIMAP_SIZE_BIG : MINIMAP_SIZE);
        }
        break;
    }
}
