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

#ifndef JOB_CONSTS_H_
#define JOB_CONSTS_H_

#include "DrawPointInit.h"
#include "helpers/SimpleMultiArray.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/JobTypes.h"
#include <array>

extern const std::array<std::string, NUM_JOB_TYPES> JOB_NAMES;

/// Berufsstruktur
struct JobConst
{
    /// Tool for this job, GD_INVALID if recruiting is not possible
    GoodType tool;
    /// Ob der Beruf dick oder dünn ist
    bool fat;
    /// ID in Jobs.BOB
    unsigned short jobs_bob_id;
    /// Arbeitszeit in gf, Wartezeit (vor dem Arbeiten) in gf
    unsigned short work_length, wait1_length, wait2_length;
};

// Welcher Beruf welches Werkzeug braucht
const JobConst JOB_CONSTS[NUM_JOB_TYPES] = {
  {GD_NOTHING, false, 0, 385, 190, 5},     // JOB_HELPER
  {GD_AXE, false, 5, 148, 789, 5},         // JOB_WOODCUTTER
  {GD_RODANDLINE, false, 12, 129, 825, 5}, // JOB_FISHER
  {GD_SHOVEL, false, 8, 66, 304, 5},       // JOB_FORESTER
  {GD_SAW, false, 6, 479, 96, 5},          // JOB_CARPENTER
  {GD_PICKAXE, false, 7, 129, 825, 5},     // JOB_STONEMASON
  {GD_BOW, false, 20, 0, 300, 5},          // JOB_HUNTER
  {GD_SCYTHE, false, 13, 117, 106, 5},     // JOB_FARMER
  {GD_NOTHING, true, 16, 470, 95, 5},      // JOB_MILLER
  {GD_ROLLINGPIN, true, 17, 470, 94, 5},   // JOB_BAKER
  {GD_CLEAVER, false, 15, 478, 80, 5},     // JOB_BUTCHER
  {GD_PICKAXE, false, 10, 583, 558, 5},    // JOB_MINER
  {GD_NOTHING, true, 3, 530, 93, 5},       // JOB_BREWER
  {GD_NOTHING, false, 14, 390, 160, 5},    // JOB_PIGBREEDER
  {GD_NOTHING, false, 24, 370, 278, 205},  // JOB_DONKEYBREEDER
  {GD_CRUCIBLE, false, 11, 950, 160, 5},   // JOB_IRONFOUNDER
  {GD_CRUCIBLE, false, 9, 1050, 170, 5},   // JOB_MINTER
  {GD_TONGS, false, 18, 850, 400, 5},      // JOB_METALWORKER
  {GD_HAMMER, true, 4, 940, 170, 5},       // JOB_ARMORER
  {GD_HAMMER, false, 23, 0, 0, 5},         // JOB_BUILDER
  {GD_SHOVEL, false, 22, 130, 0, 5},       // JOB_PLANER
  {GD_INVALID, false, 30, 0, 0, 0},        // JOB_PRIVATE
  {GD_INVALID, false, 31, 0, 0, 0},        // JOB_PRIVATEFIRSTCLASS
  {GD_INVALID, false, 32, 0, 0, 0},        // JOB_SERGEANT
  {GD_INVALID, false, 33, 0, 0, 0},        // JOB_OFFICER
  {GD_INVALID, false, 34, 0, 0, 0},        // JOB_GENERAL
  {GD_HAMMER, false, 26, 0, 0, 0},         // JOB_GEOLOGIST
  {GD_HAMMER, false, 25, 1250, 100, 5},    // JOB_SHIPWRIGHT, Todo: Timing wenn Schiffe bauen möglich
  {GD_BOW, false, 35, 0, 0, 0},            // JOB_SCOUT
  {GD_INVALID, false, 37, 0, 0, 0},        // JOB_PACKDONKEY
  {GD_INVALID, false, 37, 0, 0, 0},        // JOB_BOATCARRIER
  {GD_SHOVEL, false, 37, 117, 106, 5}      // JOB_CHARBURNER
};

/// Katapultmann-Wartezeit
const unsigned CATAPULT_WAIT1_LENGTH = 1300; // eigenlich 310 - aber hochgestellt wegen zu schneller Warenverteilung

/// Position of the ware on a donkey per direction and animation step
const helpers::SimpleMultiArray<DrawPointInit, 6, 8> WARE_POS_DONKEY = {
  {{{0, -13}, {0, -12}, {0, -12}, {1, -13}, {2, -13}, {2, -12}, {2, -12}, {1, -13}},
   {{3, -12}, {3, -11}, {3, -11}, {3, -12}, {4, -12}, {5, -11}, {5, -11}, {5, -12}},
   {{2, -9}, {1, -8}, {0, -7}, {1, -8}, {2, -9}, {3, -8}, {4, -8}, {3, -9}},
   {{-1, -8}, {-2, -7}, {-3, -7}, {-2, -8}, {-1, -8}, {0, -7}, {1, -7}, {0, -8}},
   {{-3, -7}, {-2, -6}, {-1, -6}, {-1, -7}, {-2, -7}, {-3, -6}, {-4, -6}, {-3, -7}},
   {{-3, -10}, {-3, -9}, {-3, -9}, {-2, -10}, {-1, -10}, {-1, -10}, {-1, -9}, {-1, -10}}}};

/// Positionen der Ware im Boat für jede Richtung
const std::array<DrawPointInit, 6> WARE_POS_BOAT = {{{11, -4}, {11, 0}, {-7, -1}, {-8, -5}, {-7, -7}, {6, -7}}};

#endif
