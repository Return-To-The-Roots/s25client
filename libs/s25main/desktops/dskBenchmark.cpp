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
#include "Game.h"
#include "GameLoader.h"
#include "Loader.h"
#include "PlayerInfo.h"
#include "WindowManager.h"
#include "boost/foreach.hpp"
#include "buildings/nobMilitary.h"
#include "controls/ctrlText.h"
#include "drivers/VideoDriverWrapper.h"
#include "dskMainMenu.h"
#include "factories/BuildingFactory.h"
#include "figures/nofPassiveSoldier.h"
#include "figures/nofPassiveWorker.h"
#include "helpers/mathFuncs.h"
#include "helpers/strUtils.h"
#include "lua/GameDataLoader.h"
#include "ogl/FontStyle.h"
#include "ogl/IRenderer.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include "world/GameWorldView.h"
#include "world/GameWorldViewer.h"
#include "world/MapLoader.h"
#include "gameTypes/RoadBuildState.h"
#include "libutil/Log.h"
#include "libutil/strFuncs.h"
#include <boost/random.hpp>

namespace {
enum
{
    ID_txtHelp = dskMenuBase::ID_FIRST_FREE,
    ID_txtAmount,
    ID_first
};
}

static const unsigned numTestFrames = 500u;

struct dskBenchmark::GameView
{
    GameWorldViewer viewer;
    GameWorldView view;
    GameView(GameWorldBase& gw, Extent size) : viewer(0u, gw), view(viewer, Position(0, 0), size)
    {
        viewer.InitTerrainRenderer();
        view.MoveToMapPt(MapPoint(0, 0));
        view.ToggleShowBQ();
        view.ToggleShowNames();
    }
};

dskBenchmark::dskBenchmark() : curTest_(TEST_NONE), runAll_(false), numInstances_(1000), frameCtr_(FrameCounter::clock::duration::max())
{
    AddText(ID_txtHelp, DrawPoint(5, 5), "Use F1-F5 to start benchmark, F10 for all, NUM_n to set amount of instances", COLOR_YELLOW,
            FontStyle::LEFT, LargeFont);
    AddText(ID_txtAmount, DrawPoint(795, 5), "Instances: default", COLOR_YELLOW, FontStyle::RIGHT, LargeFont);
    BOOST_FOREACH(boost::chrono::milliseconds& t, testDurations_)
        t = t.zero();
}

dskBenchmark::~dskBenchmark()
{
    printTimes();
}

