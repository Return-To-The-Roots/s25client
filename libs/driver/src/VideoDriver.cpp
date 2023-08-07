// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "driver/VideoDriver.h"
#include "commonDefines.h"
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
    : CallBack(CallBack), initialized(false), displayMode_(DisplayMode::Resizable), renderSize_(0, 0)
{
    std::fill(keyboard.begin(), keyboard.end(), false);
}

Position VideoDriver::GetMousePos() const
{
    return mouse_xy.pos;
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
}
