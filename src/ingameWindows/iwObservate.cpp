// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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
#include "iwObservate.h"
#include "CollisionDetection.h"
#include "Loader.h"
#include "Settings.h"
#include "controls/ctrlImageButton.h"
#include "driver/MouseCoords.h"
#include "drivers/VideoDriverWrapper.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "world/GameWorldBase.h"
#include "world/GameWorldView.h"
#include "world/GameWorldViewer.h"
#include "gameTypes/RoadBuildState.h"
#include "gameData/GuiConsts.h"
#include "libutil/Log.h"
#include <boost/foreach.hpp>
#include <cmath>

const Extent SmallWndSize(260, 190);
const Extent MediumWndSize(300, 250);
const Extent BigWndSize(340, 310);

iwObservate::iwObservate(GameWorldView& gwv, const MapPoint selectedPt)
    : IngameWindow(gwv.GetWorld().CreateGUIID(selectedPt), IngameWindow::posAtMouse, Extent(260, 190), _("Observation window"), NULL),
      parentView(gwv), view(new GameWorldView(gwv.GetViewer(), Point<int>(GetDrawPos() * DrawPoint(10, 15)), GetSize() - Extent::all(20))),
      selectedPt(selectedPt), lastWindowPos(Point<unsigned short>::Invalid()), isScrolling(false), zoomLvl(0),
      followMovableId(GameObject::INVALID_ID)
{
    view->MoveToMapPt(selectedPt);
    view->SetZoomFactor(1.9f, false);
    SetCloseOnRightClick(false);

    const Extent btSize(36, 36);
    DrawPoint btPos(GetSize().x / 2, GetSize().y);
    btPos -= DrawPoint(btSize.x * 2, 50);
    // Lupe: 36
    AddImageButton(1, btPos, btSize, TC_GREY, LOADER.GetImageN("io", 36), _("Zoom"));
    // Kamera (Folgen): 43
    btPos.x += btSize.x;
    AddImageButton(2, btPos, btSize, TC_GREY, LOADER.GetImageN("io", 43), _("Follow object"));
    // Zum Ort
    btPos.x += btSize.x;
    AddImageButton(3, btPos, btSize, TC_GREY, LOADER.GetImageN("io", 107), _("Go to place"));
    // Fenster vergroessern/verkleinern
    btPos.x += btSize.x;
    AddImageButton(4, btPos, btSize, TC_GREY, LOADER.GetImageN("io", 109), _("Resize window"));
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
            if(followMovableId != GameObject::INVALID_ID)
                followMovableId = GameObject::INVALID_ID;
            else
            {
                const DrawPoint centerDrawPt = DrawPoint(view->GetSize() / 2u);

                double minDistance = std::numeric_limits<double>::max();

                for(int y = view->GetFirstPt().y; y <= view->GetLastPt().y; ++y)
                {
                    for(int x = view->GetFirstPt().x; x <= view->GetLastPt().x; ++x)
                    {
                        Point<int> curOffset;
                        const MapPoint curPt = view->GetViewer().GetTerrainRenderer().ConvertCoords(Point<int>(x, y), &curOffset);
                        DrawPoint curDrawPt = view->GetWorld().GetNodePos(curPt) - view->GetOffset() + curOffset;

                        if(view->GetViewer().GetVisibility(curPt) != VIS_VISIBLE)
                            continue;

                        const std::list<noBase*>& figures = view->GetWorld().GetFigures(curPt);

                        BOOST_FOREACH(const noBase* obj, figures)
                        {
                            const noMovable* movable = dynamic_cast<const noMovable*>(obj);
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
        case 3: parentView.MoveToMapPt(MapPoint(view->GetLastPt() - (view->GetLastPt() - view->GetFirstPt()) / 2)); break;
        case 4:
            int diff = GetSize().x;

            if(GetSize().x == 260)
            {
                SetWidth(300);
                SetHeight(250);
            } else if(GetSize().x == 300)
            {
                SetWidth(340);
                SetHeight(310);
                GetCtrl<ctrlImageButton>(4)->SetImage(LOADER.GetImageN("io", 108));
            } else
            {
                SetWidth(260);
                SetHeight(190);
                GetCtrl<ctrlImageButton>(4)->SetImage(LOADER.GetImageN("io", 109));
            }

            diff -= GetSize().x;
            diff /= 2;

            view->Resize(GetSize() - Extent::all(20));

            for(unsigned i = 1; i <= 4; ++i)
                GetCtrl<ctrlImageButton>(i)->SetPos(DrawPoint(GetCtrl<ctrlImageButton>(i)->GetPos().x - diff, GetSize().y - 50));

            DrawPoint maxPos(VIDEODRIVER.GetScreenSize() - GetSize() - Extent::all(1));
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

    if(followMovableId != GameObject::INVALID_ID)
    {
        if(!MoveToFollowedObj())
            followMovableId = GameObject::INVALID_ID;
    }

    if(!IsMinimized())
    {
        RoadBuildState road;
        road.mode = RM_DISABLED;

        view->Draw(road, parentView.GetSelectedPt(), false);
        // Draw indicator for center point
        if(followMovableId == GameObject::INVALID_ID)
            LOADER.GetMapImageN(23)->DrawFull(view->GetPos() + view->GetSize() / 2u);
    }

    return IngameWindow::Draw_();
}

bool iwObservate::MoveToFollowedObj()
{
    // First look around the center (figure is normally still there)
    const MapPoint centerPt = view->GetWorld().MakeMapPoint((view->GetFirstPt() + view->GetLastPt()) / 2);
    const std::vector<MapPoint> centerPts = view->GetWorld().GetPointsInRadiusWithCenter(centerPt, 2);
    BOOST_FOREACH(const MapPoint& curPt, centerPts)
    {
        if(MoveToFollowedObj(curPt))
            return true;
    }

    // Not at the center (normally due to lags) -> Check full area
    for(int y = view->GetFirstPt().y; y <= view->GetLastPt().y; ++y)
    {
        for(int x = view->GetFirstPt().x; x <= view->GetLastPt().x; ++x)
        {
            const MapPoint curPt = view->GetWorld().MakeMapPoint(Point<int>(x, y));
            if(MoveToFollowedObj(curPt))
                return true;
        }
    }
    return false;
}

bool iwObservate::MoveToFollowedObj(const MapPoint ptToCheck)
{
    if(view->GetViewer().GetVisibility(ptToCheck) != VIS_VISIBLE)
        return false;
    const std::list<noBase*>& curObjs = view->GetWorld().GetFigures(ptToCheck);
    BOOST_FOREACH(const noBase* obj, curObjs)
    {
        if(obj->GetObjId() == followMovableId)
        {
            const noMovable* followMovable = static_cast<const noMovable*>(obj);
            DrawPoint drawPt = view->GetWorld().GetNodePos(ptToCheck);

            if(followMovable->IsMoving())
                drawPt += followMovable->CalcWalkingRelative();

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

        view->MoveTo((DrawPoint(mc.x, mc.y) - scrollOrigin));
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
        followMovableId = GameObject::INVALID_ID;
    } else
    {
        Close();
    }

    return (false);
}

bool iwObservate::Msg_RightUp(const MouseCoords& /*mc*/)
{
    isScrolling = false;

    return (false);
}
