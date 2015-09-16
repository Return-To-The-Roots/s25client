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

#include "defines.h"
#include "iwObservate.h"

#include "Loader.h"
#include "drivers/VideoDriverWrapper.h"
#include "GameClient.h"
#include "WindowManager.h"
#include "GameWorld.h"
#include "iwMsgbox.h"
#include "desktops/dskGameInterface.h"
#include "Settings.h"
#include "controls/ctrlButton.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// 260x190, 300x250, 340x310

//IngameWindow::IngameWindow(unsigned int id, const MapPoint pt, unsigned short width, unsigned short height, const std::string& title, glArchivItem_Bitmap *background, bool modal)
iwObservate::iwObservate(GameWorldViewer* const gwv, const MapPoint selectedPt)
    : IngameWindow(gwv->CreateGUIID(selectedPt), 0xFFFE, 0xFFFE, 300, 250, _("Observation window"), NULL),
      view(new GameWorldView(MapPoint(GetX() + 10, GetY() + 15), 300 - 20, 250 - 20)), selectedPt(selectedPt), last_x(-1), last_y(-1), scroll(false)
{
    view->SetGameWorldViewer(gwv);
    view->MoveToMapObject(selectedPt);
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
            break;
        case 2:
            break;
        case 3:
            view->GetGameWorldViewer()->MoveToMapObject(
                MapPoint(view->GetLastPt() - (view->GetLastPt() - view->GetFirstPt()) / 2)
                );
            break;
        case 4:
            int diff = width;

            if (width == 260)
            {
                SetWidth(300);
                SetIwHeight(250);
            }
            else if (width == 300)
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

            diff -= width;
            diff /= 2;

            view->Resize(width - 20, height - 20);

            for (unsigned i = 1; i <= 4; ++i)
                GetCtrl<ctrlImageButton>(i)->Move(GetCtrl<ctrlImageButton>(i)->GetX(false) - diff, GetHeight() - 50);

            if (x + width >= VIDEODRIVER.GetScreenWidth())
            {
                Move(VIDEODRIVER.GetScreenWidth() - width - 1, y);
            }

            if (y + height >= VIDEODRIVER.GetScreenHeight())
            {
                Move(x, VIDEODRIVER.GetScreenHeight() - height - 1);
            }
    }
}

bool iwObservate::Draw_()
{
    if ((x != last_x) || (y != last_y))
    {
        view->SetPos(MapPoint(GetX() + 10, GetY() + 15));
        last_x = x;
        last_y = y;
    }

    if (!GetMinimized())
    {
        RoadsBuilding road;

        road.mode = RM_DISABLED;
        road.point = MapPoint(0, 0);
        road.start = MapPoint(0, 0);

        view->Draw(GAMECLIENT.GetPlayerID(), NULL, true, view->GetGameWorldViewer()->GetSel(), road);
    }

    return(IngameWindow::Draw_());
}

bool iwObservate::Msg_MouseMove(const MouseCoords& mc)
{
    if (scroll)
    {
        if(SETTINGS.interface.revert_mouse)
            view->MoveTo( ( sx - mc.x) * 2,  ( sy - mc.y) * 2);
        else
            view->MoveTo(-( sx - mc.x) * 2, -( sy - mc.y) * 2);
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

bool iwObservate::Msg_RightUp(const MouseCoords& mc)
{
    scroll = false;

    return(false);
}


