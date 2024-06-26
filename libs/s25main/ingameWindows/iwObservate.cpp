// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwObservate.h"
#include "CollisionDetection.h"
#include "Loader.h"
#include "PickedMovableObject.h"
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

iwObservate::iwObservate(GameWorldView& gwv, const MapPoint selectedPt, PickedMovableObject&& pmo)
    : IngameWindow(CGI_OBSERVATION, IngameWindow::posAtMouse, SmallWndSize, _("Observation window"), nullptr, false,
                   CloseBehavior::NoRightClick),
      parentView(gwv),
      view(new GameWorldView(gwv.GetViewer(), Position(GetDrawPos() * DrawPoint(10, 15)), GetSize() - Extent::all(20))),
      selectedPt(selectedPt), lastWindowPos(Point<unsigned short>::Invalid()), isScrolling(false), zoomLvl(0),
      pickedObject(std::move(pmo)), following(false), lastValid(pickedObject.isValid())
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
    AddImageButton(2, btPos, btSize, TextureColor::Grey, LOADER.GetImageN("io", 43));
    // Zum Ort
    btPos.x += btSize.x;
    AddImageButton(3, btPos, btSize, TextureColor::Grey, LOADER.GetImageN("io", 107), _("Go to place"));
    // Fenster vergroessern/verkleinern
    btPos.x += btSize.x;
    AddImageButton(4, btPos, btSize, TextureColor::Grey, LOADER.GetImageN("io", 109), _("Resize window"));

    // Synchronize visibility of HUD elements with parentView
    parentView.CopyHudSettingsTo(*view, false);
    gwvSettingsConnection =
      parentView.onHudSettingsChanged.connect([this]() { parentView.CopyHudSettingsTo(*view, false); });

    // Set follow button tooltip
    UpdateFollowButton();
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
            if(following)
                pickedObject.invalidate();   // Stop following
            else if(!pickedObject.isValid()) // If object is invalid, pick new object at center of view
                pickedObject = PickedMovableObject::pickAtViewCenter(*view, false);

            // Follow picked object, if valid
            following = lastValid = pickedObject.isValid();
            // Ensure the object doesn't expire if we started following the initially picked object
            pickedObject.cancelExpiration();
            UpdateFollowButton();
            break;
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
    }
}

void iwObservate::Draw_()
{
    if(GetPos() != lastWindowPos)
    {
        view->SetPos(GetPos() + DrawPoint(10, 15));
        lastWindowPos = GetPos();
    }

    // If the object was valid previously, track it (checks isValid() for us)
    // If it returns false, it either expired or we lost it
    if(lastValid && !pickedObject.track(*view, following))
    {
        following = lastValid = false;
        UpdateFollowButton();
    }

    if(!IsMinimized())
    {
        RoadBuildState road;
        road.mode = RoadBuildMode::Disabled;

        view->Draw(road, parentView.GetSelectedPt(), false);
        // Draw indicator for center point
        if(!following && !lastValid)
            LOADER.GetMapTexture(23)->DrawFull(view->GetPos() + view->GetSize() / 2u);
    }

    return IngameWindow::Draw_();
}

bool iwObservate::Msg_MouseMove(const MouseCoords& mc)
{
    if(isScrolling)
    {
        int acceleration = SETTINGS.global.smartCursor ? 2 : 3;

        if(SETTINGS.interface.invertMouse)
            acceleration = -acceleration;

        view->MoveBy((mc.GetPos() - scrollOrigin) * acceleration);
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
        following = lastValid = false;
        pickedObject.invalidate();
        UpdateFollowButton();
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

void iwObservate::UpdateFollowButton()
{
    auto* button = GetCtrl<ctrlImageButton>(2);
    if(following)
        button->SetTooltip(_("Stop following object"));
    else if(pickedObject.isValid())
        button->SetTooltip(_("Follow picked object"));
    else
        button->SetTooltip(_("Follow object near the center"));
}
