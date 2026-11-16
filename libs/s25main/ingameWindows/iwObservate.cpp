// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwObservate.h"
#include "CollisionDetection.h"
#include "Loader.h"
#include "Settings.h"
#include "WindowManager.h"
#include "controls/ctrlImageButton.h"
#include "controls/ctrlTimer.h"
#include "driver/MouseCoords.h"
#include "drivers/VideoDriverWrapper.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "world/GameWorldBase.h"
#include "world/GameWorldView.h"
#include "world/GameWorldViewer.h"
#include "nodeObjs/noMovable.h"
#include "gameTypes/RoadBuildState.h"
#include "gameData/const_gui_ids.h"
#include <cmath>

const Extent SmallWndSize(260, 190);
const Extent MediumWndSize(300, 250);
const Extent BigWndSize(340, 310);

enum
{
    ID_tmrUpdateFollow = 5
};

iwObservate::iwObservate(GameWorldView& gwv, const MapPoint selectedPt)
    : IngameWindow(CGI_OBSERVATION, IngameWindow::posAtMouse, SmallWndSize, _("Observation window"), nullptr, false,
                   CloseBehavior::NoRightClick),
      parentView(gwv),
      view(new GameWorldView(gwv.getViewer(), Position(getDrawPos() * DrawPoint(10, 15)), getSize() - Extent::all(20))),
      selectedPt(selectedPt), lastWindowPos(Point<unsigned short>::Invalid()), isScrolling(false), zoomLvl(0),
      followMovableId(0)
{
    view->MoveToMapPt(selectedPt);
    view->setZoomFactor(1.9f, false);

    const Extent btSize(36, 36);
    DrawPoint btPos(getSize().x / 2, getSize().y);
    btPos -= DrawPoint(btSize.x * 2, 50);
    // Lupe: 36
    AddImageButton(1, btPos, btSize, TextureColor::Grey, LOADER.getImageN("io", 36), _("Zoom"));
    // Kamera (Folgen): 43
    btPos.x += btSize.x;
    AddImageButton(2, btPos, btSize, TextureColor::Grey, LOADER.getImageN("io", 43), _("Follow object"));
    // Zum Ort
    btPos.x += btSize.x;
    AddImageButton(3, btPos, btSize, TextureColor::Grey, LOADER.getImageN("io", 107), _("Go to place"));
    // Fenster vergroessern/verkleinern
    btPos.x += btSize.x;
    AddImageButton(4, btPos, btSize, TextureColor::Grey, LOADER.getImageN("io", 109), _("Resize window"));

    // Synchronize visibility of HUD elements with parentView
    parentView.CopyHudSettingsTo(*view, false);
    gwvSettingsConnection =
      parentView.onHudSettingsChanged.connect([this]() { parentView.CopyHudSettingsTo(*view, false); });

    // Check the followed object periodically
    using namespace std::chrono_literals;
    AddTimer(ID_tmrUpdateFollow, 1s)->Stop();
}

void iwObservate::msgButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 1:
            if(++zoomLvl > 4)
                zoomLvl = 0;
            if(zoomLvl == 0)
                view->setZoomFactor(1.f);
            else if(zoomLvl == 1)
                view->setZoomFactor(1.3f);
            else if(zoomLvl == 2)
                view->setZoomFactor(1.6f);
            else if(zoomLvl == 3)
                view->setZoomFactor(1.9f);
            else
                view->setZoomFactor(2.3f);
            break;
        case 2:
        {
            if(followMovableId)
            {
                followMovableId = 0;
                getCtrl<ctrlTimer>(ID_tmrUpdateFollow)->Stop();
            } else
            {
                const DrawPoint centerDrawPt = DrawPoint(view->getSize() / 2u);

                double minDistance = std::numeric_limits<double>::max();

                for(int y = view->getFirstPt().y; y <= view->getLastPt().y; ++y)
                {
                    for(int x = view->getFirstPt().x; x <= view->getLastPt().x; ++x)
                    {
                        Position curOffset;
                        const MapPoint curPt =
                          view->getViewer().getTerrainRenderer().ConvertCoords(Position(x, y), &curOffset);
                        DrawPoint curDrawPt = view->getWorld().getNodePos(curPt) - view->getOffset() + curOffset;

                        if(view->getViewer().getVisibility(curPt) != Visibility::Visible)
                            continue;

                        for(const noBase& obj : view->getWorld().getFigures(curPt))
                        {
                            const auto* movable = dynamic_cast<const noMovable*>(&obj);
                            if(!movable)
                                continue;

                            DrawPoint objDrawPt = curDrawPt;

                            if(movable->isMoving())
                                objDrawPt += movable->CalcWalkingRelative();

                            DrawPoint diffToCenter = objDrawPt - centerDrawPt;
                            double distance = sqrt(pow(diffToCenter.x, 2) + pow(diffToCenter.y, 2));

                            if(distance < minDistance)
                            {
                                followMovableId = movable->getObjId();
                                minDistance = distance;
                            }
                        }
                    }
                }
                if(followMovableId)
                    getCtrl<ctrlTimer>(ID_tmrUpdateFollow)->Start();
            }

            break;
        }
        case 3:
            parentView.MoveToMapPt(MapPoint(view->getLastPt() - (view->getLastPt() - view->getFirstPt()) / 2));
            break;
        case 4:
            int diff = getSize().x;

            if(getSize() == SmallWndSize)
            {
                Resize(MediumWndSize);
            } else if(getSize() == MediumWndSize)
            {
                Resize(BigWndSize);
                getCtrl<ctrlImageButton>(4)->setImage(LOADER.getImageN("io", 108));
            } else
            {
                Resize(SmallWndSize);
                getCtrl<ctrlImageButton>(4)->setImage(LOADER.getImageN("io", 109));
            }

            diff -= getSize().x;
            diff /= 2;

            view->Resize(getSize() - Extent::all(20));

            for(unsigned i = 1; i <= 4; ++i)
                getCtrl<ctrlImageButton>(i)->setPos(
                  DrawPoint(getCtrl<ctrlImageButton>(i)->getPos().x - diff, getSize().y - 50));
    }
}

