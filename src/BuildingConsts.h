// $Id: BuildingConsts.h 9504 2014-11-29 10:47:38Z marcus $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef BUILD_COSTS_H_
#define BUILD_COSTS_H_

#include "GameConsts.h"

struct BuildingCost
{
    unsigned char boards;
    unsigned char stones;
};

// Konstanten für die Baukosten der Gebäude von allen 4 Völkern
const BuildingCost BUILDING_COSTS[NATION_COUNT][40] =
{
    // Nubier
    {
        {0, 0}, {2, 0}, {2, 3}, {0, 0}, {3, 5}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {4, 7},
        {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {0, 0}, {4, 2}, {2, 0}, {2, 0}, {2, 0},
        {2, 0}, {2, 2}, {2, 0}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {4, 3}, {3, 3}, {4, 3},
        {0, 0}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 0}, {2, 3}, {3, 3}, {3, 3}, {4, 6}
    },
    // Japaner
    {
        {0, 0}, {2, 0}, {2, 3}, {0, 0}, {3, 5}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {4, 7},
        {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {0, 0}, {4, 2}, {2, 0}, {2, 0}, {2, 0},
        {2, 0}, {2, 2}, {2, 0}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {4, 3}, {3, 3}, {4, 3},
        {0, 0}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 0}, {2, 3}, {3, 3}, {3, 3}, {4, 6}
    },
    // Römer
    {
        {0, 0}, {2, 0}, {2, 3}, {0, 0}, {3, 5}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {4, 7},
        {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {0, 0}, {4, 2}, {2, 0}, {2, 0}, {2, 0},
        {2, 0}, {2, 2}, {2, 0}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {4, 3}, {3, 3}, {4, 3},
        {0, 0}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 0}, {2, 3}, {3, 3}, {3, 3}, {4, 6}
    },
    // Wikinger
    {
        {0, 0}, {2, 0}, {2, 3}, {0, 0}, {3, 5}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {4, 7},
        {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {0, 0}, {4, 2}, {2, 0}, {2, 0}, {2, 0},
        {2, 0}, {2, 2}, {2, 0}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {4, 3}, {3, 3}, {4, 3},
        {0, 0}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 0}, {2, 3}, {3, 3}, {3, 3}, {4, 6}
    },
    // Babylonier
    {
        {0, 0}, {2, 0}, {2, 3}, {0, 0}, {3, 5}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {4, 7},
        {4, 0}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {0, 0}, {4, 2}, {2, 0}, {2, 0}, {2, 0},
        {2, 0}, {2, 2}, {2, 0}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {4, 3}, {3, 3}, {4, 3},
        {0, 0}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 0}, {2, 3}, {3, 3}, {3, 3}, {4, 6}
    }
};

// Größe der Gebäude
enum BuildingSize
{
    BZ_HUT = 0,
    BZ_HOUSE,
    BZ_CASTLE,
    BZ_MINE
};

// Bauqualitäten der Gebäude
const BuildingQuality BUILDING_SIZE[40] =
{
    BQ_CASTLE,
    BQ_HUT,
    BQ_HUT,
    BQ_NOTHING,
    BQ_HOUSE,
    BQ_NOTHING,
    BQ_NOTHING,
    BQ_NOTHING,
    BQ_NOTHING,
    BQ_CASTLE,
    BQ_MINE,
    BQ_MINE,
    BQ_MINE,
    BQ_MINE,
    BQ_HUT,
    BQ_NOTHING,
    BQ_HOUSE,
    BQ_HUT,
    BQ_HUT,
    BQ_HUT,
    BQ_HUT,
    BQ_HOUSE,
    BQ_HUT,
    BQ_HOUSE,
    BQ_HOUSE,
    BQ_HOUSE,
    BQ_HOUSE,
    BQ_CASTLE,
    BQ_CASTLE,
    BQ_HOUSE,
    BQ_NOTHING,
    BQ_HOUSE,
    BQ_HOUSE,
    BQ_HOUSE,
    BQ_HOUSE,
    BQ_HUT,
    BQ_HOUSE,
    BQ_CASTLE,
    BQ_CASTLE,
    BQ_HARBOR
};

