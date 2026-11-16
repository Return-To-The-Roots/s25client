// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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
#include <RescaleWindowProp.h>
#include <limits>

// Set to highest possible so it is drawn last
const unsigned Desktop::fpsDisplayId = std::numeric_limits<unsigned>::max();
constexpr DrawPoint fpsDisplayPos(800, 0);

Desktop::Desktop(glArchivItem_Bitmap* background)
    : Window(nullptr, 0, DrawPoint::all(0), VIDEODRIVER.getRenderSize()), background(background), lastFPS_(0)
{
    setScale(true);
    setFpsDisplay(true);
    // By default limit the maximum frame rate to 60 FPS - used for main menu
    if(SETTINGS.video.framerate < 0)
        VIDEODRIVER.setTargetFramerate(60);
    else
        VIDEODRIVER.setTargetFramerate(SETTINGS.video.framerate);
    UpdateFps(VIDEODRIVER.getFPS());
}

Desktop::~Desktop() = default;

void Desktop::Draw_()
{
    unsigned curFPS = VIDEODRIVER.getFPS();
    if(curFPS != lastFPS_)
        UpdateFps(curFPS);

    if(background)
        background->DrawFull(getDrawRect());

    Window::Draw_();
}

void Desktop::msgScreenResize(const ScreenResizeEvent& sr)
{
    Window::msgScreenResize(sr);
    // Resize to new screen size
    Resize(sr.newSize);
    auto* fpsDisplay = getCtrl<ctrlText>(fpsDisplayId);
    if(fpsDisplay)
        fpsDisplay->setPos(Scale(fpsDisplayPos));
}

void Desktop::setFpsDisplay(bool show)
{
    if(!show)
        DeleteCtrl(fpsDisplayId);
    else if(!getCtrl<ctrlText>(fpsDisplayId) && SmallFont)
    {
        AddText(fpsDisplayId, fpsDisplayPos, helpers::toString(lastFPS_) + " fps", COLOR_YELLOW, FontStyle::RIGHT,
                SmallFont);
    }
}

void Desktop::UpdateFps(unsigned newFps)
{
    auto* fpsDisplay = getCtrl<ctrlText>(fpsDisplayId);
    if(fpsDisplay)
        fpsDisplay->setText(helpers::toString(newFps) + " fps");
    lastFPS_ = newFps;
}
