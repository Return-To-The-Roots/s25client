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

#include "defines.h" // IWYU pragma: keep
#include "Desktop.h"

#include "drivers/VideoDriverWrapper.h"
#include "drivers/ScreenResizeEvent.h"
#include "ogl/glArchivItem_Bitmap.h"

/**
 *  Konstruktor für einen Spieldesktop
 *
 *  @param[in] background Hintergrund des Desktops
 */
Desktop::Desktop(glArchivItem_Bitmap* background)
    : Window(), background(background)
{
    SetScale(true);
    Resize(VIDEODRIVER.GetScreenSize());
}

/**
 *  Zeichenmethode zum Zeichnen des Desktops
 *  und der ggf. enthaltenen Steuerelemente.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 */
void Desktop::Draw_()
{
    if(background)
        background->Draw(DrawPoint(0, 0), VIDEODRIVER.GetScreenWidth(), VIDEODRIVER.GetScreenHeight());

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

