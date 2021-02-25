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
