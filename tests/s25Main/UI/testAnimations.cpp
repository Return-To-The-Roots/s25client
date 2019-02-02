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

#include "rttrDefines.h" // IWYU pragma: keep
#include "Loader.h"
#include "PointOutput.h"
#include "Window.h"
#include "WindowManager.h"
#include "animation/AnimationManager.h"
#include "animation/BlinkButtonAnim.h"
#include "animation/MoveAnimation.h"
#include "animation/ToggleAnimation.h"
#include "controls/ctrlButton.h"
#include "desktops/Desktop.h"
#include "drivers/VideoDriverWrapper.h"
#include "helpers/containerUtils.h"
#include "mockupDrivers/MockupVideoDriver.h"
#include "uiHelper/uiHelpers.hpp"
#include <boost/test/unit_test.hpp>

namespace {
typedef boost::test_tools::predicate_result PredRes;
struct TestAnimation;

struct TestWindow : public Window
{
    TestWindow(const DrawPoint& position, unsigned id, Window* parent, const Extent& size) : Window(parent, id, position, size) {}

protected:
    void Draw_() override {}
};

struct WindowFixture
{
    TestWindow wnd;
    AnimationManager& animMgr;
    ctrlButton *bt, *bt2;
    bool animFinished;
    double lastNextFramepartTime;
    unsigned lastFrame;
    WindowFixture() : wnd(DrawPoint(0, 0), 0, nullptr, Extent(800, 600)), animMgr(wnd.GetAnimationManager()), animFinished(false)
    {
        bt = wnd.AddTextButton(0, DrawPoint(10, 20), Extent(100, 20), TC_RED1, "Test", NormalFont);
        bt2 = wnd.AddTextButton(1, DrawPoint(10, 40), Extent(100, 20), TC_RED1, "Test", NormalFont);
    }

    PredRes testAdvanceTime(TestAnimation* anim, unsigned time, bool reqUpdate, unsigned reqCurFrame, double reqFramepartTime);
};

struct TestAnimation : public Animation
{
    TestAnimation(WindowFixture& parent, Window* element, unsigned numFrames, unsigned frameRate, RepeatType repeat)
        : Animation(element, numFrames, frameRate, repeat), parent(parent), updateCalled(false), lastNextFramepartTime(0)
    {}
    WindowFixture& parent;
    bool updateCalled;
    double lastNextFramepartTime;
    double getLinInterpolation() const { return getCurLinearInterpolationFactor(lastNextFramepartTime); }

protected:
    void doUpdate(Window*, double nextFramepartTime) override
    {
        updateCalled = true;
        lastNextFramepartTime = nextFramepartTime;
        parent.lastNextFramepartTime = nextFramepartTime;
        parent.lastFrame = getCurFrame();
        if(isFinished())
            parent.animFinished = true;
    }
};

PredRes WindowFixture::testAdvanceTime(TestAnimation* anim, unsigned time, bool reqUpdate, unsigned reqCurFrame, double reqFramepartTime)
{
    unsigned animId = animMgr.getAnimationId(anim);
    animMgr.update(time);
    BOOST_REQUIRE(animMgr.isAnimationActive(animId));
    if(anim->updateCalled != reqUpdate)
    {
        PredRes result(false);
        result.message() << "Update: " << anim->updateCalled << " != " << reqUpdate;
        return result;
    }
    if(anim->getCurFrame() != reqCurFrame)
    {
        PredRes result(false);
        result.message() << "CurFrame: " << anim->getCurFrame() << " != " << reqCurFrame;
        return result;
    }
    if(anim->lastNextFramepartTime != reqFramepartTime) //-V550
    {
        PredRes result(false);
        result.message() << "lastNextFramepartTime: " << anim->lastNextFramepartTime << " != " << reqFramepartTime;
        return result;
    }
    anim->updateCalled = false;
    return true;
}
} // namespace

BOOST_FIXTURE_TEST_SUITE(Animations, WindowFixture)

