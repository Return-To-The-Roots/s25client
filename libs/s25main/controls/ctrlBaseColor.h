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

#ifndef ctrlBaseColor_h__
#define ctrlBaseColor_h__

/// Base class for controls showing a color
class ctrlBaseColor
{
public:
    ctrlBaseColor() : color_(0) {}
    ctrlBaseColor(unsigned color) : color_(color) {}
    void SetColor(unsigned color) { color_ = color; }
    unsigned GetColor() const { return color_; }

protected:
    unsigned color_;
};

#endif // ctrlBaseColor_h__
