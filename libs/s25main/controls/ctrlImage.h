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
#ifndef CTRLIMAGE_H_INCLUDED
#define CTRLIMAGE_H_INCLUDED

#pragma once

#include "Window.h"
#include "controls/ctrlBaseImage.h"
#include "controls/ctrlBaseTooltip.h"

class MouseCoords;
class ITexture;

class ctrlImage : public Window, public ctrlBaseTooltip, public ctrlBaseImage
{
public:
    ctrlImage(Window* parent, unsigned id, const DrawPoint& pos, ITexture* image, const std::string& tooltip);
    ~ctrlImage() override;

    bool Msg_MouseMove(const MouseCoords& mc) override;

protected:
    void Draw_() override;
};

#endif // !CTRLIMAGE_H_INCLUDED
