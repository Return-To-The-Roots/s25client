// Copyright (c) 2016 - 2018 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef testHelpers_h__
#define testHelpers_h__

namespace boost { namespace test_tools { namespace tt_detail {
    // Allow printing of pairs
    template<typename T, typename U>
    struct print_log_value<std::pair<T, U>>
    {
        void operator()(std::ostream& os, std::pair<T, U> const& v) { os << "(" << v.first << "," << v.second << ")"; }
    };
}}} // namespace boost::test_tools::tt_detail

template<typename T1, typename T2>
inline boost::test_tools::predicate_result testCmp(const char* cmp, const T1& l, const T2& r, bool equal)
{
    if((l == r) != equal)
    {
        boost::test_tools::predicate_result res(false);
        res.message() << cmp << " [" << l << (equal ? "!=" : "==") << r << "]";
        return res;
    }

    return true;
}

#define RTTR_REQUIRE_EQUAL_COLLECTIONS(Col1, Col2) BOOST_REQUIRE_EQUAL_COLLECTIONS(Col1.begin(), Col1.end(), Col2.begin(), Col2.end())

#define RTTR_REQUIRE_EQUAL_MSG(L, R, MSG)                                              \
    do                                                                                 \
    {                                                                                  \
        boost::test_tools::predicate_result res = testCmp(#L "==" #R, (L), (R), true); \
        if(!res)                                                                       \
            res.message() << MSG;                                                      \
        BOOST_REQUIRE(res);                                                            \
    } while(false)

#define RTTR_REQUIRE_NE_MSG(L, R, MSG)                                                  \
    do                                                                                  \
    {                                                                                   \
        boost::test_tools::predicate_result res = testCmp(#L "!=" #R, (L), (R), false); \
        if(!res)                                                                        \
            res.message() << MSG;                                                       \
        BOOST_REQUIRE(res);                                                             \
    } while(false)

#endif // testHelpers_h__
