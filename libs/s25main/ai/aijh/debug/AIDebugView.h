// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ai/AIResource.h"
#include "ai/aijh/runtime/AIMap.h"
#include "gameTypes/MapCoordinates.h"

#include <string>

namespace AIJH {

class AIJob;

class AIDebugView
{
public:
    virtual ~AIDebugView() = default;

    virtual const std::string& GetPlayerName() const = 0;
    virtual const AIJob* GetCurrentJob() const = 0;
    virtual unsigned GetNumJobs() const = 0;
    virtual const Node& GetAINode(MapPoint pt) const = 0;
    virtual int GetResMapValue(MapPoint pt, AIResource res) const = 0;
};

} // namespace AIJH
