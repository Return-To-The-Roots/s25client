// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "TypeId.h"

/** The notification system dispatches notifications based on their type.
 * To enable a type to be dispatched and listened on, you need to call the
 * macro 'ENABLE_NOTIFICATION' with the type name in the public part of that type */
#define ENABLE_NOTIFICATION(Type) \
    static uint32_t getNoteId() { return TypeId::value<Type>(); }
