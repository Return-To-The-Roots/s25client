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
enum class StatisticType
{
    Country,
    Buildings,
    Inhabitants,
    Merchandise,
    Military,
    Gold,
    Productivity,
    Vanquished,
    Tournament
};
static constexpr auto maxEnumValue(StatisticType)
{
    return StatisticType::Tournament;
}

/// Anzahl Warenstatistikarten
const unsigned NUM_STAT_MERCHANDISE_TYPES = 14;

/// Update intervalls in multiples of 4
enum class StatisticTime
{
    T15Minutes,
    T1Hour,
    T4Hours,
    T16Hours
};
static constexpr auto maxEnumValue(StatisticTime)
{
    return StatisticTime::T16Hours;
}

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
