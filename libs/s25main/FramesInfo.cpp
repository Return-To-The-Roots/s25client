// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "FramesInfo.h"

FramesInfo::FramesInfo()
{
    Clear();
}

void FramesInfo::Clear()
{
    using namespace std::chrono_literals;
    // Default GF len
    gf_length = 20ms;
    gfLengthReq = gf_length;
    nwf_length = 0;
    frameTime = frameTime.zero();
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
    forcePauseLen = forcePauseLen.zero();
}