bool dskBenchmark::Msg_KeyDown(const KeyEvent& ke)
{
    switch(ke.kt)
    {
        case KT_ESCAPE: WINDOWMANAGER.Switch(new dskMainMenu); break;
        case KT_F1: startTest(TEST_TEXT); break;
        case KT_F2: startTest(TEST_PRIMITIVES); break;
        case KT_F3: startTest(TEST_EMPTY_GAME); break;
        case KT_F4: startTest(TEST_BASIC_GAME); break;
        case KT_F5: startTest(TEST_FULL_GAME); break;
        case KT_F10:
            runAll_ = true;
            startTest(TEST_TEXT);
            break;
        case KT_CHAR:
            if(ke.c >= '0' && ke.c <= '9')
            {
                numInstances_ = (ke.c - '0') * 100;
                if(numInstances_ == 0)
                    numInstances_ = 1000;
                GetCtrl<ctrlText>(ID_txtAmount)->SetText("Instances: " + helpers::toString(numInstances_));
                break;
            } else
                return dskMenuBase::Msg_KeyDown(ke);
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
    if(gameView_)
    {
        RoadBuildState roadState;
        roadState.mode = RM_DISABLED;
        gameView_->view.Draw(roadState, MapPoint::Invalid(), false);
    }
    if(curTest_ != TEST_NONE)
    {
        if(frameCtr_.getCurNumFrames() + 1u >= numTestFrames)
            VIDEODRIVER.GetRenderer()->synchronize();
        frameCtr_.update();
        if(frameCtr_.getCurNumFrames() >= numTestFrames)
            finishTest();
    }
    dskMenuBase::Msg_PaintAfter();
}

void dskBenchmark::SetActive(bool activate)
{
    if(!IsActive() && activate)
        VIDEODRIVER.ResizeScreen(1600, 900, false);
    dskMenuBase::SetActive(activate);
}

void dskBenchmark::startTest(Test test)
{
    uint32_t seed = 0x1337;
    boost::random::mt19937 rng(seed);
    switch(test)
    {
        case TEST_NONE:
        case TEST_CT: return;
        case TEST_TEXT:
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
            break;
        }
        case TEST_PRIMITIVES:
        {
            Extent screenSize = VIDEODRIVER.GetScreenSize();
            boost::random::uniform_int_distribution<unsigned> distSize(5, 50);
            boost::random::uniform_int_distribution<unsigned> distClr(0, 0xFF);
            boost::random::uniform_int_distribution<unsigned> distrMove(10, 50);
            boost::random::uniform_int_distribution<unsigned> distrPosX(0, screenSize.x);
            boost::random::uniform_int_distribution<unsigned> distrPosY(0, screenSize.y);
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
                if(pt.y >= static_cast<int>(screenSize.y) - 20)
                {
                    pt.y = 0;
                    pt.x += 150 + distrMove(rng) / 3;
                    if(pt.x >= static_cast<int>(screenSize.x) - 20)
                        pt.x = distrMove(rng);
                }
                ColoredLine line;
                line.p1 = Position(distrPosX(rng), distrPosY(rng));
                line.p2 = Position(distrPosX(rng), distrPosY(rng));
                line.width = distrWidth(rng);
                line.clr = MakeColor(alpha, clr, clr, clr);
                lines_.push_back(line);
            }
            break;
        }
        case TEST_EMPTY_GAME:
            createGame();
            if(!game_)
                return;
            RTTR_FOREACH_PT(MapPoint, game_->world.GetSize())
            {
                game_->world.SetVisibility(pt, 0, VIS_VISIBLE);
            }
            break;
        case TEST_BASIC_GAME:
        {
            createGame();
            if(!game_)
                return;
            std::vector<MapPoint> hqs(2, MapPoint(0, 0));
            hqs[1].x += 30;
            MapLoader::PlaceHQs(game_->world, hqs, false);
            break;
        }
        case TEST_FULL_GAME:
        {
            createGame();
            if(!game_)
                return;
            std::vector<MapPoint> hqs(2, MapPoint(0, 0));
            hqs[1].x += 30;
            MapLoader::PlaceHQs(game_->world, hqs, false);
            for(unsigned i = 0; i < hqs.size(); i++)
            {
                std::vector<MapPoint> pts = game_->world.GetPointsInRadius(hqs[i], 15);
                boost::random::bernoulli_distribution<> dist(numInstances_ / 1000.f);
                boost::random::bernoulli_distribution<> distEqual;
                boost::array<BuildingType, 5> blds = {{BLD_BARRACKS, BLD_MILL, BLD_IRONMINE, BLD_SLAUGHTERHOUSE, BLD_BAKERY}};
                boost::random::uniform_int_distribution<unsigned> getBld(0, blds.size() - 1);
                boost::random::uniform_int_distribution<unsigned> getJob(0, NUM_JOB_TYPES - 1);
                boost::random::uniform_int_distribution<int> getDir(0, Direction::COUNT - 1);
                BOOST_FOREACH(MapPoint pt, pts)
                {
                    MapPoint flagPt = game_->world.GetNeighbour(pt, Direction::SOUTHEAST);
                    if(game_->world.GetNode(pt).obj || game_->world.GetNode(flagPt).obj || !dist(rng))
                        continue;
                    BuildingType bldType = blds[getBld(rng)];
                    noBuilding* bld =
                      BuildingFactory::CreateBuilding(game_->world, bldType, pt, i, distEqual(rng) ? NAT_AFRICANS : NAT_JAPANESE);
                    if(bldType == BLD_BARRACKS)
                    {
                        nobMilitary* mil = static_cast<nobMilitary*>(bld);
                        nofPassiveSoldier* sld = new nofPassiveSoldier(pt, i, mil, mil, 0);
                        mil->AddPassiveSoldier(sld);
                    }
                    nofPassiveWorker* figure = new nofPassiveWorker(Job(getJob(rng)), flagPt, i, NULL);
                    game_->world.AddFigure(flagPt, figure);
                    figure->StartWandering();
                    figure->StartWalking(Direction::fromInt(getDir(rng)));
                }
            }
            break;
        }
    }
    if(game_)
        gameView_.reset(new GameView(game_->world, VIDEODRIVER.GetScreenSize()));
    VIDEODRIVER.GetRenderer()->synchronize();
    VIDEODRIVER.setTargetFramerate(-1);
    curTest_ = test;
    frameCtr_ = FrameCounter(frameCtr_.getUpdateInterval());
}

