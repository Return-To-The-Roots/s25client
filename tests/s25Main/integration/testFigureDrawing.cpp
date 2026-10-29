// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GameEvent.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "buildings/nobUsual.h"
#include "factories/BuildingFactory.h"
#include "figures/nofCarrier.h"
#include "figures/nofFisher.h"
#include "figures/nofHunter.h"
#include "figures/nofPassiveWorker.h"
#include "figures/nofScout_Free.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "ogl/glArchivItem_Bob.h"
#include "uiHelper/uiHelpers.hpp"
#include "worldFixtures/CreateEmptyWorld.h"
#include "worldFixtures/WorldFixture.h"
#include "nodeObjs/noRoadNode.h"
#include <libsiedler2/ArchivItem_Palette.h>
#include <libsiedler2/PixelBufferBGRA.h>
#include <rttr/test/stubFunction.hpp>
#include <s25util/colors.h>
#include <s25util/warningSuppression.h>
#include <glad/glad.h>
#include <boost/test/unit_test.hpp>
#include <algorithm>
#include <vector>

namespace rttrOglMock3 {
RTTR_IGNORE_DIAGNOSTIC("-Wmissing-declarations")

bool colorArrayEnabled = false;
unsigned curColor = 0;
const GLubyte* colorPointer = nullptr;
/// Color (of the first vertex) of each draw call in order of drawing
std::vector<unsigned> drawColors;

void APIENTRY glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
    curColor = MakeColor(alpha, red, green, blue);
}

void APIENTRY glEnableClientState(GLenum cap)
{
    if(cap == GL_COLOR_ARRAY)
        colorArrayEnabled = true;
}

void APIENTRY glDisableClientState(GLenum cap)
{
    if(cap == GL_COLOR_ARRAY)
        colorArrayEnabled = false;
}

void APIENTRY glColorPointer(GLint, GLenum, GLsizei, const void* pointer)
{
    colorPointer = static_cast<const GLubyte*>(pointer);
}

void APIENTRY glDrawArrays(GLenum, GLint first, GLsizei)
{
    if(colorArrayEnabled)
    {
        const GLubyte* color = &colorPointer[first * 4];
        drawColors.push_back(MakeColor(color[3], color[0], color[1], color[2]));
    } else
        drawColors.push_back(curColor);
}

RTTR_POP_DIAGNOSTIC
} // namespace rttrOglMock3

BOOST_AUTO_TEST_SUITE(FigureDrawing)

namespace {
using EmptyWorldFixture1P = WorldFixture<CreateEmptyWorld, 1>;

std::unique_ptr<glArchivItem_Bitmap_Player> createPlayerBmp()
{
    const auto* pal = LOADER.GetPaletteN("colors");
    libsiedler2::PixelBufferBGRA buffer(1, 1);
    buffer.set(0, 0, pal->get(1));
    auto bmp = std::make_unique<glArchivItem_Bitmap_Player>();
    bmp->create(buffer, pal);
    return bmp;
}

struct WalkingFigureFixture : EmptyWorldFixture1P
{
    noFigure* figure;

    WalkingFigureFixture()
    {
        uiHelper::initGUITests();
        LOADER.LoadDummyMapFiles();
        const MapPoint hqFlagPos = world.GetNeighbour(world.GetPlayer(0).GetHQPos(), Direction::SouthEast);
        figure = &world.AddFigure(
          hqFlagPos, std::make_unique<nofScout_Free>(hqFlagPos, 0, world.GetSpecObj<noRoadNode>(hqFlagPos)));
        figure->ActAtFirst();
        BOOST_TEST_REQUIRE(figure->IsMoving());
        rttrOglMock3::drawColors.clear();
    }
};

// Set up a working animation without advancing the production simulation.
template<class Worker>
struct DrawingWorker : Worker
{
    using Worker::Worker;

    void setAnimation(nofBuildingWorker::State state, const GameEvent& event)
    {
        this->state = state;
        this->current_ev = &event;
    }
};

struct DrawingCarrier : nofCarrier
{
    DrawingCarrier(MapPoint pos) : nofCarrier(CarrierType::Normal, pos, 0, nullptr, nullptr)
    {
        // Waiting at a blocked node also plays the idle animation. Its initial start GF is zero.
        waiting_for_free_node = true;
    }
};

void loadPlayerImages(const ResourceId& file, unsigned first, unsigned last)
{
    // Ensure the loader has an entry even for archives not provided by the GUI fixture.
    LOADER.GetPlayerImage(file, 0);
    auto& archive = LOADER.GetArchive(file);
    if(archive.size() <= last)
        archive.alloc_inc(last + 1 - archive.size());
    for(unsigned i = first; i <= last; ++i)
        archive.set(i, createPlayerBmp());
}

#define RTTR_STUB_DRAWING_FUNCTIONS()                                             \
    RTTR_STUB_FUNCTION(glColor4ub, rttrOglMock3::glColor4ub);                     \
    RTTR_STUB_FUNCTION(glEnableClientState, rttrOglMock3::glEnableClientState);   \
    RTTR_STUB_FUNCTION(glDisableClientState, rttrOglMock3::glDisableClientState); \
    RTTR_STUB_FUNCTION(glColorPointer, rttrOglMock3::glColorPointer);             \
    RTTR_STUB_FUNCTION(glDrawArrays, rttrOglMock3::glDrawArrays)

/// The shadow must be drawn first and only once so it does not darken the figure (like in S2)
void checkShadowDrawnFirst(const size_t numFigureImages)
{
    const auto& drawColors = rttrOglMock3::drawColors;
    BOOST_TEST_REQUIRE(drawColors.size() == 1u + numFigureImages);
    BOOST_TEST(drawColors.front() == COLOR_SHADOW);
    BOOST_TEST(std::count(drawColors.begin(), drawColors.end(), COLOR_SHADOW) == 1);
}
} // namespace

