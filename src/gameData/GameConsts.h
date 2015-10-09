// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef GAMECONSTS_H_
#define GAMECONSTS_H_

#include "mygettext.h"

#include "gameData/NationConsts.h"
#include "gameTypes/JobTypes.h"
#include "gameTypes/BuildingTypes.h"
#include "gameTypes/GoodTypes.h"
#include "gameData/PlayerConsts.h"
#include "gameData/AnimalConsts.h"

#include <boost/array.hpp>

struct RoadWindowInfo
{
    bool flag;
    int mx, my;
};

/// Verfügbare Statistikarten
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
const unsigned STAT_TYPE_COUNT = 9;

/// Anzahl Warenstatistikarten
const unsigned STAT_MERCHANDISE_TYPE_COUNT = 14;

/// Statistikzeiträume
enum StatisticTime
{
    STAT_15M = 0,
    STAT_1H,
    STAT_4H,
    STAT_16H
};

/// Anzahl Statistikzeiträume
const unsigned STAT_TIME_COUNT = 4;

/// Anzahl der Statistikschritte, die gespeichert werden
const unsigned STAT_STEP_COUNT = 30;

/// Konvertierungstabelle von RttR-Nation-Indizes in Original-S2-Nation-Indizes
const boost::array<unsigned char, 4> NATION_RTTR_TO_S2 =
{{
    3,
    2,
    0,
    1
}};
/// Konvertierungstabelle von Rohstoff-Indizes von den Bergwerken --> Map
const boost::array<unsigned char, 5> RESOURCES_MINE_TO_MAP = {{3, 0, 1, 2, 4}};

/// Geschwindigkeitsabstufungen - Längen der GFs in ms
const boost::array<unsigned, 6> SPEED_GF_LENGTHS = {{80, 60, 50, 40, 30, 1}};

/// Macht ggf. aus den verschiedenen Schilden der Nationen jeweils immer das römische normale Schild für
/// die Warensysteme usw
inline GoodType ConvertShields(const GoodType& good)
{
    return (good == GD_SHIELDVIKINGS ||
            good == GD_SHIELDAFRICANS ||
            good == GD_SHIELDJAPANESE) ? GD_SHIELDROMANS : good;
}

/// Umgekehrte Konvertierung: Gibt den Schildtyp für jede Nation an
const boost::array<GoodType, NAT_COUNT> SHIELD_TYPES =
{{
    GD_SHIELDAFRICANS,
    GD_SHIELDJAPANESE,
    GD_SHIELDROMANS,
    GD_SHIELDVIKINGS,
    GD_SHIELDJAPANESE
}};

/// Reichweite der Bergarbeiter
const unsigned MINER_RADIUS = 2;

#include "gameTypes/PactTypes.h"

/// Post-Nachrichten-Kategorien
enum PostMessageCategory
{
    PMC_MILITARY, // ImagePostMsgWithLocation
    PMC_GEOLOGIST, // PostMsgWithLocation
    PMC_GENERAL, // ImagePostMsgWithLocation
    PMC_SAVEWARNING, // PostMsg
    PMC_DIPLOMACY, // DiplomacyPostQuestion (man braucht vll noch verschiedene?)
    PMC_OTHER  // PostMsg
};

/// Post-Nachrichten-Typen (entspricht den Klassen in PostMsg.h)
enum PostMessageType
{
    PMT_NORMAL,               // PostMsg
    PMT_WITH_LOCATION,        // PostMsgWithLocation
    PMT_IMAGE_WITH_LOCATION,  // ImagePostMsgWithLocation
    PMT_DIPLOMACYQUESTION,             // DiplomacyPostQuestion
    PMT_DIPLOMACYINFO,            // DiplomacyPostInfo
    PMT_SHIP
};

/// Maximale Nachrichtenanzahl im Briefkasten
const unsigned MAX_POST_MESSAGES = 20;

/// Konstante für die Pfadrichtung bei einer Schiffsverbindung
const unsigned char SHIP_DIR = 100;

/// Anzahl der Späher bei einer Erkundungs-Expedition
const unsigned SCOUTS_EXPLORATION_EXPEDITION = 3;

/// Number of "classical" objectives in a friendly match
const unsigned OBJECTIVES_COUNT = 3;
/// tournament modes
const unsigned TOURNAMENT_MODES_COUNT = 5;
const boost::array<unsigned, TOURNAMENT_MODES_COUNT> TOURNAMENT_MODES_DURATION =
{{
    30, 60, 90, 120, 240
}};

#endif
