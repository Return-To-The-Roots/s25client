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

#ifndef LandscapeType_h__
#define LandscapeType_h__

enum LandscapeType
{
    LT_GREENLAND = 0,
    LT_WASTELAND,
    LT_WINTERWORLD
};

// Keep this in sync with LandscapeType
static const unsigned char LT_COUNT = LT_WINTERWORLD + 1;

#endif // LandscapeType_h__
