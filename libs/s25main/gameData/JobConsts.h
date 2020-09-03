// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "DrawPoint.h"
#include "helpers/EnumArray.h"
#include "gameTypes/Direction.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/JobTypes.h"
#include "gameTypes/Nation.h"
#include <boost/optional/optional.hpp>
#include <array>

extern const helpers::EnumArray<std::string, Job> JOB_NAMES;

/// Game data for each job
struct JobConst
{
    /// Tool required to recruit this job (helper + tool = new worker). Empty if recruiting is not possible
    boost::optional<GoodType> tool;
    /// Duration of working and waiting (before and between work steps) in GFs
    unsigned short work_length, wait1_length, wait2_length;
};

/// Sprite definition for each job
struct JobSpriteData
{
private:
    /// Ob der Beruf dick oder dünn ist
    bool fat;
    /// ID in Jobs.BOB. Negative if abs value needs nation adjustment
    short jobBobId;

public:
    constexpr JobSpriteData(bool fat, short jobBobId) : fat(fat), jobBobId(jobBobId) {}
    /// Get ID in Jobs.BOB file
    bool isFat() const { return fat; }
    /// Get ID in Jobs.BOB file
    unsigned short getBobId(Nation) const;
};

/// Data for each job
extern const helpers::EnumArray<JobConst, Job> JOB_CONSTS;
extern const helpers::EnumArray<JobSpriteData, Job> JOB_SPRITE_CONSTS;

/// Katapultmann-Wartezeit
const unsigned CATAPULT_WAIT1_LENGTH = 1300; // eigenlich 310 - aber hochgestellt wegen zu schneller Warenverteilung

/// Position of the ware on a donkey per direction and animation step
const helpers::EnumArray<std::array<DrawPoint, 8>, Direction> WARE_POS_DONKEY = {{
  {{{0, -13}, {0, -12}, {0, -12}, {1, -13}, {2, -13}, {2, -12}, {2, -12}, {1, -13}}},
  {{{3, -12}, {3, -11}, {3, -11}, {3, -12}, {4, -12}, {5, -11}, {5, -11}, {5, -12}}},
  {{{2, -9}, {1, -8}, {0, -7}, {1, -8}, {2, -9}, {3, -8}, {4, -8}, {3, -9}}},
  {{{-1, -8}, {-2, -7}, {-3, -7}, {-2, -8}, {-1, -8}, {0, -7}, {1, -7}, {0, -8}}},
  {{{-3, -7}, {-2, -6}, {-1, -6}, {-1, -7}, {-2, -7}, {-3, -6}, {-4, -6}, {-3, -7}}},
  {{{-3, -10}, {-3, -9}, {-3, -9}, {-2, -10}, {-1, -10}, {-1, -10}, {-1, -9}, {-1, -10}}},
}};

/// Positionen der Ware im Boat für jede Richtung
const helpers::EnumArray<DrawPoint, Direction> WARE_POS_BOAT = {{{11, -4}, {11, 0}, {-7, -1}, {-8, -5}, {-7, -7}, {6, -7}}};

#endif
