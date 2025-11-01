// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "driver/VideoDriver.h"
#include "commonDefines.h"
#include "driver/VideoDriverLoaderInterface.h"
#include "helpers/mathFuncs.h"
#include <algorithm>
#include <stdexcept>

// Do not inline! That would break DLL compatibility:
// http://stackoverflow.com/questions/32444520/how-to-handle-destructors-in-dll-exported-interfaces
IVideoDriver::~IVideoDriver() = default;

/**
 *  Konstruktor von @p VideoDriver.
 *
 *  @param[in] CallBack DriverCallback für Rückmeldungen.
 */
VideoDriver::VideoDriver(VideoDriverLoaderInterface* CallBack)
    : CallBack(CallBack), initialized(false), isFullscreen_(false), renderSize_(0, 0), scaledRenderSize_(0, 0),
      dpiScale_(1.f), guiScale_(100), autoGuiScale_(false)
{
    std::fill(keyboard.begin(), keyboard.end(), false);
}

Position VideoDriver::GetMousePos() const
{
    return mouse_xy.pos * (int)dpiScale_;
}

/**
 *  Funktion zum Auslesen ob die Linke Maustaste gedrückt ist.
 *
 *  @return @p true bei Gedrückt, @p false bei Losgelassen
 */
bool VideoDriver::GetMouseStateL() const
{
    return mouse_xy.ldown;
}

/**
 *  Funktion zum Auslesen ob die Rechte Maustaste gedrückt ist.
 *
 *  @return @p true bei Gedrückt, @p false bei Losgelassen
 */
bool VideoDriver::GetMouseStateR() const
{
    return mouse_xy.rdown;
}

VideoMode VideoDriver::FindClosestVideoMode(const VideoMode& mode) const
{
    std::vector<VideoMode> avModes;
    ListVideoModes(avModes);
    if(avModes.empty())
        throw std::runtime_error("No supported video modes found!");
    unsigned minSizeDiff = std::numeric_limits<unsigned>::max();
    VideoMode best = avModes.front();
    for(const VideoMode& current : avModes)
    {
        const auto dw = absDiff(current.width, mode.width);
        const auto dh = absDiff(current.height, mode.height);
        unsigned sizeDiff = dw * dw + dh * dh;
        if(sizeDiff < minSizeDiff)
        {
            minSizeDiff = sizeDiff;
            best = current;
        }
    }
    return best;
}

void VideoDriver::SetNewSize(VideoMode windowSize, Extent renderSize)
{
    windowSize_ = windowSize;
    renderSize_ = renderSize;

    const auto ratioXY = PointF(renderSize_) / PointF(windowSize_.width, windowSize_.height);
    dpiScale_ = (ratioXY.x + ratioXY.y) / 2.f; // use the average ratio of both axes

    if(autoGuiScale_)
        guiScale_ = GuiScale(getGuiScaleRange().recommendedPercent);

    scaledRenderSize_ = guiScale_.screenToView<Extent>(renderSize);
}

void VideoDriver::setGuiScalePercent(unsigned percent)
{
    autoGuiScale_ = (percent == 0);
    if(autoGuiScale_)
        percent = getGuiScaleRange().recommendedPercent;

    if(guiScale_.percent() == percent)
        return;

    // translate current mouse position to screen space
    const auto screenPos = guiScale_.viewToScreen(mouse_xy.pos);

    guiScale_ = GuiScale(percent);
    scaledRenderSize_ = guiScale_.screenToView<Extent>(renderSize_);
    CallBack->WindowResized();

    // move cursor to new position in view space
    // must happen after window resize event to avoid drawing spurious hover events
    mouse_xy.pos = guiScale_.screenToView(screenPos);
    CallBack->Msg_MouseMove(mouse_xy);
}

GuiScaleRange VideoDriver::getGuiScaleRange() const
{
    constexpr auto min = 100u;
    const auto recommended = std::max(min, helpers::iround<unsigned>(dpiScale_ * 100.f));
    const auto maxScaleXY = renderSize_ / PointF(800.f, 600.f);
    const auto maxScale = std::min(maxScaleXY.x, maxScaleXY.y);
    // if the window shrinks below its minimum size of 800x600, max can be smaller than recommended
    const auto max = std::max(helpers::iround<unsigned>(maxScale * 100.f), recommended);

    return GuiScaleRange{min, max, recommended};
}
