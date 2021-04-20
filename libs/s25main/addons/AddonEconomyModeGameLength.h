// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AddonList.h"
#include "RTTR_Assert.h"
#include "helpers/chronoIO.h"
#include "helpers/containerUtils.h"
#include "helpers/format.hpp"
#include "helpers/make_array.h"
#include "mygettext/mygettext.h"
#include <chrono>

/**
 *  Addon allows users to adjust the game length (for Economy Mode)
 */
constexpr auto AddonEconomyModeGameLengthList =
  helpers::make_array<std::chrono::minutes>(0, 15, 30, 60, 90, 120, 150, 180, 240, 480);

class AddonEconomyModeGameLength : public AddonList
{
    static std::vector<std::string> makeOptions()
    {
        std::vector<std::string> result = {_("unlimited")};
        for(const auto duration : AddonEconomyModeGameLengthList)
            result.push_back(helpers::format("%1%", helpers::withUnit(duration)));
        return result;
    }

public:
    AddonEconomyModeGameLength()
        : AddonList(AddonId::ECONOMY_MODE_GAME_LENGTH, AddonGroup::Economy | AddonGroup::GamePlay,
                    _("Economy Mode: Game Length"),
                    _("Adjust the time after which the economy mode victory condition is checked."), makeOptions(),
                    helpers::indexOf(AddonEconomyModeGameLengthList, std::chrono::minutes(120)))

    {
        RTTR_Assert(getDefaultStatus() != unsigned(-1));
    }
};
