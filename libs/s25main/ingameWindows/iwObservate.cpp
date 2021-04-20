// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwObservate.h"
#include "CollisionDetection.h"
#include "Loader.h"
#include "Settings.h"
#include "WindowManager.h"
#include "controls/ctrlImageButton.h"
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

iwObservate::iwObservate(GameWorldView& gwv, const MapPoint selectedPt)
    : IngameWindow(CGI_OBSERVATION, IngameWindow::posAtMouse, SmallWndSize, _("Observation window"), nullptr, false,
                   false),
      parentView(gwv),
      view(new GameWorldView(gwv.GetViewer(), Position(GetDrawPos() * DrawPoint(10, 15)), GetSize() - Extent::all(20))),
      selectedPt(selectedPt), lastWindowPos(Point<unsigned short>::Invalid()), isScrolling(false), zoomLvl(0),
      followMovableId(0)
{
    view->MoveToMapPt(selectedPt);
    view->SetZoomFactor(1.9f, false);

    const Extent btSize(36, 36);
    DrawPoint btPos(GetSize().x / 2, GetSize().y);
    btPos -= DrawPoint(btSize.x * 2, 50);
    // Lupe: 36
    AddImageButton(1, btPos, btSize, TextureColor::Grey, LOADER.GetImageN("io", 36), _("Zoom"));
    // Kamera (Folgen): 43
    btPos.x += btSize.x;
    AddImageButton(2, btPos, btSize, TextureColor::Grey, LOADER.GetImageN("io", 43), _("Follow object"));
    // Zum Ort
    btPos.x += btSize.x;
    AddImageButton(3, btPos, btSize, TextureColor::Grey, LOADER.GetImageN("io", 107), _("Go to place"));
    // Fenster vergroessern/verkleinern
    btPos.x += btSize.x;
    AddImageButton(4, btPos, btSize, TextureColor::Grey, LOADER.GetImageN("io", 109), _("Resize window"));
}

void iwObservate::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 1:
            if(++zoomLvl > 4)
                zoomLvl = 0;
            if(zoomLvl == 0)
                view->SetZoomFactor(1.f);
            else if(zoomLvl == 1)
                view->SetZoomFactor(1.3f);
            else if(zoomLvl == 2)
                view->SetZoomFactor(1.6f);
            else if(zoomLvl == 3)
                view->SetZoomFactor(1.9f);
            else
                view->SetZoomFactor(2.3f);
            break;
        case 2:
        {
            if(followMovableId)
                followMovableId = 0;
            else
            {
                const DrawPoint centerDrawPt = DrawPoint(view->GetSize() / 2u);

                double minDistance = std::numeric_limits<double>::max();

                for(int y = view->GetFirstPt().y; y <= view->GetLastPt().y; ++y)
                {
                    for(int x = view->GetFirstPt().x; x <= view->GetLastPt().x; ++x)
                    {
                        Position curOffset;
                        const MapPoint curPt =
                          view->GetViewer().GetTerrainRenderer().ConvertCoords(Position(x, y), &curOffset);
                        DrawPoint curDrawPt = view->GetWorld().GetNodePos(curPt) - view->GetOffset() + curOffset;

                        if(view->GetViewer().GetVisibility(curPt) != Visibility::Visible)
                            continue;

                        for(const noBase& obj : view->GetWorld().GetFigures(curPt))
                        {
                            const auto* movable = dynamic_cast<const noMovable*>(&obj);
                            if(!movable)
                                continue;

                            DrawPoint objDrawPt = curDrawPt;

                            if(movable->IsMoving())
                                objDrawPt += movable->CalcWalkingRelative();

                            DrawPoint diffToCenter = objDrawPt - centerDrawPt;
                            double distance = sqrt(pow(diffToCenter.x, 2) + pow(diffToCenter.y, 2));

                            if(distance < minDistance)
                            {
                                followMovableId = movable->GetObjId();
                                minDistance = distance;
                            }
                        }
                    }
                }
            }

            break;
        }
        case 3:
            parentView.MoveToMapPt(MapPoint(view->GetLastPt() - (view->GetLastPt() - view->GetFirstPt()) / 2));
            break;
        case 4:
            int diff = GetSize().x;

            if(GetSize() == SmallWndSize)
            {
                Resize(MediumWndSize);
            } else if(GetSize() == MediumWndSize)
            {
                Resize(BigWndSize);
                GetCtrl<ctrlImageButton>(4)->SetImage(LOADER.GetImageN("io", 108));
            } else
            {
                Resize(SmallWndSize);
                GetCtrl<ctrlImageButton>(4)->SetImage(LOADER.GetImageN("io", 109));
            }

            diff -= GetSize().x;
            diff /= 2;

            view->Resize(GetSize() - Extent::all(20));

            for(unsigned i = 1; i <= 4; ++i)
                GetCtrl<ctrlImageButton>(i)->SetPos(
                  DrawPoint(GetCtrl<ctrlImageButton>(i)->GetPos().x - diff, GetSize().y - 50));

            DrawPoint maxPos(VIDEODRIVER.GetRenderSize() - GetSize() - Extent::all(1));
            DrawPoint newPos = elMin(maxPos, GetPos());
            if(newPos != GetPos())
                SetPos(newPos);
    }
}

