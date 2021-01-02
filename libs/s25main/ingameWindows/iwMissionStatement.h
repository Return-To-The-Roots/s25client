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

#pragma once

#include "IngameWindow.h"

class iwMissionStatement : public IngameWindow
{
public:
    /// Image shown in bottom right corner
    using HelpImage = int;
    enum HelpImageConsts
    {
        /// No image
        IM_NONE,
        /// Default, Octavianus with sword standing
        IM_SWORDSMAN = 234,
        /// Octavianus reading on stone
        IM_READER,
        /// Octavianus riding on horse
        IM_RIDER,
        /// Avatar images of the nations (so IO.LST)
        IM_AVATAR1 = 239,
        IM_AVATAR2,
        IM_AVATAR3,
        IM_AVATAR4,
        IM_AVATAR5,
        IM_AVATAR6,
        IM_AVATAR7,
        IM_AVATAR8,
        /// Mentor
        IM_AVATAR9,
        IM_AVATAR10,
        IM_AVATAR11,
        IM_AVATAR12
    };
    iwMissionStatement(const std::string& title, const std::string& content, bool pauseGame,
                       HelpImage image = IM_SWORDSMAN);
    void Msg_ButtonClick(unsigned ctrl_id) override;
    void SetActive(bool activate) override;

private:
    bool pauseGame_;
};
