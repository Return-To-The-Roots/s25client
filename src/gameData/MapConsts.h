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
#ifndef MAPCONSTS_H_INCLUDED
#define MAPCONSTS_H_INCLUDED

#pragma once

/// Width and height of a triangle.
/// Original: Water/Lave textures are 55x56 -> TR=55x28
/// We can only use 54 pixels as otherwise we would draw outside pixels in OGL, hence we adjust our triangle size
BOOST_CONSTEXPR_OR_CONST int TR_W = 54;
BOOST_CONSTEXPR_OR_CONST int TR_H = 27;

/// Number of pixels the node is adjusted in y per altitude step
BOOST_CONSTEXPR_OR_CONST int HEIGHT_FACTOR = 5;

#endif // !MAPCONSTS_H_INCLUDED
