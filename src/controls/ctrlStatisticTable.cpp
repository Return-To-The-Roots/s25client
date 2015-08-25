// $Id: ctrlTable.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "defines.h"
#include "ctrlStatisticTable.h"
#include "ctrlButton.h"
#include "ctrlPercent.h"

#include "Loader.h"
#include "ogl/glArchivItem_Font.h"
#include <sstream>
#include <cstdarg>

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


ctrlStatisticTable::ctrlStatisticTable(Window* parent,
                     unsigned int id,
                     unsigned short x,
                     unsigned short y,
                     unsigned short width,
                     unsigned short height, 
                     const std::vector<Column>& column_titles,
                     unsigned max_num_rows)
    : Window(x, y, id, parent, width, height), _columns(column_titles), _max_num_rows(max_num_rows), _num_rows(0)
{
    s.col_width = ScaleX(100);
    s.player_col_width = ScaleX(100);

    s.x_start = s.player_col_width;
    s.y_start = 0;

    s.x_grid = (width - s.player_col_width) / (column_titles.size() - 1);
    s.y_grid = height / (8 + 1); // fix fow now TODO: add scrollbar

    _fifty = 50;

    for(unsigned short i = 0; i < _columns.size(); ++i)
    {
        // Button für die Spalte hinzufügen
        if (_columns[i].is_button)
        {
            AddTextButton(i + 1, 
                s.x_start + (i-1) * s.x_grid + (s.x_grid/2) - (s.col_width/2), 
                0, 
                s.col_width, 22, TC_GREY, _columns[i].title, NormalFont);
        }
        else
        {
            AddText(i + 1, 
                (i == 0) ? (s.player_col_width/4) : (s.x_start + (i-1) * s.x_grid + (s.x_grid/2)), 
                5, 
                _columns[i].title, COLOR_YELLOW, (i == 0) ? glArchivItem_Font::DF_LEFT : glArchivItem_Font::DF_CENTER, NormalFont);
        }
    }



}


ctrlStatisticTable::~ctrlStatisticTable(void)
{

}


void ctrlStatisticTable::Resize_(unsigned short width, unsigned short height)
{
    //// changed height

    //scrollbar->Move(width - 20, 0);
    //scrollbar->Resize(20, height);

    //line_count = (height - header_height - 2) / font->getHeight();
    //scrollbar->SetPageSize(line_count);

    //// If the size was enlarged we have to check that we don't try to
    //// display more lines than present
    //if(height > this->height)
    //    while(rows.size() - scrollbar->GetPos() < line_count
    //            && scrollbar->GetPos() > 0)
    //        scrollbar->SetPos(scrollbar->GetPos() - 1);

    //// changed width

    //this->width = width;
    //ResetButtonWidths();
    //if(scrollbar->GetVisible())
    //    Msg_ScrollShow(0, true);
}

void ctrlStatisticTable::AddPlayerInfos(const std::vector<EndStatisticData::PlayerInfo> &player_names)
{
    assert(player_names.size() == _max_num_rows);

    SetScale(false);

    for (unsigned i = 0; i < player_names.size(); ++i)
    {
        unsigned id = 10 + i;

        AddText(id, 
            (s.player_col_width/4), 
            (i+1) * s.y_grid, 
            player_names[i].name, COLOR_YELLOW, glArchivItem_Font::DF_LEFT, NormalFont);

        // TODO draw team marker
        // TODO draw nation marker
        // TODO draw alive/winner marker
    }

    SetScale(true);
}

