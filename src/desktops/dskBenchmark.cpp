// Copyright (c) 2005 - 2018 Settlers Freaks (sf-team at siedler25.org)
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
#include "dskBenchmark.h"
#include "Loader.h"
#include "WindowManager.h"
#include "boost/foreach.hpp"
#include "controls/ctrlText.h"
#include "dskMainMenu.h"
#include "helpers/converters.h"
#include "ogl/FontStyle.h"
#include "libutil/Log.h"
#include "libutil/strFuncs.h"
#include <boost/random.hpp>
#include "drivers/VideoDriverWrapper.h"

namespace {
enum
{
    ID_txtHelp = dskMenuBase::ID_FIRST_FREE,
    ID_txtAmount,
    ID_first
};
}

dskBenchmark::dskBenchmark() : curTest_(TEST_NONE), numInstances_(1000), frameCtr_(FrameCounter::clock::duration::max())
{
    AddText(ID_txtHelp, DrawPoint(5, 5), "Use F1-F2 to start benchmark, NUM_n to set amount of instances", COLOR_YELLOW, FontStyle::LEFT,
            LargeFont);
    AddText(ID_txtAmount, DrawPoint(795, 5), "Instances: default", COLOR_YELLOW, FontStyle::RIGHT, LargeFont);
}

bool dskBenchmark::Msg_KeyDown(const KeyEvent& ke)
{
    switch(ke.kt)
    {
        case KT_ESCAPE: WINDOWMANAGER.Switch(new dskMainMenu); break;
        case KT_F1: startTest(TEST_TEXT); break;
        case KT_F2: startTest(TEST_PRIMITIVES); break;
        case KT_CHAR:
            if(ke.c >= '0' && ke.c <= '9')
            {
                numInstances_ = (ke.c - '0') * 100;
                if(numInstances_ == 0)
                    numInstances_ = 1000;
                GetCtrl<ctrlText>(ID_txtAmount)->SetText("Instances: " + helpers::toString(numInstances_));
            }
        default: return dskMenuBase::Msg_KeyDown(ke);
    }
    return true;
}

void dskBenchmark::Msg_PaintAfter()
{
    BOOST_FOREACH(const ColoredRect& rect, rects_)
        DrawRectangle(rect.rect, rect.clr);
    BOOST_FOREACH(const ColoredLine& line, lines_)
        DrawLine(line.p1, line.p2, line.width, line.clr);
    if(curTest_ != TEST_NONE)
    {
        frameCtr_.update();
        if(frameCtr_.getCurNumFrames() >= 150)
            finishTest();
    }
    dskMenuBase::Msg_PaintAfter();
}

void dskBenchmark::startTest(Test test)
{
    uint32_t seed = 0x1337;
    boost::random::mt19937 rng(seed);
    if(test == TEST_TEXT)
    {
        static const std::string charset =
          "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()`~-_=+[{]{\\|;:'\",<.>/? ";

        boost::random::uniform_int_distribution<unsigned> distr(1, 30);
        boost::random::uniform_int_distribution<unsigned> distr2(0, 100000);
        boost::random::uniform_int_distribution<unsigned> distrMove(10, 25);
        DrawPoint pt(0, 0);
        glArchivItem_Font* fnt = NormalFont;
        for(int i = 0; i < numInstances_; i++)
        {
            std::string txt = createRandString(distr(rng), charset, seed);
            seed += distr2(rng);
            pt.y += distrMove(rng);
            if(pt.y >= 580)
            {
                pt.y = 0;
                pt.x += 150 + distrMove(rng) / 3;
                if(pt.x >= 780)
                    pt.x = distrMove(rng);
            }
            AddText(ID_first + i, pt, txt, COLOR_YELLOW, FontStyle::LEFT, fnt);
        }
    } else if(test == TEST_PRIMITIVES)
    {
        boost::random::uniform_int_distribution<unsigned> distSize(5, 50);
        boost::random::uniform_int_distribution<unsigned> distClr(0, 0xFF);
        boost::random::uniform_int_distribution<unsigned> distrMove(10, 50);
        boost::random::uniform_int_distribution<unsigned> distrPosX(0, 800);
        boost::random::uniform_int_distribution<unsigned> distrPosY(0, 600);
        boost::random::uniform_int_distribution<unsigned> distrWidth(1, 10);
        DrawPoint pt(0, 0);
        for(int i = 0; i < numInstances_; i++)
        {
            ColoredRect rect;
            rect.rect.move(pt);
            rect.rect.setSize(Extent(distSize(rng), distSize(rng)));
            unsigned clr = distClr(rng);
            unsigned alpha = std::min((distClr(rng) + 10u) * 10u, 0xFFu);
            rect.clr = MakeColor(alpha, clr, clr, clr);
            rects_.push_back(rect);
            pt.y += distrMove(rng);
            if(pt.y >= 580)
            {
                pt.y = 0;
                pt.x += 150 + distrMove(rng) / 3;
                if(pt.x >= 780)
                    pt.x = distrMove(rng);
            }
            ColoredLine line;
            line.p1 = Position(distrPosX(rng), distrPosY(rng));
            line.p2 = Position(distrPosX(rng), distrPosY(rng));
            line.width = distrWidth(rng);
            line.clr = MakeColor(alpha, clr, clr, clr);
            lines_.push_back(line);
        }
    }
    VIDEODRIVER.setTargetFramerate(-1);
    curTest_ = test;
    frameCtr_ = FrameCounter(frameCtr_.getUpdateInterval());
}

void dskBenchmark::finishTest()
{
    using namespace boost::chrono;
    LOG.write("Benchmark #%1% took %2%. -> %3%/frame\n") % curTest_ % duration_cast<duration<float> >(frameCtr_.getCurIntervalLength())
      % duration_cast<milliseconds>(frameCtr_.getCurIntervalLength() / frameCtr_.getCurNumFrames());
    curTest_ = TEST_NONE;
    std::vector<Window*> ctrls = GetCtrls<Window>();
    BOOST_FOREACH(Window* ctrl, ctrls)
    {
        if(ctrl->GetID() >= ID_first)
            DeleteCtrl(ctrl->GetID());
    }
    rects_.clear();
    lines_.clear();
    SetFpsDisplay(true);
    VIDEODRIVER.setTargetFramerate(0);
}
