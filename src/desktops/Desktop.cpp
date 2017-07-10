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

#include "defines.h" // IWYU pragma: keep
#include "Desktop.h"

#include "WindowManager.h"
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
    Resize(VIDEODRIVER.GetScreenWidth(), VIDEODRIVER.GetScreenWidth());
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
 *  Wechselt den aktuellen Desktop im WindowManager auf diesen Desktop.
 */
void Desktop::Show()
{
    WINDOWMANAGER.Switch(this);
}

/**
 *  Reagiert auf Spielfenstergrößenänderung
 */
void Desktop::Msg_ScreenResize(const ScreenResizeEvent& sr)
{
// Keep the following block the same as in ctrlGroup class:
    // Für skalierte Desktops ist alles einfach, die brauchen im besten Fall gar nichts selbst implementieren
    if (scale_)
    {
        //Zunächst an die Kinder weiterleiten
        for(std::map<unsigned int, Window*>::iterator it = childIdToWnd_.begin(); it != childIdToWnd_.end(); ++it)
        {
            if(!it->second)
                continue;
            Window* ctrl = it->second;
            // unskalierte Position und Größe bekommen
            int realX = ctrl->GetX() * 800 / sr.oldWidth;
            int realY = ctrl->GetY() * 600 / sr.oldHeight;
            unsigned realWidth = ctrl->GetWidth() * 800 / sr.oldWidth;
            unsigned realHeight = ctrl->GetHeight() * 600 / sr.oldHeight;
            // Rundungsfehler?
            if(realX * sr.oldWidth / 800 < ctrl->GetX()) ++realX;
            if(realY * sr.oldHeight / 600 < ctrl->GetY()) ++realY;
            if(realWidth  * sr.oldWidth / 800 < ctrl->GetWidth())  ++realWidth;
            if(realHeight * sr.oldHeight / 600 < ctrl->GetHeight()) ++realHeight;
            // Und los
            ctrl->Move(realX * sr.newWidth / 800, realY * sr.newHeight / 600);
            ctrl->Msg_ScreenResize(sr);
            ctrl->Resize(realWidth * sr.newWidth / 800, realHeight * sr.newHeight / 600);
        }
    }

    // Individuelle Reaktion ist auch erlaubt
    Resize(sr.newWidth, sr.newHeight);
}

