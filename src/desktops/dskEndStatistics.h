// $Id: dskEndStatistics.h 
//
// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef dskENDSTATISTICS_H_INCLUDED
#define dskENDSTATISTICS_H_INCLUDED

#pragma once

#include "Desktop.h"
#include "EndStatisticData.h"

/// Klasse des Optionen Desktops.
class dskEndStatistics: public Desktop
{
public:
    dskEndStatistics(EndStatisticData *data);
    ~dskEndStatistics();

    // Remove these after finalizing
    virtual void Msg_PaintAfter();
    bool _info_shown;


private:

    void ShowOverview();
    void ShowCategory(EndStatisticData::CategoryIndex cat);

    void Msg_ButtonClick(const unsigned int ctrl_id);

    void Msg_StatisticGroupChange(const unsigned int ctrl_id, const unsigned short selection);

    bool _in_overview;

    EndStatisticData *data;

};

#endif // !dskENDSTATISTICS_H_INCLUDED
