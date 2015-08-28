// $Id: ctrlStatisticTable.h 
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
#ifndef CTRLSTATISTICTABLE_H_INCLUDED
#define CTRLSTATISTICTABLE_H_INCLUDED

#pragma once

#include "Window.h"
#include <vector>
#include "EndStatisticData.h"



class ctrlStatisticTable : public Window
{
    public:
        struct Column {
            std::string title;
            bool is_button;

            Column (const std::string &title, bool is_button) 
                : title(title), is_button(is_button) { }
        };




    public:
        ctrlStatisticTable(Window* parent, unsigned int id, unsigned short x, unsigned short y, 
            unsigned short width, unsigned short height, const std::vector<Column>& column_titles, unsigned num_rows);

        virtual ~ctrlStatisticTable(void);
              
        void AddPlayerInfos(const std::vector<EndStatisticData::PlayerInfo> &player_infos);
        void AddColumn(unsigned col_idx, const std::vector<unsigned> &points);

        virtual void Msg_ButtonClick(const unsigned int ctrl_id);
        virtual bool Msg_MouseMove(const MouseCoords& mc);
        virtual bool Msg_LeftUp(const MouseCoords& mc);
        virtual bool Msg_LeftDown(const MouseCoords& mc);

    protected:

        virtual bool Draw_(void);

        /// Größe ändern
        void Resize_(unsigned short width, unsigned short height);




    private:

        struct SizeInfos {
            int x_start;
            int x_grid;
            int col_width;
            int player_col_width;

            int y_start;
            int y_grid;
        } s;


        std::vector<Column> _columns;
        unsigned _max_num_rows;
        unsigned _num_rows;
        unsigned short _fifty;
        std::vector<EndStatisticData::PlayerInfo> _players;
        


};

#endif // !CTRLSTATISTICTABLE_H_INCLUDED
