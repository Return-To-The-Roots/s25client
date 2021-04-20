// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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

const std::array<FullJobData, helpers::NumEnumValues_v<Job>> fullJobData = {{
  // Start data
  {{GoodType::Nothing, 385, 190, 5}, {false, 0}, gettext_noop("Helper")},
  {{GoodType::Axe, 148, 789, 5}, {false, 5}, gettext_noop("Woodchopper")},
  {{GoodType::RodAndLine, 129, 825, 5}, {false, 12}, gettext_noop("Fisher")},
  {{GoodType::Shovel, 66, 304, 5}, {false, 8}, gettext_noop("Ranger")},
  {{GoodType::Saw, 479, 96, 5}, {false, 6}, gettext_noop("Carpenter")},
  {{GoodType::PickAxe, 129, 825, 5}, {false, 7}, gettext_noop("Stonemason")},
  {{GoodType::Bow, 0, 300, 5}, {false, 20}, gettext_noop("Huntsman")},
  {{GoodType::Scythe, 117, 106, 5}, {false, 13}, gettext_noop("Farmer")},
  {{GoodType::Nothing, 470, 95, 5}, {true, 16}, gettext_noop("Miller")},
  {{GoodType::Rollingpin, 470, 94, 5}, {true, 17}, gettext_noop("Baker")},
  {{GoodType::Cleaver, 478, 80, 5}, {false, 15}, gettext_noop("Butcher")},
  {{GoodType::PickAxe, 583, 558, 5}, {false, 10}, gettext_noop("Miner")},
  {{GoodType::Nothing, 530, 93, 5}, {true, 3}, gettext_noop("Brewer")},
  {{GoodType::Nothing, 390, 160, 5}, {false, 14}, gettext_noop("Pig breeder")},
  {{GoodType::Nothing, 370, 278, 205}, {false, 24}, gettext_noop("Donkey breeder")},
  {{GoodType::Crucible, 950, 160, 5}, {false, 11}, gettext_noop("Iron founder")},
  {{GoodType::Crucible, 1050, 170, 5}, {false, 9}, gettext_noop("Minter")},
  {{GoodType::Tongs, 850, 400, 5}, {false, 18}, gettext_noop("Metalworker")},
  {{GoodType::Hammer, 940, 170, 5}, {true, 4}, gettext_noop("Armorer")},
  {{GoodType::Hammer, 0, 0, 5}, {false, 23}, gettext_noop("Builder")},
  {{GoodType::Shovel, 130, 0, 5}, {false, 22}, gettext_noop("Planer")},
  {{}, {false, -30}, gettext_noop("Private")},
  {{}, {false, -31}, gettext_noop("Private first class")},
  {{}, {false, -32}, gettext_noop("Sergeant")},
  {{}, {false, -33}, gettext_noop("Officer")},
  {{}, {false, -34}, gettext_noop("General")},
  {{GoodType::Hammer, 0, 0, 0}, {false, 26}, gettext_noop("Geologist")},
  {{GoodType::Hammer, 1250, 100, 5},
   {false, 25},
   gettext_noop("Shipwright")}, // Todo: Timing wenn Schiffe bauen möglich
  {{GoodType::Bow, 0, 0, 0}, {false, -35}, gettext_noop("Scout")},
  {{}, {false, 37}, gettext_noop("Pack donkey")},
  {{}, {false, 37}, gettext_noop("Boat carrier")},
  {{GoodType::Shovel, 117, 106, 5}, {false, 37}, gettext_noop("Charburner")}
  // End data
}};

template<typename T, std::size_t... I>
constexpr auto makeJobArrayImpl(T&& getter, std::index_sequence<I...>)
{
    using Type = decltype(getter(std::declval<FullJobData>()));
    return helpers::EnumArray<Type, Job>{getter(fullJobData[I])...};
}

template<typename T>
constexpr auto makeJobArray(T&& getter)
{
    return makeJobArrayImpl(getter, std::make_index_sequence<helpers::NumEnumValues_v<Job>>());
}
} // namespace

const helpers::EnumArray<std::string, Job> JOB_NAMES =
  makeJobArray([](const FullJobData& data) { return std::string(data.name); });
const helpers::EnumArray<JobConst, Job> JOB_CONSTS = makeJobArray([](const FullJobData& data) { return data.data; });
const helpers::EnumArray<JobSpriteData, Job> JOB_SPRITE_CONSTS =
  makeJobArray([](const FullJobData& data) { return data.spriteData; });

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
    if(rttr::enum_cast(nation) < NUM_NATIVE_NATIONS)
        id += NATION_RTTR_TO_S2[nation] * numNationSpecificJobs;
    else
        id += newNationSpecificOffset + (rttr::enum_cast(nation) - NUM_NATIVE_NATIONS) * numNationSpecificJobs;
    return id;
}