BOOST_AUTO_TEST_CASE(AddRemoveAnimations)
{
    BOOST_REQUIRE_EQUAL(animMgr.getNumActiveAnimations(), 0u);
    // nullptr animation ignored
    BOOST_REQUIRE_EQUAL(animMgr.addAnimation(nullptr), 0u);
    BOOST_REQUIRE_EQUAL(animMgr.getNumActiveAnimations(), 0u);
    std::array<Animation*, 3> anims;
    std::array<unsigned, 3> animIds;
    for(unsigned i = 0; i < anims.size(); i++)
    {
        anims[i] = new TestAnimation(*this, bt, 10u, 100u, Animation::RPT_None);
        animIds[i] = animMgr.addAnimation(anims[i]);
        BOOST_REQUIRE_NE(animIds[i], 0u);
    }
    // Don't add anim twice
    BOOST_REQUIRE_EQUAL(animMgr.addAnimation(anims[1]), 0u);
    BOOST_REQUIRE_EQUAL(animMgr.getNumActiveAnimations(), 3u);
    // Check all are active
    for(unsigned i = 0; i < anims.size(); i++)
    {
        BOOST_REQUIRE(animMgr.isAnimationActive(animIds[i]));
        BOOST_REQUIRE_EQUAL(animMgr.getAnimation(animIds[i]), anims[i]);
        BOOST_REQUIRE_EQUAL(animMgr.getAnimationId(anims[i]), animIds[i]);
    }
    std::vector<Animation*> elAnims = animMgr.getElementAnimations(bt->GetID());
    BOOST_REQUIRE_EQUAL(elAnims.size(), 3u);
    for(unsigned i = 0; i < anims.size(); i++)
        BOOST_REQUIRE(helpers::contains(anims, elAnims[i]));
    // Remove 2nd one
    animMgr.removeAnimation(animIds[1]);
    BOOST_REQUIRE_EQUAL(animMgr.getNumActiveAnimations(), 2u);
    BOOST_REQUIRE(!animMgr.isAnimationActive(animIds[1]));
    BOOST_REQUIRE_EQUAL(animMgr.getAnimation(animIds[1]), (Animation*)nullptr);

    // Add animation for another element so we can test that it isn't returned for getElementAnimations
    // and not removed for removeElementAnimations
    animMgr.addAnimation(new TestAnimation(*this, bt2, 10, 10, Animation::RPT_Repeat));
    BOOST_REQUIRE_EQUAL(animMgr.getNumActiveAnimations(), 3u);
    BOOST_REQUIRE_EQUAL(animMgr.getElementAnimations(bt->GetID()).size(), 2u);
    animMgr.removeElementAnimations(bt->GetID());
    BOOST_REQUIRE_EQUAL(animMgr.getNumActiveAnimations(), 1u);
    BOOST_REQUIRE_EQUAL(animMgr.getElementAnimations(bt->GetID()).size(), 0u);
}

BOOST_AUTO_TEST_CASE(AnimationSetterGetter)
{
    TestAnimation anim(*this, bt, 10u, 100u, Animation::RPT_None);
    BOOST_REQUIRE_EQUAL(anim.getCurFrame(), 0u);

    BOOST_REQUIRE_EQUAL(anim.getFrameRate(), 100u);
    anim.setFrameRate(120u);
    BOOST_REQUIRE_EQUAL(anim.getFrameRate(), 120u);

    BOOST_REQUIRE_EQUAL(anim.getNumFrames(), 10u);
    anim.setNumFrames(20u);
    BOOST_REQUIRE_EQUAL(anim.getNumFrames(), 20u);

    BOOST_REQUIRE_EQUAL(anim.getRepeat(), Animation::RPT_None);
    anim.setRepeat(Animation::RPT_Oscillate);
    BOOST_REQUIRE_EQUAL(anim.getRepeat(), Animation::RPT_Oscillate);

    BOOST_REQUIRE_EQUAL(anim.getSkipType(), Animation::SKIP_FRAMES);
    anim.setSkipType(Animation::SKIP_TIME);
    BOOST_REQUIRE_EQUAL(anim.getSkipType(), Animation::SKIP_TIME);
}

