// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

#pragma once

/// Statistic categories
enum StatisticType
{
    STAT_COUNTRY = 0,
    STAT_BUILDINGS,
    STAT_INHABITANTS,
    STAT_MERCHANDISE,
    STAT_MILITARY,
    STAT_GOLD,
    STAT_PRODUCTIVITY,
    STAT_VANQUISHED,
    STAT_TOURNAMENT
};

/// Anzahl Statistikarten
const unsigned NUM_STAT_TYPES = 9;

/// Anzahl Warenstatistikarten
const unsigned NUM_STAT_MERCHANDISE_TYPES = 14;

/// Update intervalls
enum StatisticTime
{
    STAT_15M = 0,
    STAT_1H,
    STAT_4H,
    STAT_16H
};

/// Number of update intervalls
const unsigned NUM_STAT_TIMES = 4;

/// Anzahl der Statistikschritte, die gespeichert werden
const unsigned NUM_STAT_STEPS = 30;

inline unsigned short incrStatIndex(unsigned short i)
{
    return (i == NUM_STAT_STEPS - 1) ? 0 : ++i;
}
inline unsigned short decrStatIndex(unsigned short i)
{
    return (i == 0) ? NUM_STAT_STEPS - 1 : --i;
}
inline unsigned short decrStatIndex(unsigned short i, unsigned short amount)
{
    return (i < amount) ? NUM_STAT_STEPS - (amount - i) - 1 : i - amount;
}
