// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ai/AIResource.h"
#include "world/NodeMapBase.h"
#include "gameTypes/BuildingQuality.h"

namespace AIJH {
struct Node
{
    BuildingQuality bq;
    AINodeResource res; // TODO: Fixup to keep in sync
    bool owned;
    bool reachable;
    char failed_penalty; // when a node was marked reachable, but building failed, this field is >0
    bool border;
    bool farmed;
};

/// Map of AINodes
using AIMap = NodeMapBase<Node>;

} // namespace AIJH