BOOST_AUTO_TEST_CASE(EnsureTiming)
{
    // Animation with 5 frames and 10 ms each
    TestAnimation* anim = new TestAnimation(*this, bt, 5, 10, Animation::RPT_None);
    unsigned animId = animMgr.addAnimation(anim);
    // Any start time
    unsigned time = 100;
    // Init time and call first frame
    BOOST_REQUIRE(testAdvanceTime(anim, time, true, 0u, 0.));
    // Execute 9 timesteps: no update should be called
    for(unsigned i = 0; i < 9; i++)
        BOOST_REQUIRE(testAdvanceTime(anim, ++time, false, 0u, 0.));
    BOOST_REQUIRE_EQUAL(anim->getCurFrame(), 0u);
    // Now the update should be called
    BOOST_REQUIRE(testAdvanceTime(anim, ++time, true, 1u, 0.));
    // Skip 9 ms -> no update called
    BOOST_REQUIRE(testAdvanceTime(anim, time += 9, false, 1u, 0.));
    // Skip 6 ms -> update called with half way into next frame
    BOOST_REQUIRE(testAdvanceTime(anim, time += 6, true, 2u, 0.5));
    // For the next 4 ms nothing should happen
    for(unsigned i = 0; i < 4; i++)
        BOOST_REQUIRE(testAdvanceTime(anim, ++time, false, 2u, 0.5));
    // Next update at frame 3
    BOOST_REQUIRE(testAdvanceTime(anim, ++time, true, 3u, 0.));
    // Finish this animation
    BOOST_REQUIRE(!animFinished);
    time += 10;
    animMgr.update(time);
    BOOST_REQUIRE(!animMgr.isAnimationActive(animId));
    BOOST_REQUIRE(animFinished);
    BOOST_REQUIRE_EQUAL(lastFrame, 4u);
    BOOST_REQUIRE_EQUAL(lastNextFramepartTime, 0.);
    BOOST_REQUIRE_EQUAL(animMgr.getNumActiveAnimations(), 0u);

    // Add new animation
    anim = new TestAnimation(*this, bt, 2, 10, Animation::RPT_None);
    animId = animMgr.addAnimation(anim);
    // Init time
    time++;
    animMgr.update(time);
    // Finish it with over time -> next framepart time must still be 0
    animFinished = false;
    time += 15;
    animMgr.update(time);
    BOOST_REQUIRE(!animMgr.isAnimationActive(animId));
    BOOST_REQUIRE(animFinished);
    BOOST_REQUIRE_EQUAL(lastFrame, 1u);
    BOOST_REQUIRE_EQUAL(lastNextFramepartTime, 0.);
    BOOST_REQUIRE_EQUAL(animMgr.getNumActiveAnimations(), 0u);
}

BOOST_AUTO_TEST_CASE(InvalidAnimIdHandling)
{
    animMgr.addAnimation(new TestAnimation(*this, bt, 10u, 100u, Animation::RPT_None));
    // Test all calls with the 0 id (always invalid) and a non-existing one
    BOOST_REQUIRE(!animMgr.isAnimationActive(0));
    BOOST_REQUIRE(!animMgr.isAnimationActive(1337));
    BOOST_REQUIRE(animMgr.getAnimation(0) == nullptr);
    BOOST_REQUIRE(animMgr.getAnimation(1337) == nullptr);
    BOOST_REQUIRE_EQUAL(animMgr.getAnimationId(nullptr), 0u);
    animMgr.removeAnimation(0);
    animMgr.removeAnimation(1337);
    animMgr.finishAnimation(0, true);
    animMgr.finishAnimation(1337, true);
    BOOST_REQUIRE(animMgr.getElementAnimations(1337).empty());
    animMgr.finishElementAnimations(1337, true);
    animMgr.removeElementAnimations(1337);
    // Current not removed
    BOOST_REQUIRE_EQUAL(animMgr.getNumActiveAnimations(), 1u);
}

