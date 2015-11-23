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

#include "defines.h"
#include "FramesInfo.h"

FramesInfo::FramesInfo()
{
    Clear();
}

void FramesInfo::Clear()
{
    gf_nr = 0;
    gf_length = 0;
    gf_length_new = 0;
    nwf_length = 0;
    frameTime = 0;
    lastTime = 0;
    isPaused = false;
}

void FramesInfo::ApplyNewGFLength()
{
    gf_length = gf_length_new;
    if(gf_length == 1)
        nwf_length = 50;
    else
        nwf_length = 250 / gf_length;
}

FramesInfoClient::FramesInfoClient()
{
    Clear();
}

void FramesInfoClient::Clear()
{
    FramesInfo::Clear();
    gfNrServer = 0;
    pause_gf = 0;
}