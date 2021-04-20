// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#define RTTR_DLLEXPORT
#include "driver/DriverInterfaceVersion.h"
#include "driver/Interface.h"

/**
 *  API-Versions-Lieferfunktion
 *
 *  @return liefert die API-Version des Treibers
 */
unsigned GetDriverAPIVersion()
{
    return DRIVERAPIVERSION;
}
