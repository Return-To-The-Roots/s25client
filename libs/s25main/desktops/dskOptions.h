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
#ifndef dskOPTIONS_H_INCLUDED
#define dskOPTIONS_H_INCLUDED

#pragma once

#include "Desktop.h"
#include "GlobalGameSettings.h"
#include "driver/VideoMode.h"

/// Klasse des Optionen Desktops.
class dskOptions : public Desktop
{
public:
    dskOptions();
    ~dskOptions() override;

private:
    void Msg_OptionGroupChange(unsigned ctrl_id, unsigned selection) override;
    void Msg_ButtonClick(unsigned ctrl_id) override;
    void Msg_MsgBoxResult(unsigned msgbox_id, MsgboxResult mbr) override;

    void Msg_Group_ButtonClick(unsigned group_id, unsigned ctrl_id) override;
    void Msg_Group_ProgressChange(unsigned group_id, unsigned ctrl_id, unsigned short position) override;
    void Msg_Group_ComboSelectItem(unsigned group_id, unsigned ctrl_id, unsigned selection) override;
    void Msg_Group_OptionGroupChange(unsigned group_id, unsigned ctrl_id, unsigned selection) override;

private:
    GlobalGameSettings ggs;
    std::vector<VideoMode> video_modes; /// Vector für die Auflösungen

    void loadVideoModes();
};

#endif // !dskOPTIONS_H_INCLUDED
