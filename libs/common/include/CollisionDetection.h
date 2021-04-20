// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Point.h"
#include "Rect.h"

bool IsPointInRect(const Position& pt, const Rect& rect);
bool IsPointInRect(int x, int y, const Rect& rect);
bool IsPointInRect(int x, int y, int rx, int ry, int rwidth, int rheight);
bool DoRectsIntersect(const Rect& rect1, const Rect& rect2);
