// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/JobTypes.h"
#include "gameTypes/MapCoordinates.h"
#include <memory>

class noFigure;
class noRoadNode;

/// static factory class to create new job figures
class JobFactory
{
public:
    JobFactory() = delete;

    // Erstellt Job anhand der job-id
    static std::unique_ptr<noFigure> CreateJob(Job job_id, MapPoint pt, unsigned char player, noRoadNode* goal);
};
