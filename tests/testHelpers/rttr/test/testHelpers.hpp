// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/test/unit_test.hpp>
#include <utility>

namespace boost::test_tools::tt_detail {
    // Allow printing of pairs
    template<typename T, typename U>
    struct print_log_value<std::pair<T, U>>
    {
        void operator()(std::ostream& os, std::pair<T, U> const& v) { os << "(" << v.first << "," << v.second << ")"; }
    };
} // namespace boost::test_tools::tt_detail

/// Check that an exception of the given type is thrown and it contains the message
#define RTTR_CHECK_EXCEPTION_MSG(S, E, MSG)         \
    BOOST_CHECK_EXCEPTION(S, E, [](const E& e) {    \
        BOOST_TEST(std::string(e.what()) == (MSG)); \
        return std::string(e.what()) == (MSG);      \
    })