BOOST_AUTO_TEST_CASE(FinishAnims)
{
    unsigned time = 10;
    std::array<Animation::RepeatType, 4> rpts = {
      {Animation::RPT_None, Animation::RPT_Repeat, Animation::RPT_Oscillate, Animation::RPT_OscillateOnce}};

    for(unsigned i = 0; i < rpts.size(); i++)
    {
        bool isOscillate = rpts[i] == Animation::RPT_Oscillate || rpts[i] == Animation::RPT_OscillateOnce;
        unsigned expectedLastFrame = 5;
        // Those have to go back
        if(isOscillate)
            expectedLastFrame = 0;

        // Finish before it started -> 1 frame executed right away
        lastFrame = 9999;
        animMgr.addAnimation(new TestAnimation(*this, bt, 6, 10, rpts[i]));
        animMgr.finishElementAnimations(bt->GetID(), false);
        BOOST_REQUIRE_EQUAL(animMgr.getNumActiveAnimations(), 0u);
        BOOST_REQUIRE_EQUAL(lastFrame, expectedLastFrame);

        // Start animation, then finish it before 1st (actually 2nd) frame is run
        lastFrame = 9999;
        animMgr.addAnimation(new TestAnimation(*this, bt, 6, 10, rpts[i]));
        animMgr.update(time += 10);
        animMgr.finishElementAnimations(bt->GetID(), false);
        // Non-oscillating have to go to end, oscillate already are
        if(!isOscillate)
        {
            BOOST_REQUIRE_EQUAL(animMgr.getNumActiveAnimations(), 1u);
            animMgr.update(time += 50);
        }
        BOOST_REQUIRE_EQUAL(animMgr.getNumActiveAnimations(), 0u);
        BOOST_REQUIRE_EQUAL(lastFrame, expectedLastFrame);
        // Force
        lastFrame = 9999;
        animMgr.addAnimation(new TestAnimation(*this, bt, 6, 10, rpts[i]));
        animMgr.update(time += 10);
        animMgr.finishElementAnimations(bt->GetID(), true);
        BOOST_REQUIRE_EQUAL(animMgr.getNumActiveAnimations(), 0u);
        BOOST_REQUIRE_EQUAL(lastFrame, expectedLastFrame);

        // Start animation and run to 2nd frame, finish and run to end
        lastFrame = 9999;
        animMgr.addAnimation(new TestAnimation(*this, bt, 6, 10, rpts[i]));
        animMgr.update(time += 10);
        animMgr.update(time += 10);
        animMgr.finishElementAnimations(bt->GetID(), false);
        BOOST_REQUIRE_EQUAL(animMgr.getNumActiveAnimations(), 1u);
        animMgr.update(time += 40);
        // Those have to go back
        if(isOscillate)
        {
            BOOST_REQUIRE_EQUAL(animMgr.getNumActiveAnimations(), 1u);
            animMgr.update(time += 50);
        }
        BOOST_REQUIRE_EQUAL(animMgr.getNumActiveAnimations(), 0u);
        BOOST_REQUIRE_EQUAL(lastFrame, expectedLastFrame);

        // Go through whole animation -> oscillate are half way, others finished
        lastFrame = 9999;
        animMgr.addAnimation(new TestAnimation(*this, bt, 6, 10, rpts[i]));
        animMgr.update(time += 10);
        animMgr.update(time += 50);
        animMgr.finishElementAnimations(bt->GetID(), false);
        // Those have to go back
        if(isOscillate)
        {
            BOOST_REQUIRE_EQUAL(animMgr.getNumActiveAnimations(), 1u);
            animMgr.update(time += 50);
        }
        BOOST_REQUIRE_EQUAL(animMgr.getNumActiveAnimations(), 0u);
        BOOST_REQUIRE_EQUAL(lastFrame, expectedLastFrame);

        // Go twice through except repeat which needs another frame -> all are finished
        lastFrame = 9999;
        animMgr.addAnimation(new TestAnimation(*this, bt, 6, 10, rpts[i]));
        animMgr.update(time += 10);
        animMgr.update(time += 100);
        // Another frame for repeat mode
        if(rpts[i] == Animation::RPT_Repeat)
            animMgr.update(time += 10);
        animMgr.finishElementAnimations(bt->GetID(), false);
        BOOST_REQUIRE_EQUAL(animMgr.getNumActiveAnimations(), 0u);
        BOOST_REQUIRE_EQUAL(lastFrame, expectedLastFrame);
    }
    // Test finish immediately for all dts
    for(unsigned i = 0; i < rpts.size(); i++)
    {
        bool isOscillate = rpts[i] == Animation::RPT_Oscillate || rpts[i] == Animation::RPT_OscillateOnce;
        unsigned expectedLastFrame = 5;
        // Those have to go back
        if(isOscillate)
            expectedLastFrame = 0;
        // Unstarted
        lastFrame = 9999;
        animMgr.addAnimation(new TestAnimation(*this, bt, 6, 10, rpts[i]));
        animMgr.finishElementAnimations(bt->GetID(), true);
        BOOST_REQUIRE_EQUAL(animMgr.getNumActiveAnimations(), 0u);
        BOOST_REQUIRE_EQUAL(lastFrame, expectedLastFrame);
        // Start and run some time -> always at last frame
        for(unsigned dt = 0; dt < 200; dt++)
        {
            lastFrame = 9999;
            animMgr.addAnimation(new TestAnimation(*this, bt, 6, 10, rpts[i]));
            animMgr.update(time += 1);
            animMgr.update(time += dt);
            animMgr.finishElementAnimations(bt->GetID(), true);
            BOOST_REQUIRE_EQUAL(animMgr.getNumActiveAnimations(), 0u);
            BOOST_REQUIRE_EQUAL(lastFrame, expectedLastFrame);
        }
    }
}

