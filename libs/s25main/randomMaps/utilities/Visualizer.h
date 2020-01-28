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

#ifndef Visualizer_h__
#define Visualizer_h__

#include "randomMaps/terrain/TextureType.h"
#include "randomMaps/elevation/HeightSettings.h"
#include "randomMaps/utilities/Bitmap.h"
#include <string>
#include <set>

typedef std::vector<unsigned char> UCharVec;
typedef std::vector<double> DoubleVec;
typedef std::vector<bool> BoolVec;
typedef std::set<int> IntSet;

class Visualizer
{
private:
    static unsigned char RedFromTexture(TextureType texture);
    static unsigned char GreenFromTexture(TextureType texture);
    static unsigned char BlueFromTexture(TextureType texture);

public:
    static void Write(const Bitmap& bitmap, std::string filename);
    static Bitmap From(const DoubleVec& points, int width, int height);
    static Bitmap From(const UCharVec& points, int width, int height);
    static Bitmap From(const BoolVec& points, int width, int height);
    static Bitmap From(const UCharVec& z, const BoolVec& water, int width, int height);
    static Bitmap From(const UCharVec& z, const std::vector<TextureType>& texture,
                       const HeightSettings& settings, int width, int height);
    static Bitmap From(const IntSet& rsu, const IntSet& lsd, int width, int height);
};

#endif // Visualizer_h__
