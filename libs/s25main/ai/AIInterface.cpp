// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AIInterface.h"

AIInterface::AIInterface(const GameWorldBase& gwb, std::vector<gc::GameCommandPtr>& gcs, unsigned char playerID)
    : AICommandSink(gcs, gwb.GetPlayer(playerID), playerID), gwb(gwb), queryService_(gwb, playerID)
{}

AIInterface::~AIInterface() = default;
