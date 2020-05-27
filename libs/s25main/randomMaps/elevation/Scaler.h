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

#ifndef Scaler_h__
#define Scaler_h__

#include "randomMaps/elevation/HeightSettings.h"
#include <vector>

class Scaler
{
private:
    HeightSettings height_;
    
public:
    Scaler(const HeightSettings& height) : height_(height) {}    
    void Scale(std::vector<unsigned char>& heightMap);
    std::vector<unsigned char> Scale(const std::vector<int>& map);
};

#endif // Scaler_h__