void iwObservate::Draw_()
{
    if(GetPos() != lastWindowPos)
    {
        view->SetPos(GetPos() + DrawPoint(10, 15));
        lastWindowPos = GetPos();
    }

    if(followMovableId)
    {
        if(!MoveToFollowedObj())
            followMovableId = 0;
    }

    if(!IsMinimized())
    {
        RoadBuildState road;
        road.mode = RoadBuildMode::Disabled;

        view->Draw(road, parentView.GetSelectedPt(), false);
        // Draw indicator for center point
        if(!followMovableId)
            LOADER.GetMapImageN(23)->DrawFull(view->GetPos() + view->GetSize() / 2u);
    }

    return IngameWindow::Draw_();
}

bool iwObservate::MoveToFollowedObj()
{
    // First look around the center (figure is normally still there)
    const GameWorldBase& world = view->GetWorld();
    const MapPoint centerPt = world.MakeMapPoint((view->GetFirstPt() + view->GetLastPt()) / 2);
    const std::vector<MapPoint> centerPts = world.GetPointsInRadiusWithCenter(centerPt, 2);
    for(const MapPoint& curPt : centerPts)
    {
        if(MoveToFollowedObj(curPt))
            return true;
    }

    // Not at the center (normally due to lags) -> Check full area
    for(int y = view->GetFirstPt().y; y <= view->GetLastPt().y; ++y)
    {
        for(int x = view->GetFirstPt().x; x <= view->GetLastPt().x; ++x)
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
    if(view->GetViewer().GetVisibility(ptToCheck) != Visibility::Visible)
        return false;
    for(const noBase& obj : view->GetWorld().GetFigures(ptToCheck))
    {
        if(obj.GetObjId() == followMovableId)
        {
            const auto& followMovable = static_cast<const noMovable&>(obj);
            DrawPoint drawPt = view->GetWorld().GetNodePos(ptToCheck);

            if(followMovable.IsMoving())
                drawPt += followMovable.CalcWalkingRelative();

            view->MoveTo(drawPt - view->GetSize() / 2u, true);
            return true;
        }
    }
    return false;
}

bool iwObservate::Msg_MouseMove(const MouseCoords& mc)
{
    if(isScrolling)
    {
        int acceleration = SETTINGS.global.smartCursor ? 2 : 3;

        if(SETTINGS.interface.revert_mouse)
            acceleration = -acceleration;

        view->MoveTo((mc.GetPos() - scrollOrigin) * acceleration);
        VIDEODRIVER.SetMousePos(scrollOrigin);
    }

    return (false);
}

bool iwObservate::Msg_RightDown(const MouseCoords& mc)
{
    if(IsPointInRect(mc.GetPos(), Rect(view->GetPos(), view->GetSize()))
       && !IsPointInRect(mc.GetPos(), GetCtrl<ctrlImageButton>(1)->GetDrawRect())
       && !IsPointInRect(mc.GetPos(), GetCtrl<ctrlImageButton>(2)->GetDrawRect())
       && !IsPointInRect(mc.GetPos(), GetCtrl<ctrlImageButton>(3)->GetDrawRect())
       && !IsPointInRect(mc.GetPos(), GetCtrl<ctrlImageButton>(4)->GetDrawRect()))
    {
        scrollOrigin = mc.GetPos();

        isScrolling = true;
        followMovableId = 0;
        WINDOWMANAGER.SetCursor(Cursor::Scroll);
    } else
    {
        Close();
    }

    return true;
}

bool iwObservate::Msg_RightUp(const MouseCoords& /*mc*/)
{
    if(isScrolling)
        WINDOWMANAGER.SetCursor(Cursor::Hand);
    isScrolling = false;

    return true;
}