void dskBenchmark::finishTest()
{
    using namespace boost::chrono;
    LOG.write("Benchmark #%1% took %2%. -> %3%/frame\n") % curTest_ % duration_cast<duration<float> >(frameCtr_.getCurIntervalLength())
      % duration_cast<milliseconds>(frameCtr_.getCurIntervalLength() / frameCtr_.getCurNumFrames());
    if(testDurations_[curTest_] == milliseconds::zero())
        testDurations_[curTest_] = duration_cast<milliseconds>(frameCtr_.getCurIntervalLength());
    else
        testDurations_[curTest_] = duration_cast<milliseconds>(testDurations_[curTest_] + frameCtr_.getCurIntervalLength()) / 2;

    std::vector<Window*> ctrls = GetCtrls<Window>();
    BOOST_FOREACH(Window* ctrl, ctrls)
    {
        if(ctrl->GetID() >= ID_first)
            DeleteCtrl(ctrl->GetID());
    }
    rects_.clear();
    lines_.clear();
    gameView_.reset();
    game_.reset();
    SetFpsDisplay(true);
    VIDEODRIVER.setTargetFramerate(0);
    if(!runAll_)
        curTest_ = TEST_NONE;
    else
    {
        curTest_ = Test(curTest_ + 1);
        if(curTest_ == TEST_CT)
            curTest_ = TEST_NONE;
        else
            startTest(curTest_);
    }
}

void dskBenchmark::createGame()
{
    RANDOM.Init(42);
    std::vector<PlayerInfo> players;
    PlayerInfo p;
    p.ps = PS_OCCUPIED;
    p.nation = NAT_AFRICANS;
    p.color = PLAYER_COLORS[0];
    players.push_back(p);
    p.nation = NAT_JAPANESE;
    p.color = PLAYER_COLORS[1];
    players.push_back(p);
    game_.reset(new Game(GlobalGameSettings(), 0u, players));
    try
    {
        GameDataLoader gdLoader(game_->world.GetDescriptionWriteable());
        if(!gdLoader.Load())
            throw "Game data";
        game_->world.Init(MapExtent(128, 128));
        const WorldDescription& desc = game_->world.GetDescription();
        DescIdx<TerrainDesc> lastTerrain(0);
        int lastHeight = 10;
        boost::random::mt19937 rng(42);
        using boost::random::uniform_int_distribution;
        uniform_int_distribution<int> percentage(0, 100);
        uniform_int_distribution<int> randTerrain(0, desc.terrain.size() / 2);
        RTTR_FOREACH_PT(MapPoint, game_->world.GetSize())
        {
            MapNode& node = game_->world.GetNodeWriteable(pt);
            DescIdx<TerrainDesc> t;
            // 90% chance of using the same terrain
            if(percentage(rng) <= 90)
                t = lastTerrain;
            else
                t.value = randTerrain(rng);
            node.t1 = t;
            lastTerrain = t;
            if(percentage(rng) <= 90)
                t = lastTerrain;
            else
                t.value = randTerrain(rng);
            node.t2 = t;
            lastTerrain = t;
            if(percentage(rng) <= 70)
                lastHeight = helpers::clamp(lastHeight + uniform_int_distribution<int>(-1, 1)(rng), 8, 13);
            node.altitude = lastHeight;
        }
        MapLoader::InitShadows(game_->world);
        MapLoader::SetMapExplored(game_->world);

        GameLoader loader(game_);
        if(!loader.load())
            throw "GUI";
    } catch(...)
    {
        game_.reset();
    }
}

void dskBenchmark::printTimes() const
{
    using namespace boost::chrono;
    milliseconds total(0);
    for(unsigned i = 1; i < testDurations_.size(); i++)
    {
        LOG.write("Benchmark #%1% took %2% -> %3%/frame\n") % i % duration_cast<duration<float> >(testDurations_[i])
          % duration_cast<milliseconds>(testDurations_[i] / numTestFrames);
        total += testDurations_[i];
    }
    LOG.write("Total benchmark time; %1% -> %2%/frame\n") % duration_cast<duration<float> >(total)
      % duration_cast<milliseconds>(total / numTestFrames);
}