BOOST_AUTO_TEST_CASE(SkipFrames)
{
    // Animation with 15 frames and 10 ms each
    TestAnimation* anim = new TestAnimation(*this, bt, 15, 10, Animation::RPT_None);
    unsigned animId = animMgr.addAnimation(anim);
    // Init with any start time
    unsigned time = 100;
    animMgr.update(time);
    BOOST_REQUIRE_EQUAL(anim->getCurFrame(), 0u);

    // First test: Skip time, play every frame
    anim->setSkipType(Animation::SKIP_TIME);
    // Skip 15 ms -> half way into frame 2
    BOOST_REQUIRE(testAdvanceTime(anim, time += 15, true, 1u, 0.5));
    // Skip another 15 ms -> timewise we are at the start of frame 3
    // but we should be at the end of frame 2 which we are not allowed to skip
    BOOST_REQUIRE(testAdvanceTime(anim, time += 15, true, 2u, 9. / 10.));
    // But frame 3 should follow immediately
    BOOST_REQUIRE(testAdvanceTime(anim, time += 1, true, 3u, 0.));
    // Same behavior if we skip many ms
    BOOST_REQUIRE(testAdvanceTime(anim, time += 1000, true, 4u, 9. / 10.));
    BOOST_REQUIRE(testAdvanceTime(anim, time += 1, true, 5u, 0.));

    // 2nd test: Skip Frames, but always play last frame
    anim->setSkipType(Animation::SKIP_FRAMES);
    // Skip 15 ms -> half way into frame 7
    BOOST_REQUIRE(testAdvanceTime(anim, time += 15, true, 6u, 0.5));
    // Skip 20 ms -> frame 7 skipped, half way into frame 9
    BOOST_REQUIRE(testAdvanceTime(anim, time += 20, true, 8u, 0.5));
    // Skip 30 ms -> frame 9&10 skipped, halfway into frame 12
    BOOST_REQUIRE(testAdvanceTime(anim, time += 30, true, 11u, 0.5));

    // Finish this animation
    BOOST_REQUIRE(!animFinished);
    time += 1000;
    animMgr.update(time);
    BOOST_REQUIRE(!animMgr.isAnimationActive(animId));
    BOOST_REQUIRE(animFinished);
    BOOST_REQUIRE_EQUAL(lastFrame, 14u);
    BOOST_REQUIRE_EQUAL(lastNextFramepartTime, 0.);
    BOOST_REQUIRE_EQUAL(animMgr.getNumActiveAnimations(), 0u);
}

BOOST_AUTO_TEST_CASE(Oscillate)
{
    // Animation with 6 frames and 10 steps each
    TestAnimation* anim = new TestAnimation(*this, bt, 6, 10, Animation::RPT_Oscillate);
    animMgr.addAnimation(anim);
    // Init with any start time
    unsigned time = 100;
    animMgr.update(time);
    BOOST_REQUIRE_EQUAL(anim->getCurFrame(), 0u);
    // Advance till end
    for(unsigned i = 1; i < 6; i++)
        BOOST_REQUIRE(testAdvanceTime(anim, time += 10, true, i, 0.));
    BOOST_REQUIRE(!anim->isFinished());
    // And back again
    for(unsigned i = 4; i > 0; i--)
        BOOST_REQUIRE(testAdvanceTime(anim, time += 10, true, i, 0.));
    BOOST_REQUIRE(!anim->isFinished());
    // Advance till end
    for(unsigned i = 0; i < 6; i++)
        BOOST_REQUIRE(testAdvanceTime(anim, time += 10, true, i, 0.));
    // Test skipping of frames
    BOOST_REQUIRE_EQUAL(anim->getSkipType(), Animation::SKIP_FRAMES);
    // 30 ms -> Skip frames 4 & 3
    BOOST_REQUIRE(testAdvanceTime(anim, time += 30, true, 2, 0.));
    // 40 ms -> Skip frames 1, 0, 1
    BOOST_REQUIRE(testAdvanceTime(anim, time += 40, true, 2, 0.));
    // Next frame
    BOOST_REQUIRE(testAdvanceTime(anim, time += 10, true, 3, 0.));
    // 40 ms -> Skip frames 4, 5, 4
    BOOST_REQUIRE(testAdvanceTime(anim, time += 40, true, 3, 0.));
    // Next frame
    BOOST_REQUIRE(testAdvanceTime(anim, time += 10, true, 2, 0.));
    // Multiple cycles
    // 130 ms -> Skip frames 1,0,1,2,3,4,5,4,3,2,1,0
    BOOST_REQUIRE(testAdvanceTime(anim, time += 130, true, 1, 0.));
    // Next frame
    BOOST_REQUIRE(testAdvanceTime(anim, time += 10, true, 2, 0.));
    // 100 ms -> Skip frames 3,4,5,4,3,2,1,0,1
    BOOST_REQUIRE(testAdvanceTime(anim, time += 100, true, 2, 0.));
    // Next frame
    BOOST_REQUIRE(testAdvanceTime(anim, time += 10, true, 3, 0.));

    // Test skipping time
    anim->setSkipType(Animation::SKIP_TIME);
    // Skip 15 ms -> half way into frame 5
    BOOST_REQUIRE(testAdvanceTime(anim, time += 15, true, 4u, 0.5));
    BOOST_REQUIRE(testAdvanceTime(anim, time += 15, true, 5u, 9. / 10.));
    // next frame should follow immediately
    BOOST_REQUIRE(testAdvanceTime(anim, time += 1, true, 4u, 0.));
    // Same behavior if we skip many ms
    BOOST_REQUIRE(testAdvanceTime(anim, time += 1000, true, 3u, 9. / 10.));
    BOOST_REQUIRE(testAdvanceTime(anim, time += 1, true, 2u, 0.));
}