void iwObservate::msgTimer(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case ID_tmrUpdateFollow:
            if(followMovableId && !MoveToFollowedObj())
            {
                followMovableId = 0;
                getCtrl<ctrlTimer>(ID_tmrUpdateFollow)->Stop();
            }
            break;
    }
}

void iwObservate::DrawBackground()
{
    if(getPos() != lastWindowPos)
    {
        view->setPos(getPos() + DrawPoint(10, 15));
        lastWindowPos = getPos();
    }

    RoadBuildState road;
    road.mode = RoadBuildMode::Disabled;

    view->Draw(road, parentView.getSelectedPt(), false);
    // Draw indicator for center point
    if(!followMovableId)
        LOADER.getMapTexture(23)->DrawFull(view->getPos() + view->getSize() / 2u);
}

bool iwObservate::MoveToFollowedObj()
{
    // First look around the center (figure is normally still there)
    const GameWorldBase& world = view->getWorld();
    const MapPoint centerPt = world.MakeMapPoint((view->getFirstPt() + view->getLastPt()) / 2);
    const std::vector<MapPoint> centerPts = world.GetPointsInRadiusWithCenter(centerPt, 2);
    for(const MapPoint& curPt : centerPts)
    {
        if(MoveToFollowedObj(curPt))
            return true;
    }

    // Not at the center (normally due to lags) -> Check full area
    for(int y = view->getFirstPt().y; y <= view->getLastPt().y; ++y)
    {
        for(int x = view->getFirstPt().x; x <= view->getLastPt().x; ++x)
        {
            const MapPoint curPt = world.MakeMapPoint(Position(x, y));
            if(MoveToFollowedObj(curPt))
                return true;
        }
    }
    return false;
}

bool iwObservate::MoveToFollowedObj(const MapPoint ptToCheck)
{
    if(view->getViewer().getVisibility(ptToCheck) != Visibility::Visible)
        return false;
    for(const noBase& obj : view->getWorld().getFigures(ptToCheck))
    {
        if(obj.getObjId() == followMovableId)
        {
            const auto& followMovable = static_cast<const noMovable&>(obj);
            DrawPoint drawPt = view->getWorld().getNodePos(ptToCheck);

            if(followMovable.isMoving())
                drawPt += followMovable.CalcWalkingRelative();

            view->MoveTo(drawPt - view->getSize() / 2u);
            return true;
        }
    }
    return false;
}

bool iwObservate::msgMouseMove(const MouseCoords& mc)
{
    if(isScrolling)
    {
        int acceleration = SETTINGS.global.smartCursor ? 2 : 3;

        if(SETTINGS.interface.mapScrollMode == MapScrollMode::ScrollSame)
            acceleration = -acceleration;

        view->MoveBy((mc.pos - scrollOrigin) * acceleration);
        VIDEODRIVER.setMousePos(scrollOrigin);
        return true;
    } else
        return IngameWindow::msgMouseMove(mc);
}

bool iwObservate::msgRightDown(const MouseCoords& mc)
{
    if(IsPointInRect(mc.pos, Rect(view->getPos(), view->getSize()))
       && !IsPointInRect(mc.pos, getCtrl<ctrlImageButton>(1)->getDrawRect())
       && !IsPointInRect(mc.pos, getCtrl<ctrlImageButton>(2)->getDrawRect())
       && !IsPointInRect(mc.pos, getCtrl<ctrlImageButton>(3)->getDrawRect())
       && !IsPointInRect(mc.pos, getCtrl<ctrlImageButton>(4)->getDrawRect()))
    {
        scrollOrigin = mc.pos;

        isScrolling = true;
        followMovableId = 0;
        WINDOWMANAGER.setCursor(Cursor::Scroll);
    } else
    {
        Close();
    }

    return true;
}

bool iwObservate::msgRightUp(const MouseCoords& /*mc*/)
{
    if(isScrolling)
        WINDOWMANAGER.setCursor(Cursor::Hand);
    isScrolling = false;

    return true;
}
