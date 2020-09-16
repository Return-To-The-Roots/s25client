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

#include "Window.h"
class MouseCoords;

class ctrlScrollBar final : public Window
{
public:
    ctrlScrollBar(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, unsigned short button_height,
                  TextureColor tc, unsigned short pagesize);
    void Resize(const Extent& newSize) override;
    void SetScrollPos(unsigned short scroll_pos);
    void SetRange(unsigned short scroll_range);
    void SetPageSize(unsigned short pagesize);

    unsigned short GetPageSize() const { return pagesize; }
    unsigned short GetScrollPos() const { return scroll_pos; }
    /// Scrolls by the given distance (less than 0 = up, else down)
    void Scroll(int distance);

    bool Msg_LeftUp(const MouseCoords& mc) override;
    bool Msg_LeftDown(const MouseCoords& mc) override;
    bool Msg_MouseMove(const MouseCoords& mc) override;
    void Msg_ButtonClick(unsigned ctrl_id) override;

protected:
    void Draw_() override;

private:
    void UpdatePosFromSlider();
    void UpdateSliderFromPos();
    void RecalculateSizes();

    unsigned short button_height;
    TextureColor tc;
    /// (max) Number of elements/lines visible
    unsigned short pagesize;
    /// Total number of elements
    unsigned short scroll_range;
    /// Current element at top
    unsigned short scroll_pos;
    /// Height of the actual area in which we can move the slider
    unsigned short scroll_height;
    /// Size of the slider
    unsigned short sliderHeight;
    /// Position of the slider in the scroll area (in [0, scroll_height - sliderHeight] )
    unsigned short sliderPos;

    bool isMouseScrolling;
    int last_y;
};