BOOST_AUTO_TEST_CASE(OscillateOnce)
{
    // Animation with 6 frames and 10 steps each
    TestAnimation* anim = new TestAnimation(*this, bt, 6, 10, Animation::RPT_OscillateOnce);
    animMgr.addAnimation(anim);
    // Init with any start time
    unsigned time = 100;
    animMgr.update(time);
    BOOST_REQUIRE_EQUAL(anim->getCurFrame(), 0u);
    // Advance till end
    for(unsigned i = 1; i < 6; i++)
        BOOST_REQUIRE(testAdvanceTime(anim, time += 10, true, i, 0.));
    BOOST_REQUIRE(!anim->isFinished());
    // And back again
    for(unsigned i = 4; i > 0; i--)
        BOOST_REQUIRE(testAdvanceTime(anim, time += 10, true, i, 0.));
    BOOST_REQUIRE(!anim->isFinished());
    // And end:
    lastFrame = 1111;
    animMgr.update(time += 10);
    BOOST_REQUIRE_EQUAL(animMgr.getNumActiveAnimations(), 0u);
    BOOST_REQUIRE_EQUAL(lastFrame, 0u);

    // Test skipping of frames
    anim = new TestAnimation(*this, bt, 6, 10, Animation::RPT_OscillateOnce);
    animMgr.addAnimation(anim);
    animMgr.update(time += 10);
    BOOST_REQUIRE_EQUAL(anim->getSkipType(), Animation::SKIP_FRAMES);

    // 30 ms -> Skip frames 1 & 2
    BOOST_REQUIRE(testAdvanceTime(anim, time += 30, true, 3, 0.));
    // 40 ms -> Skip frames 4, 5, 4
    BOOST_REQUIRE(testAdvanceTime(anim, time += 40, true, 3, 0.));
    // Next frame
    BOOST_REQUIRE(testAdvanceTime(anim, time += 10, true, 2, 0.));
    // Skip over end
    lastFrame = 1111;
    animMgr.update(time += 100);
    BOOST_REQUIRE_EQUAL(animMgr.getNumActiveAnimations(), 0u);
    BOOST_REQUIRE_EQUAL(lastFrame, 0u);

    anim = new TestAnimation(*this, bt, 6, 10, Animation::RPT_OscillateOnce);
    animMgr.addAnimation(anim);
    animMgr.update(time += 10);
    // Skip over end
    lastFrame = 1111;
    animMgr.update(time += 1000);
    BOOST_REQUIRE_EQUAL(animMgr.getNumActiveAnimations(), 0u);
    BOOST_REQUIRE_EQUAL(lastFrame, 0u);

    anim = new TestAnimation(*this, bt, 6, 10, Animation::RPT_OscillateOnce);
    animMgr.addAnimation(anim);
    animMgr.update(time += 10);
    // execute 4 frames
    BOOST_REQUIRE(testAdvanceTime(anim, time += 40, true, 4, 0.));
    // Set size to less
    anim->setNumFrames(3);
    // We should still be able to get to end
    lastFrame = 1111;
    animMgr.update(time += 1000);
    BOOST_REQUIRE_EQUAL(animMgr.getNumActiveAnimations(), 0u);
    BOOST_REQUIRE_EQUAL(lastFrame, 0u);
}

