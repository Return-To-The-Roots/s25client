// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Desktop.h"
#include "Loader.h"
#include "Settings.h"
#include "controls/ctrlText.h"
#include "drivers/ScreenResizeEvent.h"
#include "drivers/VideoDriverWrapper.h"
#include "helpers/toString.h"
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
Desktop::Desktop(glArchivItem_Bitmap* background)
    : Window(nullptr, 0, DrawPoint::all(0), VIDEODRIVER.GetRenderSize()), background(background), lastFPS_(0)
{
    SetScale(true);
    SetFpsDisplay(true);
    // By default limit the maximum frame rate to 60 FPS
    if(SETTINGS.video.vsync < 0)
        VIDEODRIVER.setTargetFramerate(60);
    else
        VIDEODRIVER.setTargetFramerate(SETTINGS.video.vsync);
    UpdateFps(VIDEODRIVER.GetFPS());
}

Desktop::~Desktop() = default;

/**
 *  Zeichenmethode zum Zeichnen des Desktops
 *  und der ggf. enthaltenen Steuerelemente.
 *
 *  @return @p true bei Erfolg, @p false bei Fehler
 */
void Desktop::Draw_()
{
    unsigned curFPS = VIDEODRIVER.GetFPS();
    if(curFPS != lastFPS_)
        UpdateFps(curFPS);

    if(background)
        background->DrawFull(GetDrawRect());

    Window::Draw_();
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
    {
        AddText(fpsDisplayId, DrawPoint(800, 0), helpers::toString(lastFPS_) + " fps", COLOR_YELLOW, FontStyle::RIGHT,
                SmallFont);
    }
}

void Desktop::UpdateFps(unsigned newFps)
{
    auto* fpsDisplay = GetCtrl<ctrlText>(fpsDisplayId);
    if(fpsDisplay)
        fpsDisplay->SetText(helpers::toString(newFps) + " fps");
    lastFPS_ = newFps;
}
