// Copyright (c) 2021 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "TestNote1.h"
#include "notifications/NotificationManager.h"
#include "testNoteFunctions.h"

void sendTestNote1(NotificationManager& mgr, int value)
{
    mgr.publish(TestNote1{value});
}

uint32_t getTestNote1IdSend()
{
    return TestNote1::getNoteId();
}
