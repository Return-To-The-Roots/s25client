// Copyright (c) 2017 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "mapGenerator/BrushSettings.h"

namespace rttr {
namespace mapGenerator {

BrushSettings::BrushSettings(std::vector<Position> rsuArg, std::vector<Position> lsdArg)
    : rsu(std::move(rsuArg)), lsd(std::move(lsdArg))
{
}

BrushSettings CreateBrushSettings(const std::vector<int>& rsuArg, const std::vector<int>& lsdArg)
{
    int rsuCount = rsuArg.size() / 2;
    std::vector<Position> rsu(rsuCount);
    for (int i = 0; i < rsuCount; ++i)
    {
        rsu[i] = Position(rsuArg[i*2], rsuArg[i*2+1]);
    }
    
    int lsdCount = lsdArg.size() / 2;
    std::vector<Position> lsd(lsdCount);
    for (int i = 0; i < lsdCount; ++i)
    {
        lsd[i] = Position(lsdArg[i*2], lsdArg[i*2+1]);
    }
    
    return BrushSettings(rsu, lsd);
}


}}