BOOST_AUTO_TEST_CASE(Repeat)
{
    // Animation with 6 frames and 10 steps each
    TestAnimation* anim = new TestAnimation(*this, bt, 6, 10, Animation::RPT_Repeat);
    animMgr.addAnimation(anim);
    // Init with any start time
    unsigned time = 100;
    animMgr.update(time);
    BOOST_REQUIRE_EQUAL(anim->getCurFrame(), 0u);
    // Advance till end
    for(unsigned i = 1; i < 6; i++)
        BOOST_REQUIRE(testAdvanceTime(anim, time += 10, true, i, 0.));
    BOOST_REQUIRE(!anim->isFinished());
    // And another cycle
    for(unsigned i = 0; i < 6; i++)
        BOOST_REQUIRE(testAdvanceTime(anim, time += 10, true, i, 0.));
    // Test skipping of frames
    BOOST_REQUIRE_EQUAL(anim->getSkipType(), Animation::SKIP_FRAMES);
    // 30 ms -> Skip frames 0 & 1
    BOOST_REQUIRE(testAdvanceTime(anim, time += 30, true, 2, 0.));
    // 30 ms -> Skip frames 3 & 4
    BOOST_REQUIRE(testAdvanceTime(anim, time += 30, true, 5, 0.));
    // 40 ms -> Skip frames 0, 1, 2
    BOOST_REQUIRE(testAdvanceTime(anim, time += 40, true, 3, 0.));
    // Multiple cycles
    // 130 ms -> Skip frames 4,5,0,1,2,3,4,5,0,1,2,3
    BOOST_REQUIRE(testAdvanceTime(anim, time += 130, true, 4, 0.));
    // 100 ms -> Skip frames 5,0,1,2,3,4,5,0,1
    BOOST_REQUIRE(testAdvanceTime(anim, time += 100, true, 2, 0.));

    // Test skipping time
    anim->setSkipType(Animation::SKIP_TIME);
    // Skip 15 ms -> half way into frame 5
    BOOST_REQUIRE(testAdvanceTime(anim, time += 15, true, 3u, 0.5));
    BOOST_REQUIRE(testAdvanceTime(anim, time += 15, true, 4u, 9. / 10.));
    // next frame should follow immediately
    BOOST_REQUIRE(testAdvanceTime(anim, time += 1, true, 5u, 0.));
    // Same behavior if we skip many ms
    BOOST_REQUIRE(testAdvanceTime(anim, time += 1000, true, 0u, 9. / 10.));
    BOOST_REQUIRE(testAdvanceTime(anim, time += 1, true, 1u, 0.));
}

BOOST_AUTO_TEST_CASE(LinearInterpolationFactor)
{
    // 11 frames with 100ms
    TestAnimation* anim = new TestAnimation(*this, bt, 11, 100, Animation::RPT_None);
    animMgr.addAnimation(anim);
    // Init with any start time
    unsigned time = 100;
    animMgr.update(time);
    BOOST_REQUIRE_EQUAL(anim->getCurFrame(), 0u);
    BOOST_REQUIRE(testAdvanceTime(anim, time += 100, true, 1u, 0.));
    BOOST_REQUIRE_CLOSE(anim->getLinInterpolation(), 0.1, 0.001);
    BOOST_REQUIRE(testAdvanceTime(anim, time += 120, true, 2u, 0.2));
    BOOST_REQUIRE_CLOSE(anim->getLinInterpolation(), 0.22, 0.001);
    BOOST_REQUIRE(testAdvanceTime(anim, time += 160, true, 3u, 0.8));
    BOOST_REQUIRE_CLOSE(anim->getLinInterpolation(), 0.38, 0.001);

    // Test around wrapping
    anim->setRepeat(Animation::RPT_Repeat);
    BOOST_REQUIRE(testAdvanceTime(anim, time += 600, true, 9u, 0.8));
    BOOST_REQUIRE_CLOSE(anim->getLinInterpolation(), 0.98, 0.001);
    BOOST_REQUIRE(testAdvanceTime(anim, time += 100, true, 10u, 0.)); // 0.8 hidden
    BOOST_REQUIRE_EQUAL(anim->getLinInterpolation(), 1.);
    BOOST_REQUIRE(testAdvanceTime(anim, time += 90, true, 0u, 0.7));
    BOOST_REQUIRE_CLOSE(anim->getLinInterpolation(), 0.07, 0.001);

    anim->setRepeat(Animation::RPT_Oscillate);
    BOOST_REQUIRE(testAdvanceTime(anim, time += 1000, true, 10u, 0.7));
    BOOST_REQUIRE_CLOSE(anim->getLinInterpolation(), 0.93, 0.001);
    BOOST_REQUIRE(testAdvanceTime(anim, time += 120, true, 9u, 0.9));
    BOOST_REQUIRE_CLOSE(anim->getLinInterpolation(), 0.81, 0.001);
    BOOST_REQUIRE(testAdvanceTime(anim, time += 800, true, 1u, 0.9));
    BOOST_REQUIRE_CLOSE(anim->getLinInterpolation(), 0.01, 0.001);
    BOOST_REQUIRE(testAdvanceTime(anim, time += 50, true, 0u, 0.4));
    BOOST_REQUIRE_CLOSE(anim->getLinInterpolation(), 0.04, 0.001);
    BOOST_REQUIRE(testAdvanceTime(anim, time += 250, true, 2u, 0.9));
    BOOST_REQUIRE_CLOSE(anim->getLinInterpolation(), 0.29, 0.001);
}