// Konstanten zu den "normalen Gebäuden" (Betrieben), beginnt erst mit Granitmine
struct UsualBuilding
{
    /// Arbeitertyp, der in diesem Gebäude arbeitet
    Job job;
    /// Ware, die das Gebäude produziert
    GoodType produced_ware;
    /// Anzahl Waren, die das Gebäude benötigt
    unsigned char wares_needed_count;
    /// Waren, die das Gebäude benötigt
    GoodType wares_needed[3];
};



const UsualBuilding USUAL_BUILDING_CONSTS[30] =
{
    {JOB_MINER, GD_STONES, 3, {GD_FISH, GD_MEAT, GD_BREAD}},
    {JOB_MINER, GD_COAL, 3, {GD_FISH, GD_MEAT, GD_BREAD}},
    {JOB_MINER, GD_IRON, 3, {GD_FISH, GD_MEAT, GD_BREAD}},
    {JOB_MINER, GD_GOLD, 3, {GD_FISH, GD_MEAT, GD_BREAD}},
    {JOB_SCOUT, GD_NOTHING, 0, {GD_NOTHING, GD_NOTHING, GD_NOTHING}},
    {JOB_NOTHING, GD_NOTHING, 0, {GD_NOTHING, GD_NOTHING, GD_NOTHING}},
    {JOB_HELPER, GD_NOTHING, 1, {GD_STONES, GD_NOTHING, GD_NOTHING}},
    {JOB_WOODCUTTER, GD_WOOD, 0, {GD_NOTHING, GD_NOTHING, GD_NOTHING}},
    {JOB_FISHER, GD_FISH, 0, {GD_NOTHING, GD_NOTHING, GD_NOTHING}},
    {JOB_STONEMASON, GD_STONES, 0, {GD_NOTHING, GD_NOTHING, GD_NOTHING}},
    {JOB_FORESTER, GD_NOTHING, 0, {GD_NOTHING, GD_NOTHING, GD_NOTHING}},
    {JOB_BUTCHER, GD_MEAT, 1, {GD_HAM, GD_NOTHING, GD_NOTHING}},
    {JOB_HUNTER, GD_MEAT, 0, {GD_NOTHING, GD_NOTHING, GD_NOTHING}},
    {JOB_BREWER, GD_BEER, 2, {GD_GRAIN, GD_WATER, GD_NOTHING}},
    {JOB_ARMORER, GD_SWORD, 2, {GD_IRON, GD_COAL, GD_NOTHING}},
    {JOB_METALWORKER, GD_TONGS, 2, {GD_IRON, GD_BOARDS, GD_NOTHING}},
    {JOB_IRONFOUNDER, GD_IRON, 2, {GD_IRONORE, GD_COAL, GD_NOTHING}},
    {JOB_CHARBURNER, GD_COAL, 2, {GD_WOOD, GD_GRAIN, GD_NOTHING}},
    {JOB_PIGBREEDER, GD_HAM, 2, {GD_GRAIN, GD_WATER, GD_NOTHING}},
    {JOB_NOTHING, GD_NOTHING, 0, {GD_NOTHING, GD_NOTHING, GD_NOTHING}},
    {JOB_NOTHING, GD_NOTHING, 0, {GD_NOTHING, GD_NOTHING, GD_NOTHING}},
    {JOB_MILLER, GD_FLOUR, 1, {GD_GRAIN, GD_NOTHING, GD_NOTHING}},
    {JOB_BAKER, GD_BREAD, 2, {GD_FLOUR, GD_WATER}},
    {JOB_CARPENTER, GD_BOARDS, 1, {GD_WOOD, GD_NOTHING}},
    {JOB_MINTER, GD_COINS, 2, {GD_GOLD, GD_COAL, GD_NOTHING}},
    {JOB_HELPER, GD_WATER, 0, {GD_NOTHING, GD_NOTHING, GD_NOTHING}},
    {JOB_SHIPWRIGHT, GD_BOAT, 1, {GD_BOARDS, GD_NOTHING, GD_NOTHING}},
    {JOB_FARMER, GD_GRAIN, 0, {GD_NOTHING, GD_NOTHING, GD_NOTHING}},
    {JOB_DONKEYBREEDER, GD_NOTHING, 2, {GD_GRAIN, GD_WATER, GD_NOTHING}},
    {JOB_NOTHING, GD_NOTHING, 0, {GD_NOTHING, GD_NOTHING, GD_NOTHING}},
};

