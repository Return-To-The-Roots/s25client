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

#include "commonDefines.h" // IWYU pragma: keep
#include "driver/VideoDriver.h"
#include <algorithm>
#include <stdexcept>

// Do not inline! That would break DLL compatibility:
// http://stackoverflow.com/questions/32444520/how-to-handle-destructors-in-dll-exported-interfaces
IVideoDriver::~IVideoDriver() {}

/**
 *  Konstruktor von @p VideoDriver.
 *
 *  @param[in] CallBack DriverCallback für Rückmeldungen.
 */
VideoDriver::VideoDriver(VideoDriverLoaderInterface* CallBack) : CallBack(CallBack), initialized(false), isFullscreen_(false)
{
    std::fill(keyboard.begin(), keyboard.end(), false);
}

/**
 *  Funktion zum Auslesen der Mauskoordinaten.
 *
 *  @param[out] x X-Koordinate
 *  @param[out] y Y-Koordinate
 */
void VideoDriver::GetMousePos(int& x, int& y) const
{
    x = mouse_xy.pos.x;
    y = mouse_xy.pos.y;
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
        unsigned sizeDiff = safeDiff(current.width, mode.width) * safeDiff(current.height, mode.height);
        if(sizeDiff < minSizeDiff)
        {
            minSizeDiff = sizeDiff;
            best = current;
        }
    }
    return best;
}
