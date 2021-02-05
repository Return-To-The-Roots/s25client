// Copyright (c) 2021 - 2021 Settlers Freaks (sf-team at siedler25.org)
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

#include "TestNote1.h"
#include "notifications/NotificationManager.h"
#include "testNoteFunctions.h"

namespace {
int& lastValue()
{
    static int value = 0;
    return value;
}
} // namespace

void subscribeTestNote1(NotificationManager& mgr)
{
    static Subscription sub;
    sub = mgr.subscribe<TestNote1>([](TestNote1 note) { lastValue() = note.value; });
}

int getLastTestNote1()
{
    return lastValue();
}

uint32_t getTestNote1IdRecv()
{
    return TestNote1::getNoteId();
}
