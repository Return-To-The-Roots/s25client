// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "iwMinimap.h"
#include "Loader.h"
#include "controls/ctrlImageButton.h"
#include "controls/ctrlIngameMinimap.h"
#include "gameData/const_gui_ids.h"

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
    : IngameWindow(CGI_MINIMAP, IngameWindow::posLastOrCenter, MINIMAP_SIZE, _("Outline map"), LOADER.GetImageN("resource", 41)),
      extended(false)
{
    AddCtrl(
      new ctrlIngameMinimap(this, 0, DrawPoint(contentOffset), Extent::all(WINDOW_MAP_SPACE), Extent::all(WINDOW_MAP_SPACE), minimap, gwv));

    // Land, Häuser, Straßen an/aus
    DrawPoint curPos(contentOffset.x + WINDOW_MAP_SPACE, 0);
    for(unsigned i = 0; i < 3; ++i)
    {
        AddImageButton(i + 1, curPos, BUTTON_SIZE, TC_GREY, LOADER.GetImageN("io", 85 + i));
        curPos.x += BUTTON_SIZE.x;
    }

    // Fenster vergrößern/verkleinern
    AddImageButton(4, DrawPoint(0, 0), BUTTON_SIZE, TC_GREY, LOADER.GetImageN("io", 109));

    Resize(GetSize());
}

/// Verändert die Größe des Fensters und positioniert alle Controls etc. neu
void iwMinimap::Resize(const Extent& newSize)
{
    IngameWindow::Resize(newSize);
    auto* im = GetCtrl<ctrlIngameMinimap>(0);

    im->Resize(newSize - contentOffset - contentOffsetEnd);

    // Control kürzen in der Höhe
    im->RemoveBoundingBox(Extent(BUTTON_SIZE.x * 4 + WINDOW_MAP_SPACE * 2, 0));

    // Fensterbreite anpassen
    SetIwSize(im->GetSize() + Extent(0, WINDOW_MAP_SPACE + BUTTON_MAP_SPACE + BUTTON_SIZE.y + BUTTON_WINDOW_SPACE));

    // Buttonpositionen anpassen, nach unten verschieben
    for(unsigned i = 1; i < 4; ++i)
    {
        auto* ctrl = GetCtrl<Window>(i);
        DrawPoint ctrlPos = ctrl->GetPos();
        ctrlPos.y = GetRightBottomBoundary().y - BUTTON_SIZE.y - BUTTON_WINDOW_SPACE;
        ctrl->SetPos(ctrlPos);
    }

    // Vergrößern/Verkleinern-Button nach unten rechts verschieben
    GetCtrl<Window>(4)->SetPos(GetRightBottomBoundary() - BUTTON_SIZE - DrawPoint(WINDOW_MAP_SPACE, BUTTON_WINDOW_SPACE));

    // Bild vom Vergrößern/Verkleinern-Button anpassen
    GetCtrl<ctrlImageButton>(4)->SetImage(LOADER.GetTextureN("io", extended ? 108 : 109));
}

void iwMinimap::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 1: GetCtrl<ctrlIngameMinimap>(0)->ToggleTerritory(); break;
        case 2: GetCtrl<ctrlIngameMinimap>(0)->ToggleHouses(); break;
        case 3: GetCtrl<ctrlIngameMinimap>(0)->ToggleRoads(); break;
        case 4:
        {
            // Fenster vergrößern/verkleinern
            this->extended = !extended;

            Resize(extended ? MINIMAP_SIZE_BIG : MINIMAP_SIZE);
        }
        break;
    }
}
