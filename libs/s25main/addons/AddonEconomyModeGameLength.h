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
#include "mygettext/mygettext.h"
#include <boost/format.hpp>

/**
 *  Addon allows users to adjust the game length (for Economy Mode)
 */
const unsigned int AddonEconomyModeGameLengthList[] = {0, 15, 30, 60, 90, 120, 150, 180, 240, 480}; // length in minutes

class AddonEconomyModeGameLength : public AddonList
{
public:
    AddonEconomyModeGameLength()
        : AddonList(AddonId::ECONOMY_MODE_GAME_LENGTH, AddonGroup::Economy | AddonGroup::GamePlay,
                    _("Economy Mode: Game Length"),
                    _("Adjust the time after which the economy mode victory condition is checked."),
                    {_("unlimited"), (boost::format(_("%1%min")) % AddonEconomyModeGameLengthList[1]).str(),
                     (boost::format(_("%1%min")) % AddonEconomyModeGameLengthList[2]).str(),
                     (boost::format(_("%1%min")) % AddonEconomyModeGameLengthList[3]).str(),
                     (boost::format(_("%1%min")) % AddonEconomyModeGameLengthList[4]).str(),
                     (boost::format(_("%1%min")) % AddonEconomyModeGameLengthList[5]).str(),
                     (boost::format(_("%1%min")) % AddonEconomyModeGameLengthList[6]).str(),
                     (boost::format(_("%1%min")) % AddonEconomyModeGameLengthList[7]).str(),
                     (boost::format(_("%1%min")) % AddonEconomyModeGameLengthList[8]).str(),
                     (boost::format(_("%1%min")) % AddonEconomyModeGameLengthList[9]).str()},
                    5)
    {}
};
