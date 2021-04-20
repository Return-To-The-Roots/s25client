// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "FramesInfo.h"

FramesInfo::FramesInfo()
{
    Clear();
}

void FramesInfo::Clear()
{
    // Default GF len of 20
    gf_length = milliseconds32_t(20);
    gfLengthReq = gf_length;
    nwf_length = 0;
    frameTime = milliseconds32_t::zero();
    lastTime = UsedClock::time_point();
    isPaused = false;
}

FramesInfoClient::FramesInfoClient()
{
    Clear();
}

void FramesInfoClient::Clear()
{
    FramesInfo::Clear();
    forcePauseStart = UsedClock::time_point();
    forcePauseLen = milliseconds32_t::zero();
}
