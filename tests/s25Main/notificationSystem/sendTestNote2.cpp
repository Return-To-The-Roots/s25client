// Copyright (C) 2021 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "TestNote2.h"
#include "notifications/NotificationManager.h"
#include "testNoteFunctions.h"

void sendTestNote2(NotificationManager& mgr, int value)
{
    mgr.publish(TestNote2{value});
}

uint32_t getTestNote2IdSend()
{
    return TestNote2::getNoteId();
}
