// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Fixture.hpp"
#include <boost/filesystem/operations.hpp>
#include <RttrConfig.h>
#include <stdexcept>

namespace bfs = boost::filesystem;

rttr::test::Fixture::Fixture()
{
    if(!RTTRCONFIG.init())
        throw std::runtime_error("Could not init working directory. Misplaced binary?");
    if(!bfs::is_directory(RTTRCONFIG.expandPath("<RTTR_RTTR>")))
        throw std::runtime_error(RTTRCONFIG.expandPath("<RTTR_RTTR>").string()
                                 + " not found. Binary misplaced or RTTR folder not copied?");
}