void ctrlStatisticTable::AddColumn(unsigned col_idx, const std::vector<unsigned> &points)
{
    assert(col_idx > 0);
    assert(points.size() == _max_num_rows);

    SetScale(false);

    unsigned max_points = *std::max_element(points.begin(), points.end());

    for(unsigned short i = 0; i < points.size(); ++i)
    {
        unsigned id = 10 + col_idx * _max_num_rows + i;

        std::stringstream ss;

        ss << points[i];
        unsigned box_height = ScaleY(22);
        unsigned height_center_offset = ScaleY(40)/2 - (box_height/2) - 4; // TODO why 4?

        AddColorBar(id, 
            s.x_start + (col_idx - 1) * s.x_grid + (s.x_grid/2) - (s.col_width/2),  //was: (col_idx+1)
            s.y_start + (i + 1) * s.y_grid - height_center_offset,
            s.col_width, box_height, 
            TC_GREY,
            COLOR_YELLOW, NormalFont, COLORS[i], points[i], max_points);

        if (points[i] == max_points)
        {

            AddImage(100 + col_idx * _max_num_rows + i,
                s.x_start + (col_idx - 1) * s.x_grid + (s.x_grid/2) - (s.col_width/2) + 16,
                s.y_start + (i + 1) * s.y_grid - height_center_offset + 16,
                LOADER.GetImageN("io_new", 14), _("best in category"));

        }
    }
    _num_rows++;
    SetScale(true);
}

void ctrlStatisticTable::AddRow(const std::string player_name, const std::vector<int> &points)
{
    SetScale(false);
    unsigned base_id = 10 + (_num_rows + 1) * (points.size() + 5);

    AddText(base_id + 1, 
        (s.x_grid/2) - (s.col_width/2), 
        (_num_rows+1) * s.y_grid, 
        player_name, COLOR_YELLOW, glArchivItem_Font::DF_LEFT, NormalFont);

    for(unsigned short i = 0; i < points.size(); ++i)
    {
        std::stringstream ss;

        ss << points[i];

        /*AddText(base_id + 2 + i, 
            (i + 1) * s.x_grid + (s.x_grid/2),
            s.y_start + (_num_rows + 1) * s.y_grid, 
            ss.str(), COLOR_YELLOW, glArchivItem_Font::DF_CENTER, NormalFont);
*/
        unsigned fake_val = _num_rows;
        unsigned fake_max = _max_num_rows;

        unsigned box_height = ScaleY(22);
        unsigned height_center_offset = ScaleY(40)/2 - (box_height/2) - 4; // why 4?

        AddColorBar(base_id + 2 + i, 
            (i+1) * s.x_grid + (s.x_grid/2) - (s.col_width/2), 
            s.y_start + (_num_rows + 1) * s.y_grid - height_center_offset,
            s.col_width, box_height, 
            TC_GREY,
            COLOR_YELLOW, NormalFont, COLORS[_num_rows], fake_val, fake_max);
    }
    _num_rows++;
    SetScale(true);
}




bool ctrlStatisticTable::Draw_()
{
    SetScale(false);
    //Draw3D(GetX(), GetY(), width, height, tc, 2);

    // Die farbigen Zeilen malen
    for(unsigned i = 0; i < _max_num_rows; ++i)
    {
        // Rechtecke in Spielerfarbe malen mit entsprechender Transparenz
        // Font Height = 12
        unsigned box_height = ScaleY(40);
        unsigned height_center_offset = (box_height / 2) - (12/2);
        Window::DrawRectangle(GetX(), GetY() + (i+1) * s.y_grid - height_center_offset, width, box_height, (COLORS[i] & 0x00FFFFFF) | 0x40000000);
    }
    SetScale(true);
    DrawControls();



    return true;
}


void ctrlStatisticTable::Msg_ButtonClick(const unsigned int ctrl_id)
{
    //SortRows(ctrl_id - 1);
    if (ctrl_id > 1 && ctrl_id < _columns.size() + 1)
    {
        parent->Msg_StatisticGroupChange(id, ctrl_id - 1);
    }
}

bool ctrlStatisticTable::Msg_LeftDown(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_LeftDown, mc);
}

bool ctrlStatisticTable::Msg_MouseMove(const MouseCoords& mc)
{
    // ButtonMessages weiterleiten
    return RelayMouseMessage(&Window::Msg_MouseMove, mc);
}

bool ctrlStatisticTable::Msg_LeftUp(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_LeftUp, mc);
}