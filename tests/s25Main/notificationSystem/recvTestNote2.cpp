// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "TestNote2.h"
#include "notifications/NotificationManager.h"
#include "testNoteFunctions.h"

namespace {
int& lastValue()
{
    static int value = 0;
    return value;
}
} // namespace

void subscribeTestNote2(NotificationManager& mgr)
{
    static Subscription sub;
    sub = mgr.subscribe<TestNote2>([](TestNote2 note) noexcept { lastValue() = note.value; });
}

int getLastTestNote2()
{
    return lastValue();
}

uint32_t getTestNote2IdRecv()
{
    return TestNote2::getNoteId();
}
