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
#include "FramesInfo.h"
#include "helpers/mathFuncs.h"

FramesInfo::FramesInfo()
{
    Clear();
}

void FramesInfo::Clear()
{
    // Default GF len of 20
    gf_length = milliseconds32_t(20);
    gfLenghtNew = milliseconds32_t::zero();
    gfLenghtNew2 = milliseconds32_t::zero();
    nwf_length = 0;
    frameTime = milliseconds32_t::zero();
    lastTime = UsedClock::time_point();
    isPaused = false;
}

void FramesInfo::ApplyNewGFLength()
{
    // Current length of a NWF in ms
    milliseconds32_t nwfLenInMs = gf_length * nwf_length;
    gf_length = gfLenghtNew;
    // Time for one NWF should stay the same
    nwf_length = helpers::roundedDiv(nwfLenInMs.count(), gf_length / milliseconds32_t(1));
}

FramesInfoClient::FramesInfoClient()
{
    Clear();
}

void FramesInfoClient::Clear()
{
    FramesInfo::Clear();
    gfLengthReq = milliseconds32_t::zero();
    gfNrServer = 0;
    forcePauseStart = UsedClock::time_point();
    forcePauseLen = milliseconds32_t::zero();
}
