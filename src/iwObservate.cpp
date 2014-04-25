// $Id: iwObservate.cpp 7414 2011-08-25 17:44:38Z marcus $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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

#include "main.h"
#include "iwObservate.h"

#include "Loader.h"
#include "VideoDriverWrapper.h"
#include "GameClient.h"
#include "WindowManager.h"
#include "GameWorld.h"
#include "iwMsgbox.h"
#include "GameCommands.h"
#include "dskGameInterface.h"
#include "Settings.h"
#include "ctrlButton.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// 260x190, 300x250, 340x310

//IngameWindow::IngameWindow(unsigned int id, unsigned short x, unsigned short y, unsigned short width, unsigned short height, const std::string& title, glArchivItem_Bitmap *background, bool modal)
iwObservate::iwObservate(GameWorldViewer* const gwv, const unsigned short selected_x, const unsigned short selected_y)
    : IngameWindow(gwv->CreateGUIID(selected_x, selected_y), 0xFFFE, 0xFFFE, 300, 250, _("Observation window"), NULL),
      view(new GameWorldView(gwv, GetX() + 10, GetY() + 15, 300 - 20, 250 - 20)), selected_x(selected_x), selected_y(selected_y), last_x(-1), last_y(-1), scroll(false)
{
    view->MoveToMapObject(selected_x, selected_y);
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
            view->GetGameWorldViewer()->MoveToMapObject(view->GetLastX() - (view->GetLastX() - view->GetFirstX()) / 2, view->GetLastY() - (view->GetLastY() - view->GetFirstY()) / 2);
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

            if (x + width >= VideoDriverWrapper::inst().GetScreenWidth())
            {
                Move(VideoDriverWrapper::inst().GetScreenWidth() - width - 1, y);
            }

            if (y + height >= VideoDriverWrapper::inst().GetScreenHeight())
            {
                Move(x, VideoDriverWrapper::inst().GetScreenHeight() - height - 1);
            }
    }
}

bool iwObservate::Draw_()
{
    if ((x != last_x) || (y != last_y))
    {
        view->SetX(GetX() + 10);
        view->SetY(GetY() + 15);
        last_x = x;
        last_y = y;
    }

    if (!GetMinimized())
    {
        RoadsBuilding road;

        road.mode = RM_DISABLED;
        road.point_x = 0;
        road.point_y = 0;
        road.start_x = 0;
        road.start_y = 0;

        view->Draw(GAMECLIENT.GetPlayerID(), NULL, true, view->GetGameWorldViewer()->GetSelX(), view->GetGameWorldViewer()->GetSelY(), road);
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
        VideoDriverWrapper::inst().SetMousePos(sx, sy);
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


