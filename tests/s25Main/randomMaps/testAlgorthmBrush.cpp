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

#include "rttrDefines.h" // IWYU pragma: keep
#include "randomMaps/algorithm/Brush.h"

#include <boost/test/unit_test.hpp>
#include <set>

struct BrushParams
{
    std::set<int> rsu;
    std::set<int> lsd;
    std::vector<int> rsu_;
    std::vector<int> lsd_;
    std::vector<int> dupes;
};

class BrushFunction
{
public:
    static void Collect(BrushParams& params, int index, bool rsu)
    {
        if (rsu)
        {
            if (params.rsu.insert(index).second)
            {
                params.rsu_.push_back(index);
            }
            else
            {
                params.dupes.push_back(index);
            }
        }
        else
        {
            if (params.lsd.insert(index).second)
            {
                params.lsd_.push_back(index);
            }
            else
            {
                params.dupes.push_back(index);
            }
        }
    }
};

BOOST_AUTO_TEST_SUITE(BrushTests)

BOOST_AUTO_TEST_CASE(TouchesAllRsuAndLsdTrianglesSpecifiedInBrushSettings)
{
    auto rsu = {Position(1,1)};
    auto lsd = {Position(1,0)};
    
    Brush<BrushParams> brush(BrushFunction::Collect);
    BrushParams params;
    BrushSettings settings(rsu, lsd);
    MapExtent size(2,2);
    
    brush.Paint(params, settings, Position(0,0), size);

    BOOST_REQUIRE(params.rsu_.size() == 1u);
    BOOST_REQUIRE(params.rsu_[0] == 3);

    BOOST_REQUIRE(params.lsd_.size() == 1u);
    BOOST_REQUIRE(params.lsd_[0] == 1);
}

BOOST_AUTO_TEST_CASE(TouchesAllRsuAndLsdTrianglesOnlyOnce)
{
    auto rsu = {Position(1,1)};
    auto lsd = {Position(1,0)};
    
    Brush<BrushParams> brush(BrushFunction::Collect);
    BrushParams params;
    BrushSettings settings(rsu, lsd);
    MapExtent size(2,2);
    
    brush.Paint(params, settings, Position(0,0), size);
    
    BOOST_REQUIRE(params.dupes.empty());
}

BOOST_AUTO_TEST_SUITE_END()
