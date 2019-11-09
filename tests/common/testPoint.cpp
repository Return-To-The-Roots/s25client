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

#include "Point.h"
#include "PointOutput.h"
#include <rttr/test/random.hpp>
#include <boost/mpl/list.hpp>
#include <boost/test/unit_test.hpp>
#include <type_traits>

using boost::test_tools::tolerance;
using rttr::test::randomPoint;
using rttr::test::randomValue;

using SignedTypes = boost::mpl::list<int8_t, int16_t, int32_t, int64_t, float, double>;
// Custom trait to support float/double
template<typename T>
using make_unsigned_t = typename std::conditional_t<std::is_floating_point<T>::value, std::common_type<T>, std::make_unsigned<T>>::type;

template<typename T>
constexpr T abs(T val)
{
    return val < 0 ? -val : val;
}

BOOST_AUTO_TEST_CASE_TEMPLATE(negate_point, T, SignedTypes)
{
    using U = make_unsigned_t<T>;
    using SignedPoint = Point<T>;
    using UnsignedPoint = Point<U>;

    auto x = randomValue<T>();
    auto y = randomValue<T>();
    const SignedPoint pt(x, y);
    BOOST_TEST(-pt == SignedPoint(-x, -y));
    x = abs(x);
    y = abs(y);
    const UnsignedPoint pt2(x, y);
    const auto negated = -pt2;
    static_assert(std::is_same<decltype(negated.x), T>::value, "Result must be signed");
    BOOST_TEST(negated == SignedPoint(-x, -y));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(add_points, T, SignedTypes)
{
    using U = make_unsigned_t<T>;
    using SignedPoint = Point<T>;
    using UnsignedPoint = Point<U>;

    const auto x = randomValue<T>(-100, 100);
    const auto y = randomValue<T>(-100, 100);
    SignedPoint pt(x, y);
    const auto x2 = randomValue<U>(1, 100);
    const auto y2 = randomValue<U>(1, 100);
    UnsignedPoint pt2(x2, y2);

    const auto result = pt + pt2;
    static_assert(std::is_same<decltype(result.x), T>::value, "Result must be signed");
    BOOST_TEST(result == SignedPoint(x + x2, y + y2));

    const auto resultSigned = pt + SignedPoint(pt2);
    static_assert(std::is_same<decltype(resultSigned.x), T>::value, "Result must be signed");
    BOOST_TEST(resultSigned == result);

    const auto resultUnsigned = UnsignedPoint(pt) + pt2;
    static_assert(std::is_same<decltype(resultUnsigned.x), U>::value, "Result must be unsigned");
    BOOST_TEST(resultUnsigned == UnsignedPoint(result));

    pt += SignedPoint(pt2);
    BOOST_TEST(pt == result);

    pt2 += UnsignedPoint(x, y);
    BOOST_TEST(pt2 == resultUnsigned);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(subtract_points, T, SignedTypes)
{
    using U = make_unsigned_t<T>;
    using SignedPoint = Point<T>;
    using UnsignedPoint = Point<U>;

    const auto x = randomValue<T>(-100, 100);
    const auto y = randomValue<T>(-100, 100);
    SignedPoint pt(x, y);
    const auto x2 = randomValue<U>(1, 100);
    const auto y2 = randomValue<U>(1, 100);
    const UnsignedPoint pt2(x2, y2);

    const auto result = pt - pt2;
    static_assert(std::is_same<decltype(result.x), T>::value, "Result must be signed");
    BOOST_TEST(result == SignedPoint(x - x2, y - y2));

    const auto resultSigned = pt - SignedPoint(pt2);
    static_assert(std::is_same<decltype(resultSigned.x), T>::value, "Result must be signed");
    BOOST_TEST(resultSigned == result);

    const auto x3 = randomValue<U>(x2);
    const auto y3 = randomValue<U>(y2);
    UnsignedPoint pt3(x3, y3);
    const auto resultUnsigned = pt3 - pt2;
    static_assert(std::is_same<decltype(resultUnsigned.x), U>::value, "Result must be unsigned");
    BOOST_TEST(resultUnsigned == UnsignedPoint(x3 - x2, y3 - y2));

    pt -= SignedPoint(pt2);
    BOOST_TEST(pt == result);

    pt3 -= pt2;
    BOOST_TEST(pt3 == resultUnsigned);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(multiply_points, T, SignedTypes)
{
    using U = make_unsigned_t<T>;
    using SignedPoint = Point<T>;
    using UnsignedPoint = Point<U>;

    const auto x = randomValue<T>(-100, 100);
    const auto y = randomValue<T>(-100, 100);
    SignedPoint pt(x, y);
    const auto x2 = randomValue<U>(1, 100);
    const auto y2 = randomValue<U>(1, 100);
    UnsignedPoint pt2(x2, y2);

    const auto result = pt * pt2;
    static_assert(std::is_same<decltype(result.x), T>::value, "Result must be signed");
    BOOST_TEST(result == SignedPoint(x * x2, y * y2));

    const auto resultSigned = pt * SignedPoint(pt2);
    static_assert(std::is_same<decltype(resultSigned.x), T>::value, "Result must be signed");
    BOOST_TEST(resultSigned == result);

    const auto resultUnsigned = UnsignedPoint(pt) * pt2;
    static_assert(std::is_same<decltype(resultUnsigned.x), U>::value, "Result must be unsigned");
    BOOST_TEST(resultUnsigned == UnsignedPoint(result));

    pt *= SignedPoint(pt2);
    BOOST_TEST(pt == result);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(scale_point, T, SignedTypes)
{
    using U = make_unsigned_t<T>;
    using SignedPoint = Point<T>;
    using UnsignedPoint = Point<U>;

    auto x = randomValue<T>(-100, 100);
    auto y = randomValue<T>(-100, 100);
    SignedPoint pt(x, y);
    const auto scale = randomValue<U>(1, 100);

    const auto result = pt * scale;
    static_assert(std::is_same<decltype(result.x), T>::value, "Result must be signed");
    BOOST_TEST(result == pt * SignedPoint::all(scale));

    const auto resultSigned = pt * T(scale);
    static_assert(std::is_same<decltype(resultSigned.x), T>::value, "Result must be signed");
    BOOST_TEST(resultSigned == result);

    x = abs(x);
    y = abs(y);
    UnsignedPoint pt2(x, y);
    const auto resultUnsigned = pt2 * scale;
    static_assert(std::is_same<decltype(resultUnsigned.x), U>::value, "Result must be unsigned");
    BOOST_TEST(resultUnsigned == pt2 * UnsignedPoint::all(scale));

    pt *= scale;
    BOOST_TEST(pt == result);

    pt2 *= scale;
    BOOST_TEST(pt2 == resultUnsigned);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(divide_points, T, SignedTypes)
{
    using U = make_unsigned_t<T>;
    using SignedPoint = Point<T>;
    using UnsignedPoint = Point<U>;

    const auto x = randomValue<T>();
    const auto y = randomValue<T>();
    SignedPoint pt(x, y);
    const auto x2 = randomValue<U>(1, 100);
    const auto y2 = randomValue<U>(1, 100);
    UnsignedPoint pt2(x2, y2);

    const auto result = pt / pt2;
    static_assert(std::is_same<decltype(result.x), T>::value, "Result must be signed");
    BOOST_TEST(result == SignedPoint(x / static_cast<T>(x2), y / static_cast<T>(y2)));

    const auto resultSigned = pt / SignedPoint(pt2);
    static_assert(std::is_same<decltype(resultSigned.x), T>::value, "Result must be signed");
    BOOST_TEST(resultSigned == result);

    const auto x3 = randomValue<U>(x2);
    const auto y3 = randomValue<U>(y2);
    UnsignedPoint pt3(x3, y3);
    const auto resultUnsigned = pt3 / pt2;
    static_assert(std::is_same<decltype(resultUnsigned.x), U>::value, "Result must be unsigned");
    BOOST_TEST(resultUnsigned == UnsignedPoint(x3 / x2, y3 / y2));

    pt /= SignedPoint(pt2);
    BOOST_TEST(pt == result);

    pt3 /= pt2;
    BOOST_TEST(pt3 == resultUnsigned);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(unscale_point, T, SignedTypes)
{
    using U = make_unsigned_t<T>;
    using SignedPoint = Point<T>;
    using UnsignedPoint = Point<U>;

    auto x = randomValue<T>();
    auto y = randomValue<T>();
    SignedPoint pt(x, y);
    const auto scale = randomValue<U>(1, 100);

    constexpr auto epsilon = std::conditional_t<std::is_same<T, float>::value, float, double>(0.001);

    const auto result = pt / scale;
    static_assert(std::is_same<decltype(result.x), T>::value, "Result must be signed");
    BOOST_TEST(result == pt / SignedPoint::all(scale), epsilon % tolerance());

    const auto resultSigned = pt / T(scale);
    static_assert(std::is_same<decltype(resultSigned.x), T>::value, "Result must be signed");
    BOOST_TEST(resultSigned == result, epsilon % tolerance());

    x = abs(x);
    y = abs(y);
    UnsignedPoint pt2(x, y);
    const auto resultUnsigned = pt2 / scale;
    static_assert(std::is_same<decltype(resultUnsigned.x), U>::value, "Result must be unsigned");
    BOOST_TEST(resultUnsigned == pt2 / UnsignedPoint::all(scale), epsilon % tolerance());

    pt /= scale;
    BOOST_TEST(pt == result, epsilon % tolerance());

    pt2 /= scale;
    BOOST_TEST(pt2 == resultUnsigned, epsilon % tolerance());

    x = randomValue<T>(1, 100);
    y = randomValue<T>(1, 100);
    const auto scale2 = randomValue<T>();
    pt = SignedPoint(x, y);
    const auto resultPt = scale2 / pt;
    static_assert(std::is_same<decltype(resultPt.x), T>::value, "Result must be signed");
    BOOST_TEST(resultPt == SignedPoint::all(scale2) / pt, epsilon % tolerance());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(symetric_operators, T, SignedTypes)
{
    using U = make_unsigned_t<T>;
    using SignedPoint = Point<T>;
    using UnsignedPoint = Point<U>;

    const SignedPoint p1 = randomPoint<SignedPoint>(-100, 100);
    const SignedPoint p2 = randomPoint<SignedPoint>(-100, 100);
    const auto scale = randomValue<T>(-10, 10);
    BOOST_TEST(p1 + p2 == p2 + p1);
    BOOST_TEST(p1 * p2 == p2 * p1);
    BOOST_TEST(p1 * scale == scale * p1);

    const UnsignedPoint p3 = randomPoint<UnsignedPoint>(0, 100);
    const UnsignedPoint p4 = randomPoint<UnsignedPoint>(0, 100);
    const auto scale2 = randomValue<U>(0, 10);
    BOOST_TEST(p3 + p4 == p4 + p3);
    BOOST_TEST(p3 * p4 == p4 * p3);
    BOOST_TEST(p3 * scale2 == scale2 * p3);
}

BOOST_AUTO_TEST_CASE(ProdOfComponents)
{
    Point<uint16_t> pt(256, 256);
    BOOST_TEST(prodOfComponents(pt) == 256u * 256u);
    Point<int16_t> ptI(256, 256);
    BOOST_TEST(prodOfComponents(ptI) == 256 * 256);
    Point<float> ptF(256.5, 256.5);
    BOOST_TEST(prodOfComponents(ptF) == 256.5f * 256.5f);
}
