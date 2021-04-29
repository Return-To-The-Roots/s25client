// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "notifications/notifications.h"

struct TestNote1
{
    ENABLE_NOTIFICATION(TestNote1)

    int value = 0;
};
