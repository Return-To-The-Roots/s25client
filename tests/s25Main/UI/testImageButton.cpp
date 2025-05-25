// Copyright (C) 2005 - 2022 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "RectOutput.h"
#include "controls/ctrlImageButton.h"
#include "ogl/ITexture.h"
#include "uiHelper/uiHelpers.hpp"
#include <rttr/test/random.hpp>
#include <turtle/mock.hpp>
#include <boost/test/unit_test.hpp>

using namespace rttr::test;

namespace {
MOCK_BASE_CLASS(TestTexture, ITexture)
{
public:
    TestTexture(Extent size, Position origin) : size_(size), origin_(origin) {}
    Position GetOrigin() const override
    {
        return origin_;
    }
    Extent GetSize() const override
    {
        return size_;
    }
    MOCK_NON_CONST_METHOD(DrawFull, 2) // LCOV_EXCL_LINE
    MOCK_NON_CONST_METHOD(Draw, 3)

private:
    Extent size_;
    Position origin_;
};
} // namespace

BOOST_FIXTURE_TEST_SUITE(ImageButton, uiHelper::Fixture)

BOOST_AUTO_TEST_CASE(DrawSmallerImageThanButton)
{
    Window wnd(nullptr, randomValue<unsigned>(), DrawPoint::all(0));

    {
        TestTexture img(Extent(20, 10), Position(0, 0));
        ctrlImageButton btn(&wnd, randomValue<unsigned>(), DrawPoint(10, 5), Extent(40, 30), TextureColor::Green1, &img,
                            "");
        MOCK_EXPECT(img.Draw).once().with(Rect(20, 15, 20, 10), Rect(0, 0, 20, 10), 0xFFFFFFFFu);
        btn.Draw();
    }

    {
        TestTexture img(Extent(20, 10), Position(13, 2));
        ctrlImageButton btn(&wnd, randomValue<unsigned>(), DrawPoint(10, 5), Extent(40, 30), TextureColor::Green1, &img,
                            "");
        MOCK_EXPECT(img.Draw).once().with(Rect(33, 17, 20, 10), Rect(0, 0, 20, 10), 0xFFFFFFFFu);
        btn.Draw();
    }
}

BOOST_AUTO_TEST_CASE(DrawLargerImageThanButton)
{
    Window wnd(nullptr, randomValue<unsigned>(), DrawPoint::all(0));

    {
        TestTexture img(Extent(100, 80), Position(0, 0));
        ctrlImageButton btn(&wnd, randomValue<unsigned>(), DrawPoint(10, 20), Extent(40, 30), TextureColor::Green1,
                            &img, "");
        // Note that the image's center part is drawn, shrinked by 2px from each side to prevent dirtying 3d button's
        // ridge
        MOCK_EXPECT(img.Draw).once().with(Rect(12, 22, 36, 26), Rect(32, 27, 36, 26), 0xFFFFFFFFu);
        btn.Draw();
    }

    {
        TestTexture img(Extent(100, 80), Position(68, 32));
        ctrlImageButton btn(&wnd, randomValue<unsigned>(), DrawPoint(10, 20), Extent(40, 30), TextureColor::Green1,
                            &img, "");
        // Note that the image's center part is drawn, shrinked by 2px from each side to prevent dirtying 3d button's
        // ridge
        MOCK_EXPECT(img.Draw).once().with(Rect(80, 54, 36, 26), Rect(32, 27, 36, 26), 0xFFFFFFFFu);
        btn.Draw();
    }
}

BOOST_AUTO_TEST_SUITE_END()
