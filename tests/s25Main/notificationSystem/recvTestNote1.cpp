// Copyright (c) 2021 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
