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
#include "defines.h" // IWYU pragma: keep
#include "ctrlMinimap.h"
#include "gameData/MinimapConsts.h"
#include "Minimap.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

ctrlMinimap::ctrlMinimap( Window* parent,
                          const unsigned int id,
                          const unsigned short x,
                          const unsigned short y,
                          const unsigned short width,
                          const unsigned short height,
                          const unsigned short padding_x,
                          const unsigned short padding_y,
                          const unsigned short map_width,
                          const unsigned short map_height)
    : Window(x, y, id, parent, width, height), padding_x(padding_x), padding_y(padding_y), mapWidth_(map_width), mapHeight_(map_height)
{
    SetDisplaySize(width, height, map_width, map_height);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Größe ändern
 *
 *  @author Divan
 */
void ctrlMinimap::Resize_(unsigned short width, unsigned short height)
{
    SetDisplaySize(width, height, mapWidth_, mapHeight_);
}

void ctrlMinimap::SetDisplaySize(const unsigned short width, const unsigned short height, const unsigned short map_width, const unsigned short map_height)
{
    this->width_ = width;
    this->height_ = height;

    this->mapWidth_ = map_width;
    this->mapHeight_ = map_height;

    unsigned short scaled_map_width = static_cast<unsigned short>(map_width * MINIMAP_SCALE_X);
    double x_scale = double(scaled_map_width) / double(width - padding_x * 2);
    double y_scale = double(map_height) / double(height - padding_y * 2);

    bool scale_width = false;

    scale_width = x_scale <= y_scale;

    RTTR_Assert(map_height != 0);
    RTTR_Assert(scaled_map_width != 0);

    if(scale_width)
    {
        height_show = height - padding_y * 2;
        width_show  = (scaled_map_width * height_show / map_height) & 0xFFFF; // to mask unsigned to unsigned short (VS debugger crying)
    }
    else
    {
        width_show  = width - padding_x * 2;
        height_show = map_height * width_show / scaled_map_width;
    }
}

void ctrlMinimap::DrawMap(Minimap& map)
{
    // Map ansich zeichnen
    map.Draw(GetX() + GetLeft(), GetY() + GetTop(), width_show, height_show);
}

void ctrlMinimap::RemoveBoundingBox(const unsigned short width_min, const unsigned short height_min)
{
    width_  = max<unsigned short>( width_show + padding_x * 2,  width_min);
    height_ = max<unsigned short>(height_show + padding_y * 2, height_min);
}