/// Rauch-Konstanten zu den "normalen Gebäuden" (Betrieben), beginnt erst mit Granitmine
struct SmokeConst
{
    /// Art des Rauches (von 1-4), 0 = kein Rauch!
    unsigned char type;
    /// Position des Rauches relativ zum Nullpunkt des Gebäudes
    signed char x, y;
};

/// Rauchkonstanten für alle 4 Völker unterschiedlich und erst beginnend nach den Militärgebäuden, denn die rauchen nicht
const SmokeConst BUILDING_SMOKE_CONSTS[NATION_COUNT][30] =
{
    // Nubier
    {
        {0, 0, 0}, // BLD_GRANITEMINE
        {0, 0, 0}, // BLD_COALMINE
        {0, 0, 0}, // BLD_IRONMINE
        {0, 0, 0}, // BLD_GOLDMINE
        {0, 0, 0}, // BLD_LOOKOUTTOWER
        {0, 0, 0}, // BLD_NOTHING7
        {0, 0, 0}, // BLD_CATAPULT
        {0, 0, 0}, // BLD_WOODCUTTER
        {0, 0, 0}, // BLD_FISHERY
        {1, 3, -32}, // BLD_QUARRY
        {0, 0, 0}, // BLD_FORESTER
        {0, 0, 0}, // BLD_SLAUGHTERHOUSE
        {0, 0, 0}, // BLD_HUNTER
        {0, 0, 0}, // BLD_BREWERY
        {1, -32, -23}, // BLD_ARMORY
        {4, -26, -47}, // BLD_METALWORKS
        {2, -20, -37}, // BLD_IRONSMELTER
        {2, -22, -57}, // BLD_CHARBURNER
        {0, 0, 0}, // BLD_PIGFARM
        {0, 0, 0}, // BLD_STOREHOUSE
        {0, 0, 0}, // BLD_NOTHING9
        {0, 0, 0}, // BLD_MILL
        {4, 27, -39}, // BLD_BAKERY
        {0, 0, 0}, // BLD_SAWMILL
        {1, 17, -52}, // BLD_MINT
        {0, 0, 0}, // BLD_WELL
        {0, 0, 0}, // BLD_SHIPYARD
        {0, 0, 0}, // BLD_FARM
        {0, 0, 0}, // BLD_DONKEYBREEDER
        {0, 0, 0} // BLD_HARBORBUILDING
    },
    // Japaner
    {
        {0, 0, 0}, // BLD_GRANITEMINE
        {0, 0, 0}, // BLD_COALMINE
        {0, 0, 0}, // BLD_IRONMINE
        {0, 0, 0}, // BLD_GOLDMINE
        {0, 0, 0}, // BLD_LOOKOUTTOWER
        {0, 0, 0}, // BLD_NOTHING7
        {0, 0, 0}, // BLD_CATAPULT
        {0, 0, 0}, // BLD_WOODCUTTER
        {0, 0, 0}, // BLD_FISHERY
        {0, 0, 0}, // BLD_QUARRY
        {0, 0, 0}, // BLD_FORESTER
        {0, 0, 0}, // BLD_SLAUGHTERHOUSE
        {0, 0, 0}, // BLD_HUNTER
        {0, 0, 0}, // BLD_BREWERY
        {1, -22, -43}, // BLD_ARMORY
        {0, 0, 0}, // BLD_METALWORKS
        {0, 0, 0}, // BLD_IRONSMELTER
        {2, -33, -57}, // BLD_CHARBURNER
        {0, 0, 0}, // BLD_PIGFARM
        {0, 0, 0}, // BLD_STOREHOUSE
        {0, 0, 0}, // BLD_NOTHING9
        {0, 0, 0}, // BLD_MILL
        {4, -30, -39}, // BLD_BAKERY
        {0, 0, 0}, // BLD_SAWMILL
        {3, 18, -58}, // BLD_MINT
        {0, 0, 0}, // BLD_WELL
        {0, 0, 0}, // BLD_SHIPYARD
        {0, 0, 0}, // BLD_FARM
        {0, 0, 0}, // BLD_DONKEYBREEDER
        {0, 0, 0} // BLD_HARBORBUILDING
    },
    // Römer
    {
        {0, 0, 0}, // BLD_GRANITEMINE
        {0, 0, 0}, // BLD_COALMINE
        {0, 0, 0}, // BLD_IRONMINE
        {0, 0, 0}, // BLD_GOLDMINE
        {0, 0, 0}, // BLD_LOOKOUTTOWER
        {0, 0, 0}, // BLD_NOTHING7
        {0, 0, 0}, // BLD_CATAPULT
        {0, 0, 0}, // BLD_WOODCUTTER
        {0, 0, 0}, // BLD_FISHERY
        {0, 0, 0}, // BLD_QUARRY
        {0, 0, 0}, // BLD_FORESTER
        {0, 0, 0}, // BLD_SLAUGHTERHOUSE
        {0, 0, 0}, // BLD_HUNTER
        {1, -26, -45}, // BLD_BREWERY
        {2, -36, -34}, // BLD_ARMORY
        {0, 0, 0}, // BLD_METALWORKS
        {1, -16, -34}, // BLD_IRONSMELTER
        {2, -44, -50}, // BLD_CHARBURNER
        {0, 0, 0}, // BLD_PIGFARM
        {0, 0, 0}, // BLD_STOREHOUSE
        {0, 0, 0}, // BLD_NOTHING9
        {0, 0, 0}, // BLD_MILL
        {4, -15, -26}, // BLD_BAKERY
        {0, 0, 0}, // BLD_SAWMILL
        {4, 20, -50}, // BLD_MINT
        {0, 0, 0}, // BLD_WELL
        {0, 0, 0}, // BLD_SHIPYARD
        {0, 0, 0}, // BLD_FARM
        {0, 0, 0}, // BLD_DONKEYBREEDER
        {0, 0, 0} // BLD_HARBORBUILDING
    },
    // Wikinger
    {
        {0, 0, 0}, // BLD_GRANITEMINE
        {0, 0, 0}, // BLD_COALMINE
        {0, 0, 0}, // BLD_IRONMINE
        {0, 0, 0}, // BLD_GOLDMINE
        {0, 0, 0}, // BLD_LOOKOUTTOWER
        {0, 0, 0}, // BLD_NOTHING7
        {0, 0, 0}, // BLD_CATAPULT
        {1, 2, -36}, // BLD_WOODCUTTER
        {1, 4, -36}, // BLD_FISHERY
        {1, 0, -34}, // BLD_QUARRY
        {1, -5, -29}, // BLD_FORESTER
        {1, 7, -41}, // BLD_SLAUGHTERHOUSE
        {1, -6, -38}, // BLD_HUNTER
        {3, 5, -39}, // BLD_BREWERY
        {3, -23, -36}, // BLD_ARMORY
        {1, -9, -35}, // BLD_METALWORKS
        {2, -2, -38}, // BLD_IRONSMELTER
        {2, -22, -55}, // BLD_CHARBURNER
        {2, -30, -37}, // BLD_PIGFARM
        {0, 0, 0}, // BLD_STOREHOUSE
        {0, 0, 0}, // BLD_NOTHING9
        {0, 0, 0}, // BLD_MILL
        {4, -21, -26}, // BLD_BAKERY
        {1, -11, -45}, // BLD_SAWMILL
        {1, 16, -38}, // BLD_MINT
        {0, 0, 0}, // BLD_WELL
        {0, 0, 0}, // BLD_SHIPYARD
        {1, -17, -48}, // BLD_FARM
        {4, -27, -40}, // BLD_DONKEYBREEDER
        {0, 0, 0} // BLD_HARBORBUILDING
    },
    // Babylonier
    {
        {0, 0, 0}, // BLD_GRANITEMINE
        {0, 0, 0}, // BLD_COALMINE
        {0, 0, 0}, // BLD_IRONMINE
        {0, 0, 0}, // BLD_GOLDMINE
        {0, 0, 0}, // BLD_LOOKOUTTOWER
        {0, 0, 0}, // BLD_NOTHING7
        {0, 0, 0}, // BLD_CATAPULT
        {0, 0, 0}, // BLD_WOODCUTTER
        {0, 0, 0}, // BLD_FISHERY
        {0, 0, 0}, // BLD_QUARRY
        {0, 0, 0}, // BLD_FORESTER
        {0, 0, 0}, // BLD_SLAUGHTERHOUSE
        {0, 0, 0}, // BLD_HUNTER
        {2, -18, -43}, // BLD_BREWERY
        {1, -22, -47}, // BLD_ARMORY
        {0, 0, 0}, // BLD_METALWORKS
        {2, -23, -36}, // BLD_IRONSMELTER
        {0, 0, 0}, // BLD_CHARBURNER
        {0, 0, 0}, // BLD_PIGFARM
        {0, 0, 0}, // BLD_STOREHOUSE
        {0, 0, 0}, // BLD_NOTHING9
        {0, 0, 0}, // BLD_MILL
        {4, -27, -32}, // BLD_BAKERY
        {0, 0, 0}, // BLD_SAWMILL
        {3, 11, -58}, // BLD_MINT
        {0, 0, 0}, // BLD_WELL
        {0, 0, 0}, // BLD_SHIPYARD
        {0, 0, 0}, // BLD_FARM
        {0, 0, 0}, // BLD_DONKEYBREEDER
        {0, 0, 0} // BLD_HARBORBUILDING
    }
};

