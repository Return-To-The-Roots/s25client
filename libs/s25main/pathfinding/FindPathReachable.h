// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/MapCoordinates.h"

class GameWorldBase;
bool DoesReachablePathExist(const GameWorldBase& world, MapPoint startPt, MapPoint endPt, unsigned maxLen);
