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

///////////////////////////////////////////////////////////////////////////////
// Header

#include "defines.h" // IWYU pragma: keep
#include "iwObservate.h"
#include "world/GameWorldViewer.h"
#include "Loader.h"
#include "driver/src/MouseCoords.h"
#include "drivers/VideoDriverWrapper.h"
#include "world/GameWorldView.h"
#include "Settings.h"
#include "controls/ctrlButton.h"
#include "gameTypes/RoadBuildState.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

// 260x190, 300x250, 340x310

iwObservate::iwObservate(GameWorldView& gwv, const MapPoint selectedPt):
    IngameWindow(gwv.GetViewer().CreateGUIID(selectedPt), 0xFFFE, 0xFFFE, 300, 250, _("Observation window"), NULL),
    parentView(gwv),
    view(new GameWorldView(gwv.GetViewer(), Point<int>(GetX() + 10, GetY() + 15), 300 - 20, 250 - 20)),
    selectedPt(selectedPt), last_x(-1), last_y(-1), scroll(false), zoomLvl(0)
{
    view->MoveToMapPt(selectedPt);
    SetCloseOnRightClick(false);

    // Lupe: 36
    AddImageButton(1, GetWidth() / 2 - 36 * 2, GetHeight() - 50, 36, 36, TC_BRICKS, LOADER.GetImageN("io", 36));
    // Kamera (Folgen): 43
    AddImageButton(2, GetWidth() / 2 - 36, GetHeight() - 50, 36, 36, TC_BRICKS, LOADER.GetImageN("io", 43));
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
            break;
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

    if (!GetMinimized())
    {
        RoadBuildState road;
        road.mode = RM_DISABLED;

        view->Draw(road, true, parentView.GetSelectedPt());
    }

    return IngameWindow::Draw_();
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

    return(false);
}

bool iwObservate::Msg_RightUp(const MouseCoords&  /*mc*/)
{
    scroll = false;

    return(false);
}