/// Konstanten der "Produktions-Stopp"- und der "Gold-Stopp"-Schilder bei normalen und Militärgebäuden
struct BuildingSignConst
    { signed char x, y; };

const BuildingSignConst BUILDING_SIGN_CONSTS[NATION_COUNT][40] =
{
    // Nubier
    {
        {0, 0}, {19, -4}, {19, -3}, {0, 0}, {23, -19}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {29, -23},
        { -2, -15}, {2, -13}, { -5, -16}, { -5, -15}, {0, 0}, {0, 0}, {0, 0}, {4, -16}, {9, -12}, {7, -10},
        {28, -18}, { -6, -16}, { -3, -15}, {21, -11}, {14, -7}, { -14, -9}, {11, -9}, { -8, -21}, { -18, -34}, {0, 0},
        {0, 0}, {3, -13}, {13, -13}, { -4, -20}, {7, -14}, {15, -4}, {0, 0}, { -22, -16}, { -14, -8}, {0, 0}
    },
    // Japaner
    {   {0, 0}, {12, -10}, {13, -10}, {0, 0}, {25, -22}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {10, -14},
        {20, -2}, {12, -14}, {14, -13}, {13, -14}, {0, 0}, {0, 0}, {0, 0}, { -8, -6}, {0, 0}, { -13, -7},
        {14, -11}, {15, -10}, { -13, -7}, {16, -11}, {21, -3}, { -6, -2}, {14, -14}, { -5, -22}, {30, -25}, {0, 0},
        {0, 0}, {0, -22}, { -30, -13}, {35, -20}, {13, -34}, {19, -15}, { -22, -10}, {37, -13}, { -15, -36}, {0, 0}
    },
    // Römer
    {
        {0, 0}, {15, -3}, {14, -2}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {20, -39},
        {15, -2}, {26, 4}, {21, 4}, {21, 4}, {0, 0}, {0, 0}, {0, 0}, { -29, -10}, {0, 0}, { -4, 4},
        {7, -20}, {3, -13}, {0, 0}, {22, -8}, {18, -11}, {6, -14}, {23, -10}, { -36, -23}, {35, -27}, {0, 0},
        {0, 0}, {2, -21}, {15, -13}, {11, -30}, {23, -5}, {10, -9}, {0, -25}, {41, -43}, { -34, -19}, {0, 0}
    },
    // Wikinger
    {
        {0, 0}, { -5, 0}, { -5, 0}, {0, 0}, { -7, -5}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {17, -10},
        {20, 2}, {20, 0}, {20, -2}, {20, 0}, {0, 0}, {0, 0}, {13, -5}, {15, -6}, { -7, 2}, {17, -8},
        { -5, 0}, { -3, 2}, { -6, 0}, { -7, 0}, { -32, -2}, { -13, -3}, { -8, -2}, { -22, -18}, { -25, -8}, {0, 0},
        {0, 0}, { -8, -2}, { -17, -4}, {28, -16}, { -1, 0}, {8, -9}, {16, -15}, { -2, -25}, { -29, -9}, {0, 0}
    },
    // Babylonier
    {

        {0, 0}, {19, -6}, {19, -20}, {0, 0}, {5, -15}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {20, -10},
        {15, -10}, {15, -10}, {15, -10}, {15, -10}, {0, 0}, {0, 0}, {13, -5}, { -5, -13}, {15, -20}, {15, -1},
        {11, -7}, { 1, -22}, {7, -12}, {14, -16}, {21, -18}, {13, -11}, { 5, -17}, { -6, -33}, {18, -20}, {0, 0},
        {0, 0}, {14, -13}, {3, -17}, {0, -18}, {12, -10}, {16, 0}, {4, -16}, { -15, -11}, { -24, -9}, {0, 0}
    }
};

