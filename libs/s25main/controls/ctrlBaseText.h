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

#ifndef ctrlBaseText_h__
#define ctrlBaseText_h__

#include <string>

class glArchivItem_Font;

/// Base class for controls containing a text
class ctrlBaseText
{
public:
    ctrlBaseText(std::string text, unsigned color, const glArchivItem_Font* font);

    void SetText(const std::string& text);
    const std::string& GetText() const { return text; }
    void SetFont(glArchivItem_Font* font);
    const glArchivItem_Font* GetFont() const { return font; }
    void SetTextColor(unsigned color) { color_ = color; }
    unsigned GetTextColor() const { return color_; }

protected:
    std::string text;
    unsigned color_;
    const glArchivItem_Font* font;
};

#endif // ctrlBaseText_h__
