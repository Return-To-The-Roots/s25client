// Copyright (c) 2018 - 2018 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef MockClock_h__
#define MockClock_h__

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
}}     // namespace rttr::test
#endif // MockClock_h__
