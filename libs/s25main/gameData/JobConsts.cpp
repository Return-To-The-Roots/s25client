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

#include "gameData/JobConsts.h"
#include "mygettext/mygettext.h"
#include "gameData/NationConsts.h"

namespace {
struct FullJobData
{
    JobConst data;
    JobSpriteData spriteData;
    const char* name;
};

constexpr std::array<FullJobData, NUM_JOB_TYPES> fullJobData = {{
  // Start data
  {{GD_NOTHING, 385, 190, 5}, {false, 0}, gettext_noop("Helper")},
  {{GD_AXE, 148, 789, 5}, {false, 5}, gettext_noop("Woodchopper")},
  {{GD_RODANDLINE, 129, 825, 5}, {false, 12}, gettext_noop("Fisher")},
  {{GD_SHOVEL, 66, 304, 5}, {false, 8}, gettext_noop("Ranger")},
  {{GD_SAW, 479, 96, 5}, {false, 6}, gettext_noop("Carpenter")},
  {{GD_PICKAXE, 129, 825, 5}, {false, 7}, gettext_noop("Stonemason")},
  {{GD_BOW, 0, 300, 5}, {false, 20}, gettext_noop("Huntsman")},
  {{GD_SCYTHE, 117, 106, 5}, {false, 13}, gettext_noop("Farmer")},
  {{GD_NOTHING, 470, 95, 5}, {true, 16}, gettext_noop("Miller")},
  {{GD_ROLLINGPIN, 470, 94, 5}, {true, 17}, gettext_noop("Baker")},
  {{GD_CLEAVER, 478, 80, 5}, {false, 15}, gettext_noop("Butcher")},
  {{GD_PICKAXE, 583, 558, 5}, {false, 10}, gettext_noop("Miner")},
  {{GD_NOTHING, 530, 93, 5}, {true, 3}, gettext_noop("Brewer")},
  {{GD_NOTHING, 390, 160, 5}, {false, 14}, gettext_noop("Pig breeder")},
  {{GD_NOTHING, 370, 278, 205}, {false, 24}, gettext_noop("Donkey breeder")},
  {{GD_CRUCIBLE, 950, 160, 5}, {false, 11}, gettext_noop("Iron founder")},
  {{GD_CRUCIBLE, 1050, 170, 5}, {false, 9}, gettext_noop("Minter")},
  {{GD_TONGS, 850, 400, 5}, {false, 18}, gettext_noop("Metalworker")},
  {{GD_HAMMER, 940, 170, 5}, {true, 4}, gettext_noop("Armorer")},
  {{GD_HAMMER, 0, 0, 5}, {false, 23}, gettext_noop("Builder")},
  {{GD_SHOVEL, 130, 0, 5}, {false, 22}, gettext_noop("Planer")},
  {{GD_INVALID, 0, 0, 0}, {false, -30}, gettext_noop("Private")},
  {{GD_INVALID, 0, 0, 0}, {false, -31}, gettext_noop("Private first class")},
  {{GD_INVALID, 0, 0, 0}, {false, -32}, gettext_noop("Sergeant")},
  {{GD_INVALID, 0, 0, 0}, {false, -33}, gettext_noop("Officer")},
  {{GD_INVALID, 0, 0, 0}, {false, -34}, gettext_noop("General")},
  {{GD_HAMMER, 0, 0, 0}, {false, 26}, gettext_noop("Geologist")},
  {{GD_HAMMER, 1250, 100, 5}, {false, 25}, gettext_noop("Shipwright")}, // Todo: Timing wenn Schiffe bauen möglich
  {{GD_BOW, 0, 0, 0}, {false, -35}, gettext_noop("Scout")},
  {{GD_INVALID, 0, 0, 0}, {false, 37}, gettext_noop("Pack donkey")},
  {{GD_INVALID, 0, 0, 0}, {false, 37}, gettext_noop("Boat carrier")},
  {{GD_SHOVEL, 117, 106, 5}, {false, 37}, gettext_noop("Charburner")}
  // End data
}};

template<typename T, std::size_t... I>
constexpr auto makeJobArrayImpl(T&& getter, std::index_sequence<I...>)
{
    using Type = decltype(getter(std::declval<FullJobData>()));
    return std::array<Type, sizeof...(I)>{getter(fullJobData[I])...};
}

template<typename T>
constexpr auto makeJobArray(T&& getter)
{
    return makeJobArrayImpl(getter, std::make_index_sequence<NUM_JOB_TYPES>());
}
} // namespace

const std::array<std::string, NUM_JOB_TYPES> JOB_NAMES = makeJobArray([](const FullJobData& data) { return std::string(data.name); });
const std::array<JobConst, NUM_JOB_TYPES> JOB_CONSTS = makeJobArray([](const FullJobData& data) { return data.data; });
const std::array<JobSpriteData, NUM_JOB_TYPES> JOB_SPRITE_CONSTS = makeJobArray([](const FullJobData& data) { return data.spriteData; });

unsigned short JobSpriteData::getBobId(Nation nation) const
{
    constexpr unsigned numNationSpecificJobs = 6;
    constexpr unsigned firstNationSpecificJobBobId = 30;
    constexpr unsigned lastS2JobBobId = 92;
    // First Job.BOB id for nation specific job images (links) for new nations
    constexpr unsigned newNationSpecificOffset = lastS2JobBobId - firstNationSpecificJobBobId + 1;

    if(jobBobId >= 0)
        return static_cast<unsigned short>(jobBobId);
    auto id = static_cast<unsigned short>(-jobBobId);
    if(nation < NUM_NATIVE_NATIONS)
        id += NATION_RTTR_TO_S2[nation] * numNationSpecificJobs;
    else
        id += newNationSpecificOffset + (nation - NUM_NATIVE_NATIONS) * numNationSpecificJobs;
    return id;
}