/// Position der nubischen Feuer für alle 4 Bergwerke
/// (Granit, Kohle, Eisen, Gold)
const int NUBIAN_MINE_FIRE[4][2] =
{
    {31, -18}, {34, -10}, {30, -11}, {32, -10},
};


/// Hilfetexte für Gebäude
const char* const BUILDING_HELP_STRINGS[40] =
{
    // Headquarters
    gettext_noop(
        "The headquarters represents the "
        "center of your realm. The large "
        "amount of storage space "
        "available means a great many "
        "things can be safely stored "
        "here. You can release certain "
        "merchandise from the "
        "headquarters, as and when "
        "required or stop its storage. To "
        "do this, first choose the "
        "corresponding icon followed by "
        "the desired merchandise or job "
        "symbol. On the third page, you "
        "can adjust the number of reserve "
        "soldiers who are responsible for "
        "guarding the headquarters. There "
        "are two values given: the first "
        "value indicates the current "
        "number of men, the second value "
        "indicates the desired number. "),

    // Barracks
    gettext_noop(
        "The barracks is a very small hut "
        "which can be used by your "
        "soldiers as quarters. Using the "
        "gold coin button you can stop "
        "the delivery of gold coins to "
        "the soldiers stationed here. "
        "However, without gold coins the "
        "soldiers here can not train and "
        "improve their skills."),

    // Guardhouse
    gettext_noop(
        "The guardhouse is a comfortable "
        "place for the military which is "
        "also protected by solid stone "
        "walls. Using the gold coin "
        "button you can stop the delivery "
        "of gold coins to the soldiers "
        "stationed here. However, without "
        "gold coins the soldiers here can "
        "not train and improve their "
        "skills."),

    // Nothing
    "",

    // Watchtower
    gettext_noop(
        "The watchtower with its large "
        "amount of space is best suited "
        "for stationing a large number "
        "of your troops. Using the gold coin "
        "button you can stop the delivery "
        "of gold coins to the soldiers "
        "stationed here. However, without "
        "gold coins the soldiers here can "
        "not train and improve their "
        "skills."),

    // 4x Nothing
    "", "", "", "",

    // Fortress
    gettext_noop(
        "The defensive capabilities and "
        "size of the fortress are "
        "unsurpassed. This stronghold "
        "ensures that other valuable "
        "buildings and commodities are "
        "protected. Using the gold coin "
        "button you can stop the delivery "
        "of gold coins to the soldiers "
        "stationed here. However, without "
        "gold coins the soldiers here can "
        "not train and improve their "
        "skills."),

    // Granite mine
    gettext_noop(
        "The quarrying of stone in a "
        "granite mine guarantees the "
        "supply of stone for buildings. "
        "However, even a granite mine has "
        "to feed its workers."),

    // Coal mine
    gettext_noop(
        "The mining of coal supports the "
        "metalworks and smithy. This hard "
        "work requires an adequate supply "
        "of food."),

    // Iron mine
    gettext_noop(
        "Deep within the mountains, "
        "miners dig for iron ore. They "
        "will obviously need a lot of "
        "food for the strenuous work."),

    // Gold mine
    gettext_noop(
        "A gold mine allows you to "
        "prospect for valuable gold "
        "deposits. For this, it is "
        "necessary to ensure that the "
        "miners are provided with plenty "
        "of food."),

    // Lookout-tower
    gettext_noop(
        "From the lookout tower you can "
        "see far into previously "
        "unexplored lands."),

    // Nothing
    "",

    // Catapult
    gettext_noop(
        "Thanks to its immense strength, "
        "the catapults represents an "
        "effective weapon against enemy "
        "military buildings."),

    // Woodcutter
    gettext_noop(
        "A woodcutter provides the "
        "sawmill with logs. A forester is "
        "able to replant the depleted "
        "forest."),

    // Fishery
    gettext_noop(
        "The fish man is responsible for "
        "finding water rich in fish. His "
        "fish feed a great many miners."),

    // Quarry
    gettext_noop(
        "The stonemason works the stone "
        "near his quarry into bricks. "
        "These are needed mainly for "
        "building houses and as "
        "ammunition for catapults."),

    // Forester
    gettext_noop(
        "Within his area, the forester "
        "ensures the survival of the "
        "forest. He plants all types of "
        "trees."),

    // Slaughterhouse
    gettext_noop(
        "The butcher processes the "
        "livestock delivered into "
        "nutritious ham on which you "
        "miners are fed."),

    // Hunter
    gettext_noop(
        "Meat the hunter acquires is used "
        "to feed the miners."),

    // Brewery
    gettext_noop(
        "The brewer produces fine beer "
        "from grain and water. This drink "
        "is needed to fill the soldiers "
        "with courage."),

    // Armory
    gettext_noop(
        "The armory produces swords and "
        "strong shields. This equipment "
        "is vital for your soldiers."),

    // Metalworks
    gettext_noop(
        "The countless tools which your "
        "workers need are made in the "
        "metalworks. This requires boards "
        "and iron."),

    // Iron smelter
    gettext_noop(
        "Raw iron ore is smelted in the "
        "iron smelters with the help of "
        "coal. The processed iron is "
        "then used to making weapons "
        "(in the Armory) and tools "
        "(in the metalworks)."),

    // Nothing
    "",

    // Pig farm
    gettext_noop(
        "Grain and water are needed for "
        "rearing pigs. The meat thus "
        "obtained can then be processed "
        "by a butcher."),

    // Storehouse
    gettext_noop(
        "The storehouse can help reduce "
        "long transportation journeys and "
        "is suited to the temporary "
        "storage of merchandise and "
        "inhabitants. You can release "
        "certain merchandise from the "
        "storehouse, as and when "
        "required. Alternatively, the "
        "storage function can be "
        "disabled. To do this, first "
        "choose the relevant icon "
        "followed by the desired "
        "merchandise or job symbol."),

    // Nothing
    "",

    // Mill
    gettext_noop(
        "The grain is ground in the "
        "windmill. The flour from the "
        "windmill is later used at the "
        "bakery to bake bread."),

    // Bakery
    gettext_noop(
        "The flour produced at the "
        "windmill can be combined with "
        "water in the bakery to make "
        "oven-fresh bread. It's your "
        "miners' favorite!"),

    // Sawmill
    gettext_noop(
        "The carpenter turns the "
        "woodcutter's logs into "
        "made-to-measure planks. These "
        "form the basic for building "
        "houses and ships."),

    // Mint
    gettext_noop(
        "The mint is responsible for "
        "producing valuable gold coins. "
        "These precious objects are "
        "produced using coal and gold."),

    // Well
    gettext_noop(
        "A well supplies water to the "
        "bakery, brewery, donkey breeder "
        "and pig farm."),

    // Shipyard
    gettext_noop(
        "It is possible to build small "
        "rowing boats as well as huge "
        "cargo ships in a shipyard. The "
        "boards required for this are "
        "skillfully worked by "
        "shipwrights."),

    // Farm
    gettext_noop(
        "The farmer plants and harvests "
        "grain in the surrounding fields. "
        "A windmill then processes the "
        "harvested grain into flour or "
        "can be used to feed the pigs."),

    // Donkey breeder
    gettext_noop(
        "The pack donkeys bred here are "
        "used to transport your "
        "merchandise more efficiently. "
        "They are reared on water and "
        "grain."),

    // Harbor building
    gettext_noop(
        "Ships can only be loaded and "
        "unloaded in a harbor. "
        "Expeditions can also be prepared "
        "here. You can release certain "
        "merchandise from the storehouse, "
        "as and when required. "
        "Alternatively, the storage "
        "function can be disabled. To do "
        "this, first choose the relevant "
        "icon followed by the desired "
        "merchandise or job symbol."),

};




#endif
