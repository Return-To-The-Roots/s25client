// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
static const std::map<StatisticType, std::string> statistic_names = {
    {StatisticType::Country, "Country"},
    {StatisticType::Buildings, "Buildings"},
    {StatisticType::Inhabitants, "Inhabitants"},
    {StatisticType::Merchandise, "Merchandise"},
    {StatisticType::Military, "Military"},
    {StatisticType::Gold, "GoldCoins"},
    {StatisticType::Productivity, "Productivity"},
    {StatisticType::Vanquished, "Vanquished"},
    {StatisticType::Tournament, "Tournament"}
};

inline std::string StatisticTypeName(StatisticType type) {
    return statistic_names.at(type);
}