// Copyright (c) 2016 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "BaseFixture.hpp"

namespace rttr { namespace test {
    /// Init locale, logger and random numbers and rttr config
    struct Fixture : BaseFixture
    {
        Fixture();
    };
}} // namespace rttr::test
