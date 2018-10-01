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

#include "rttrDefines.h" // IWYU pragma: keep
#include "Desktop.h"
#include "Loader.h"
#include "controls/ctrlText.h"
#include "drivers/ScreenResizeEvent.h"
#include "drivers/VideoDriverWrapper.h"
#include "helpers/converters.h"
#include "ogl/FontStyle.h"
#include "ogl/glArchivItem_Bitmap.h"
#include <limits>

// Set to highest possible so it is drawn last
const unsigned Desktop::fpsDisplayId = std::numeric_limits<unsigned>::max();

/**
 *  Konstruktor für einen Spieldesktop
 *
 *  @param[in] background Hintergrund des Desktops
 */
Desktop::Desktop(glArchivItem_Bitmap* background) : Window(NULL, 0, DrawPoint::all(0), VIDEODRIVER.GetScreenSize()), background(background)
{
    SetScale(true);
    SetFpsDisplay(true);
}

Desktop::~Desktop() {}

/**
 *  Zeichenmethode zum Zeichnen des Desktops
 *  und der ggf. enthaltenen Steuerelemente.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 */
void Desktop::Draw_()
{
    if(background)
        background->DrawFull(GetDrawRect());

    DrawControls();
}

/**
 *  Reagiert auf Spielfenstergrößenänderung
 */
void Desktop::Msg_ScreenResize(const ScreenResizeEvent& sr)
{
    Window::Msg_ScreenResize(sr);
    // Resize to new screen size
    Resize(sr.newSize);
}

void Desktop::SetFpsDisplay(bool show)
{
    if(!show)
        DeleteCtrl(fpsDisplayId);
    else if(!GetCtrl<ctrlText>(fpsDisplayId) && SmallFont)
        AddText(fpsDisplayId, DrawPoint(800, 0), "", COLOR_YELLOW, FontStyle::RIGHT, SmallFont);
}

void Desktop::UpdateFps(unsigned newFps)
{
    ctrlText* fpsDisplay = GetCtrl<ctrlText>(fpsDisplayId);
    if(fpsDisplay)
        fpsDisplay->SetText(helpers::toString(newFps) + " fps");
}
