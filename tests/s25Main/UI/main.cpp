// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#define BOOST_TEST_MODULE RTTR_UI

#include <rttr/test/Fixture.hpp>
#include <boost/test/unit_test.hpp>

#if RTTR_HAS_VLD
#    include <vld.h>
#endif

struct Fixture : rttr::test::Fixture
{};

BOOST_GLOBAL_FIXTURE(Fixture);
