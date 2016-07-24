// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#include "defines.h" // IWYU pragma: keep
#include "iwObservate.h"
#include "Loader.h"
#include "driver/src/MouseCoords.h"
#include "drivers/VideoDriverWrapper.h"
#include "world/GameWorldView.h"
#include "world/GameWorldViewer.h"
#include "world/GameWorldBase.h"
#include "Settings.h"
#include "controls/ctrlButton.h"
#include "gameTypes/RoadBuildState.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "libutil/src/Log.h"

// 260x190, 300x250, 340x310

iwObservate::iwObservate(GameWorldView& gwv, const MapPoint selectedPt):
    IngameWindow(gwv.GetWorld().CreateGUIID(selectedPt), 0xFFFE, 0xFFFE, 300, 250, _("Observation window"), NULL),
    parentView(gwv),
    view(new GameWorldView(gwv.GetViewer(), Point<int>(GetX() + 10, GetY() + 15), 300 - 20, 250 - 20)),
    selectedPt(selectedPt), last_x(-1), last_y(-1), scroll(false), zoomLvl(0), followPoint(MapPoint::Invalid()), followMovableId(0xFFFFFFFF)
{
    view->MoveToMapPt(selectedPt);
    SetCloseOnRightClick(false);

    // Lupe: 36
    AddImageButton(1, GetWidth() / 2 - 36 * 2, GetHeight() - 50, 36, 36, TC_GREY, LOADER.GetImageN("io", 36));
    // Kamera (Folgen): 43
    AddImageButton(2, GetWidth() / 2 - 36, GetHeight() - 50, 36, 36, TC_GREY, LOADER.GetImageN("io", 43));
    // Zum Ort
    AddImageButton(3, GetWidth() / 2, GetHeight() - 50, 36, 36, TC_GREY, LOADER.GetImageN("io", 107));
    // Fenster vergroessern/verkleinern
    AddImageButton(4, GetWidth() / 2 + 36, GetHeight() - 50, 36, 36, TC_GREY, LOADER.GetImageN("io", 109));
}

void iwObservate::Msg_ButtonClick(const unsigned int ctrl_id)
{
    switch (ctrl_id)
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
            if (followMovableId == 0xFFFFFFFF)
            {
                followPoint = MapPoint((view->GetFirstPt() + view->GetLastPt()) / 2);
            } else
            {
                followMovableId = 0xFFFFFFFF;
                followPoint = MapPoint::Invalid();
            }

            break;
        }
        case 3:
            parentView.MoveToMapPt( MapPoint(view->GetLastPt() - (view->GetLastPt() - view->GetFirstPt()) / 2) );
            break;
        case 4:
            int diff = width_;

            if (width_ == 260)
            {
                SetWidth(300);
                SetIwHeight(250);
            }
            else if (width_ == 300)
            {
                SetWidth(340);
                SetIwHeight(310);
                GetCtrl<ctrlImageButton>(4)->SetImage(LOADER.GetImageN("io", 108));
            }
            else
            {
                SetWidth(260);
                SetIwHeight(190);
                GetCtrl<ctrlImageButton>(4)->SetImage(LOADER.GetImageN("io", 109));
            }

            diff -= width_;
            diff /= 2;

            view->Resize(width_ - 20, height_ - 20);

            for (unsigned i = 1; i <= 4; ++i)
                GetCtrl<ctrlImageButton>(i)->Move(GetCtrl<ctrlImageButton>(i)->GetX(false) - diff, GetHeight() - 50);

            if (x_ + width_ >= VIDEODRIVER.GetScreenWidth())
            {
                Move(VIDEODRIVER.GetScreenWidth() - width_ - 1, y_);
            }

            if (y_ + height_ >= VIDEODRIVER.GetScreenHeight())
            {
                Move(x_, VIDEODRIVER.GetScreenHeight() - height_ - 1);
            }
    }
}

bool iwObservate::Draw_()
{
    if ((x_ != last_x) || (y_ != last_y))
    {
        view->SetPos(Point<int>(GetX() + 10, GetY() + 15));
        last_x = x_;
        last_y = y_;
    }

    // For now, this workaround seems to be needed, as the list seems to sometimes
    // contain invalid node objects in the button handler.
    if (followPoint.isValid())
    {
        std::vector<MapPoint> pts = view->GetWorld().GetPointsInRadius(followPoint, 10);

        // look at our current center as well
        pts.insert(pts.begin(), followPoint);

        for(std::vector<MapPoint>::const_iterator it = pts.begin(); it != pts.end(); ++it)
        {
            Visibility visibility = view->GetViewer().GetVisibility(*it);

            if(visibility != VIS_VISIBLE)
                continue;

            std::vector<noBase*> movables(parentView.GetWorld().GetDynamicObjectsFrom(*it));

           if (!movables.empty())
            {
                followMovableId = movables.front()->GetObjId();
                break;
            }
        }

        followPoint = MapPoint::Invalid();
    }

    if (followMovableId != 0xFFFFFFFF)
    {
        const noMovable *movable = NULL;
        
        for(int y = view->GetFirstPt().y; y <= view->GetLastPt().y; ++y)
        {
            for(int x = view->GetFirstPt().x; x <= view->GetLastPt().x; ++x)
            {
                Point<int> curOffset;
                const MapPoint curPt = view->GetViewer().GetTerrainRenderer().ConvertCoords(Point<int>(x, y), &curOffset);

                Visibility visibility = view->GetViewer().GetVisibility(curPt);

                if(visibility != VIS_VISIBLE)
                    continue;

                const std::list<noBase*>& figures = view->GetWorld().GetNode(curPt).figures;

                for(std::list<noBase*>::const_iterator it = figures.begin(); it != figures.end(); ++it)
                {
                    if ((*it)->GetObjId() == followMovableId)
                    {
                        movable = static_cast<noMovable*>(*it);

                        DrawPoint drawPt = view->GetWorld().GetNodePos(curPt);

                        if (movable->IsMoving())
                            drawPt += movable->CalcWalkingRelative();

                        view->MoveTo(drawPt.x - (width_ - 20) / 2, drawPt.y - (height_ - 20) / 2, true);
                    }
                }
            }
        }

        if (movable == NULL)
        {
            followMovableId = 0xFFFFFFFF;
        }
    }

    if (!GetMinimized())
    {
        RoadBuildState road;
        road.mode = RM_DISABLED;

        view->Draw(road, true, parentView.GetSelectedPt());
    }

    bool res = IngameWindow::Draw_();

    if (followMovableId == 0xFFFFFFFF)
    {
        LOADER.GetMapImageN(23)->Draw(DrawPoint(GetX() + 10 + (width_ - 20) / 2, GetY() + 15 + (height_ - 20) / 2));
    }

    return res;
}

bool iwObservate::Msg_MouseMove(const MouseCoords& mc)
{
    if (scroll)
    {
        int acceleration = SETTINGS.global.smartCursor ? 2 : 3;

        if(SETTINGS.interface.revert_mouse)
            acceleration = -acceleration;

        view->MoveTo((mc.x - sx) * acceleration, (mc.y - sy) * acceleration);
        VIDEODRIVER.SetMousePos(sx, sy);
    }

    return(false);
}

bool iwObservate::Msg_RightDown(const MouseCoords& mc)
{
    sx = mc.x;
    sy = mc.y;

    scroll = true;
    followMovableId = 0xFFFFFFFF;

    return(false);
}

bool iwObservate::Msg_RightUp(const MouseCoords&  /*mc*/)
{
    scroll = false;

    return(false);
}
