// $Id: iwMinimap.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "iwMinimap.h"
#include "ctrlIngameMinimap.h"
#include "Loader.h"
#include "ctrlButton.h"


/// (maximale) Größe des Minimapfensters normal
const unsigned short MINIMAP_WINDOW_WIDTH = 200;
const unsigned short MINIMAP_WINDOW_HEIGHT = 200;
/// (maximale) Größe des Minimapfensters groß
const unsigned short MINIMAP_WINDOW_BIG_WIDTH = 400;
const unsigned short MINIMAP_WINDOW_BIG_HEIGHT = 400;

/// Abstand der Kartenränder zum Fensterrand
const unsigned short WINDOW_MAP_SPACE = 5;
/// Minimale Fensterbreite
const unsigned short MIN_WINDOW_WIDTH = 200;
/// Breite der unteren Buttons
const unsigned short BUTTON_WIDTH = 36;
/// Höhe der unteren Buttons
const unsigned short BUTTON_HEIGHT = 36;
/// Abstand zwischen Buttons und Karte (Y)
const unsigned short BUTTON_MAP_SPACE = 3;
/// Abstand zwischen Buttons und unteren Fensterrand
const unsigned short BUTTON_WINDOW_SPACE = 5;



iwMinimap::iwMinimap(IngameMinimap* minimap, GameWorldViewer& gwv)
    : IngameWindow(CGI_MINIMAP, 0xFFFF, 0xFFF, MINIMAP_WINDOW_WIDTH,
                   MINIMAP_WINDOW_HEIGHT, _("Outline map"), LOADER.GetImageN("resource", 41)), extended(false)
{


    AddCtrl(0, new ctrlIngameMinimap(this, 0, 10, 20, WINDOW_MAP_SPACE, WINDOW_MAP_SPACE, WINDOW_MAP_SPACE, WINDOW_MAP_SPACE, minimap, gwv));


    // Land, Häuser, Straßen an/aus
    for(unsigned i = 0; i < 3; ++i)
        AddImageButton(i + 1, 10 + WINDOW_MAP_SPACE + BUTTON_WIDTH * i, 0, BUTTON_WIDTH, BUTTON_HEIGHT, TC_GREY, LOADER.GetImageN("io", 85 + i));

    // Fenster vergrößern/verkleinern
    AddImageButton(4, 0, 0, BUTTON_WIDTH, BUTTON_HEIGHT, TC_GREY, LOADER.GetImageN("io", 109));


    ChangeWindowSize(width, height);
}

/// Verändert die Größe des Fensters und positioniert alle Controls etc. neu
void iwMinimap::ChangeWindowSize(const unsigned short width, const unsigned short height)
{
    ctrlIngameMinimap* im = GetCtrl<ctrlIngameMinimap>(0);

    im->SetDisplaySize(width - 20, height - 30);

    // Control kürzen in der Höhe
    im->RemoveBoundingBox(BUTTON_WIDTH * 4 + WINDOW_MAP_SPACE * 2, 0);

    ////// Und nach ganz oben verschieben
    //im->Move(im->GetX(false),20);

    // Fensterbreite anpassen
    SetWidth(im->GetWidth() + 20);
    SetIwHeight(im->GetHeight() + 30 + BUTTON_HEIGHT + BUTTON_MAP_SPACE + BUTTON_WINDOW_SPACE);


    // Buttonpositionen anpassen, nach unten verschieben
    for(unsigned i = 1; i < 4; ++i)
        GetCtrl<ctrlImageButton>(i)->Move(GetCtrl<ctrlImageButton>(i)->GetX(false), GetHeight() - 10 - BUTTON_HEIGHT - BUTTON_WINDOW_SPACE);

    // Vergrößern/Verkleinern-Button nach unten rechts verschieben
    GetCtrl<ctrlImageButton>(4)->Move(GetWidth() - 10 - BUTTON_WIDTH - WINDOW_MAP_SPACE, GetHeight() - 10 - BUTTON_HEIGHT - BUTTON_WINDOW_SPACE);

    // Bild vom Vergrößern/Verkleinern-Button anpassen
    GetCtrl<ctrlImageButton>(4)->SetImage(LOADER.GetImageN("io", extended ? 108 : 109));
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

            ChangeWindowSize(extended ? MINIMAP_WINDOW_BIG_WIDTH : MINIMAP_WINDOW_WIDTH,
                             extended ? MINIMAP_WINDOW_BIG_HEIGHT : MINIMAP_WINDOW_HEIGHT);
        } break;
    }
}

