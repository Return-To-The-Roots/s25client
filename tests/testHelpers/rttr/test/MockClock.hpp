// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Clock.h"

namespace rttr { namespace test {
    struct MockClock : public BaseClock
    {
        duration& currentTime;
        MockClock(duration& timeRef) : currentTime(timeRef) {}
        duration time_since_epoch() override { return currentTime; }
    };

    struct MockClockFixture
    {
        MockClock::duration currentTime;

        MockClockFixture() : currentTime(0) { Clock::setClock(std::make_unique<MockClock>(currentTime)); }
        ~MockClockFixture() { Clock::setClock(std::make_unique<BaseClock>()); }
    };
}} // namespace rttr::test