BOOST_AUTO_TEST_CASE(MoveAni)
{
    bt->SetPos(DrawPoint(100, 200));
    DrawPoint targetPt(110, 300);
    animMgr.addAnimation(new MoveAnimation(bt, targetPt, 500, Animation::RPT_None));
    // Init with any start time
    unsigned time = 100;
    animMgr.update(time);
    // Move half way
    animMgr.update(time += 250);
    BOOST_REQUIRE_EQUAL(bt->GetPos(), DrawPoint(105, 250));
    // Move over target
    animMgr.update(time += 270);
    BOOST_REQUIRE_EQUAL(bt->GetPos(), targetPt);
    BOOST_REQUIRE_EQUAL(animMgr.getNumActiveAnimations(), 0u);

    bt->SetPos(DrawPoint(100, 200));
    unsigned animId = animMgr.addAnimation(new MoveAnimation(bt, targetPt, 500, Animation::RPT_Repeat));
    unsigned frameRate = animMgr.getAnimation(animId)->getFrameRate();
    animMgr.update(time += 1);
    // Move over target: Animation is 500ms long and restarts after 1 frame -> 500 + frame + 50(10%)
    animMgr.update(time += (550 + frameRate));
    BOOST_REQUIRE_EQUAL(bt->GetPos(), DrawPoint(101, 210));
    BOOST_REQUIRE(animMgr.isAnimationActive(animId));

    animMgr.getAnimation(animId)->setRepeat(Animation::RPT_Oscillate);
    // Move over target. Here we don't need the additional frame as we move directly to the 2nd last frame
    animMgr.update(time += 500);
    BOOST_REQUIRE_EQUAL(bt->GetPos(), DrawPoint(109, 290));
    BOOST_REQUIRE(animMgr.isAnimationActive(animId));
}

BOOST_AUTO_TEST_CASE(MoveAniScale)
{
    MockupVideoDriver* video = uiHelper::GetVideoDriver();
    Desktop* dsk = new Desktop(nullptr);
    WINDOWMANAGER.Switch(dsk);
    WINDOWMANAGER.Draw();
    bt = dsk->AddTextButton(0, DrawPoint(10, 20), Extent(100, 150), TC_RED1, "", NormalFont);
    ctrlButton* btReference = dsk->AddTextButton(1, DrawPoint(130, bt->GetPos().y), bt->GetSize(), TC_RED1, "", NormalFont);
    dsk->GetAnimationManager().addAnimation(new MoveAnimation(bt, btReference->GetPos(), 1000, Animation::RPT_None));
    dsk->Msg_PaintBefore();
    // Pass the animation
    video->tickCount_ += 1100;
    dsk->Msg_PaintBefore();
    BOOST_REQUIRE_EQUAL(bt->GetPos(), btReference->GetPos());

    // Restart
    bt->SetPos(DrawPoint(10, 20));
    dsk->GetAnimationManager().addAnimation(new MoveAnimation(bt, btReference->GetPos(), 1000, Animation::RPT_None));
    video->tickCount_ += 1;
    dsk->Msg_PaintBefore();
    VIDEODRIVER.ResizeScreen(1024, 768, false);
    // Pass the animation
    video->tickCount_ += 1100;
    dsk->Msg_PaintBefore();
    BOOST_REQUIRE_EQUAL(bt->GetPos(), btReference->GetPos());
}

BOOST_AUTO_TEST_CASE(ToogleAnim)
{
    animMgr.addAnimation(new ToggleAnimation<Window>(bt, &Window::SetVisible, true, 1000));
    animMgr.addAnimation(new BlinkButtonAnim(bt2, true, 1000));
    // Init with any start time
    unsigned time = 100;
    animMgr.update(time);
    BOOST_REQUIRE(bt->IsVisible());
    BOOST_REQUIRE(bt2->GetIlluminated());
    // Switch
    animMgr.update(time += 1000);
    BOOST_REQUIRE(!bt->IsVisible());
    BOOST_REQUIRE(!bt2->GetIlluminated());
    animMgr.update(time += 1000);
    BOOST_REQUIRE(bt->IsVisible());
    BOOST_REQUIRE(bt2->GetIlluminated());
}

BOOST_AUTO_TEST_SUITE_END()
