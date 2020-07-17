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
    : CallBack(CallBack), initialized(false), isFullscreen_(false), renderSize_(0, 0)
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
        const auto dw = safeDiff(current.width, mode.width);
        const auto dh = safeDiff(current.height, mode.height);
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
