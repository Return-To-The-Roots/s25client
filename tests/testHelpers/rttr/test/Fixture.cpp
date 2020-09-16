// Copyright (c) 2016 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "Fixture.hpp"
#include <boost/filesystem/operations.hpp>
#include <RttrConfig.h>
#include <stdexcept>

namespace bfs = boost::filesystem;

rttr::test::Fixture::Fixture()
{
    if(!RTTRCONFIG.Init())
        throw std::runtime_error("Could not init working directory. Misplaced binary?");
    if(!bfs::is_directory(RTTRCONFIG.ExpandPath("<RTTR_RTTR>")))
        throw std::runtime_error(RTTRCONFIG.ExpandPath("<RTTR_RTTR>").string()
                                 + " not found. Binary misplaced or RTTR folder not copied?");
}

rttr::test::Fixture::~Fixture() = default;