BOOST_FIXTURE_TEST_CASE(WalkingBobDrawsShadowUnderFigure, WalkingFigureFixture)
{
    // A bob sprite is a body image plus an optional overlay image (e.g. the job specific head/tool) on top.
    // Create the slim body images and link all sprites of item 0 to overlay 0 (default link data).
    // The overlay image itself is not set yet, so only the body is drawn.
    constexpr unsigned numBodyImages = 2 * 6 * 8;
    constexpr unsigned numLinksPerItem = 8 * 2 * 6;
    glArchivItem_Bob bob;
    bob.alloc(numBodyImages);
    for(unsigned i = 0; i < numBodyImages / 2; i++)
        bob.set(i, createPlayerBmp());
    bob.mergeLinks({{numLinksPerItem - 1, {}}});

    RTTR_STUB_DRAWING_FUNCTIONS();
    figure->DrawWalking(DrawPoint(0, 0), &bob, 0, false);
    checkShadowDrawnFirst(1);

    // Now add the image for overlay 0 (stored after the body images): Body and overlay must both be above the shadow
    bob.alloc_inc(1);
    bob.set(numBodyImages, createPlayerBmp());
    rttrOglMock3::drawColors.clear();
    figure->DrawWalking(DrawPoint(0, 0), &bob, 0, false);
    checkShadowDrawnFirst(2);
}

BOOST_FIXTURE_TEST_CASE(WalkingNullBobDrawsOnlyShadow, WalkingFigureFixture)
{
    RTTR_STUB_DRAWING_FUNCTIONS();
    figure->DrawWalking(DrawPoint(0, 0), nullptr, 0, false);
    checkShadowDrawnFirst(0);
}

BOOST_FIXTURE_TEST_CASE(WalkingLstDrawsShadowUnderFigure, WalkingFigureFixture)
{
    // Use an otherwise unused range of the map archive for the 6 directions * 8 animation steps
    constexpr unsigned imgSetIndex = 1000;
    constexpr unsigned numWalkImages = 6 * 8;
    libsiedler2::Archiv& archive = LOADER.GetArchive("map_0_z");
    if(archive.size() < imgSetIndex + numWalkImages)
        archive.alloc_inc(imgSetIndex + numWalkImages - archive.size());
    for(unsigned i = 0; i < numWalkImages; i++)
        archive.set(imgSetIndex + i, createPlayerBmp());

    RTTR_STUB_DRAWING_FUNCTIONS();
    figure->DrawWalking(DrawPoint(0, 0), "map_0_z", imgSetIndex);
    checkShadowDrawnFirst(1);
}

BOOST_FIXTURE_TEST_CASE(ShootingHunterDrawsShadowUnderFigure, WalkingFigureFixture)
{
    auto* workplace = static_cast<nobUsual*>(
      BuildingFactory::CreateBuilding(world, BuildingType::Hunter, MapPoint(2, 2), 0, Nation::Romans));
    DrawingWorker<nofHunter> hunter(workplace->GetPos(), 0, workplace);
    const GameEvent event(0, &hunter, 0, 100, 0);
    hunter.setAnimation(nofBuildingWorker::State::HunterShooting, event);
    loadPlayerImages("rom_bobs", 1686, 1733);

    RTTR_STUB_DRAWING_FUNCTIONS();
    hunter.Draw(DrawPoint(0, 0));
    checkShadowDrawnFirst(1);
}

BOOST_FIXTURE_TEST_CASE(WorkingFisherDrawsShadowUnderFigure, WalkingFigureFixture)
{
    auto* workplace = static_cast<nobUsual*>(
      BuildingFactory::CreateBuilding(world, BuildingType::Fishery, MapPoint(2, 2), 0, Nation::Romans));
    DrawingWorker<nofFisher> fisher(workplace->GetPos(), 0, workplace);
    const GameEvent event(0, &fisher, 0, 100, 0);
    fisher.setAnimation(nofBuildingWorker::State::Work, event);
    loadPlayerImages("rom_bobs", 108, 131);
    loadPlayerImages("rom_bobs", 1566, 1589);

    RTTR_STUB_DRAWING_FUNCTIONS();
    fisher.Draw(DrawPoint(0, 0));
    checkShadowDrawnFirst(1);
}

BOOST_FIXTURE_TEST_CASE(IdleCarrierDrawsShadowUnderFigure, WalkingFigureFixture)
{
    // First frame of animation zero for either randomly selected body size.
    loadPlayerImages("rom_bobs", 1726, 1726);
    loadPlayerImages("rom_bobs", 1745, 1745);
    DrawingCarrier carrier(figure->GetPos());

    RTTR_STUB_DRAWING_FUNCTIONS();
    carrier.Draw(DrawPoint(0, 0));
    checkShadowDrawnFirst(1);
}

BOOST_FIXTURE_TEST_CASE(WalkingPackDonkeyDrawsShadowUnderFigure, WalkingFigureFixture)
{
    loadPlayerImages("map_0_z", 2000, 2050);
    nofPassiveWorker donkey(Job::PackDonkey, figure->GetPos(), 0, nullptr);
    donkey.StartMoving(Direction::East, 20);

    RTTR_STUB_DRAWING_FUNCTIONS();
    donkey.DrawWalking(DrawPoint(0, 0));
    checkShadowDrawnFirst(1);
}

BOOST_AUTO_TEST_SUITE_END()
