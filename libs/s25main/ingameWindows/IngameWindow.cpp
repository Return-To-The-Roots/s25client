// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "IngameWindow.h"
#include "CollisionDetection.h"
#include "Loader.h"
#include "RTTR_Assert.h"
#include "Settings.h"
#include "WindowManager.h"
#include "driver/MouseCoords.h"
#include "drivers/VideoDriverWrapper.h"
#include "helpers/EnumRange.h"
#include "helpers/MultiArray.h"
#include "helpers/containerUtils.h"
#include "ogl/FontStyle.h"
#include "ogl/SoundEffectItem.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glFont.h"
#include "gameData/const_gui_ids.h"
#include "s25util/error.h"
#include <algorithm>
#include <utility>

namespace {
constexpr Extent ButtonSize(16, 16);
constexpr unsigned TitleMargin = 32;
} // namespace

const DrawPoint IngameWindow::posLastOrCenter(DrawPoint::MaxElementValue, DrawPoint::MaxElementValue);
const DrawPoint IngameWindow::posCenter(DrawPoint::MaxElementValue - 1, DrawPoint::MaxElementValue);
const DrawPoint IngameWindow::posAtMouse(DrawPoint::MaxElementValue - 1, DrawPoint::MaxElementValue - 1);

const Extent IngameWindow::borderSize(1, 1);
IngameWindow::IngameWindow(unsigned id, const DrawPoint& pos, const Extent& size, std::string title,
                           glArchivItem_Bitmap* background, bool modal, CloseBehavior closeBehavior, Window* parent)
    : Window(parent, id, pos, size), title_(std::move(title)), background(background), lastMousePos(0, 0),
      isModal_(modal), closeme(false), isPinned_(false), isMinimized_(false), isMoving(false),
      closeBehavior_(closeBehavior)
{
    std::fill(buttonStates_.begin(), buttonStates_.end(), ButtonState::Up);
    contentOffset.x = LOADER.GetImageN("resource", 38)->getWidth();     // left border
    contentOffset.y = LOADER.GetImageN("resource", 42)->getHeight();    // title bar
    contentOffsetEnd.x = LOADER.GetImageN("resource", 39)->getWidth();  // right border
    contentOffsetEnd.y = LOADER.GetImageN("resource", 40)->getHeight(); // bottom bar

    const auto it = SETTINGS.windows.persistentSettings.find(GetGUIID());
    windowSettings_ = (it == SETTINGS.windows.persistentSettings.cend() ? nullptr : &it->second);

    // For compatibility we treat the given height as the window height, not the content height
    // First we have to make sure the size is not to small
    Window::Resize(elMax(contentOffset + contentOffsetEnd, GetSize()));
    iwHeight = GetSize().y - contentOffset.y - contentOffsetEnd.y;

    // Save to settings that window is open
    SaveOpenStatus(true);

    if(windowSettings_)
    {
        // Restore minimized state
        if(windowSettings_->isMinimized)
        {
            isMinimized_ = true;
            Extent minimizedSize(GetSize().x, contentOffset.y + contentOffsetEnd.y);
            Window::Resize(minimizedSize);
        }
        isPinned_ = windowSettings_->isPinned;     // Restore pinned state
        restorePos_ = windowSettings_->restorePos; // Load restorePos
    }

    // Load last position or center the window
    if(pos == posLastOrCenter)
    {
        if(windowSettings_ && windowSettings_->lastPos.isValid())
            SetPos(windowSettings_->lastPos, !restorePos_.isValid());
        else
            MoveToCenter();
    } else if(pos == posCenter)
        MoveToCenter();
    else if(pos == posAtMouse)
        MoveNextToMouse();
    else
        SetPos(pos); // always call SetPos() to update restorePos
}

void IngameWindow::Resize(const Extent& newSize)
{
    DrawPoint iSize(newSize);
    iSize = elMax(DrawPoint(0, 0), iSize - DrawPoint(contentOffset + contentOffsetEnd));
    SetIwSize(Extent(iSize));
}

void IngameWindow::SetIwSize(const Extent& newSize)
{
    // Is the window connecting with the bottom screen edge?
    const auto atBottom = (GetPos().y + GetSize().y) >= VIDEODRIVER.GetRenderSize().y;

    iwHeight = newSize.y;
    Extent wndSize = newSize;
    if(isMinimized_)
        wndSize.y = 0;
    wndSize += contentOffset + contentOffsetEnd;
    Window::Resize(wndSize);

    // Adjust restorePos if the window was connecting with the bottom screen edge before being minimized
    const auto pos = (atBottom && isMinimized_) ? DrawPoint(restorePos_.x, DrawPoint::MaxElementValue) : restorePos_;

    // Reset the position
    // 1) to check if parts of the window are out of the visible area
    // 2) to re-connect the window with the bottom screen edge, if needed
    SetPos(pos, false);
}

Extent IngameWindow::GetIwSize() const
{
    return Extent(GetSize().x - contentOffset.x - contentOffsetEnd.x, iwHeight);
}

Extent IngameWindow::GetFullSize() const
{
    return Extent(GetSize().x, contentOffset.y + contentOffsetEnd.y + iwHeight);
}

DrawPoint IngameWindow::GetRightBottomBoundary()
{
    return DrawPoint(GetSize() - contentOffsetEnd);
}

void IngameWindow::SetPos(DrawPoint newPos, bool saveRestorePos)
{
    const Extent screenSize = VIDEODRIVER.GetRenderSize();
    DrawPoint newRestorePos = newPos;
    // Too far left or right?
    if(newPos.x < 0)
        newRestorePos.x = newPos.x = 0;
    else if(newPos.x + GetSize().x >= screenSize.x)
    {
        newPos.x = screenSize.x - GetSize().x;
        newRestorePos.x = DrawPoint::MaxElementValue; // make window stick to the right
    }

    // Too high or low?
    if(newPos.y < 0)
        newRestorePos.y = newPos.y = 0;
    else if(newPos.y + GetSize().y >= screenSize.y)
    {
        newPos.y = screenSize.y - GetSize().y;
        newRestorePos.y = DrawPoint::MaxElementValue; // make window stick to the bottom
    }

    if(saveRestorePos)
        restorePos_ = newRestorePos;

    // if possible save the positions to settings
    if(windowSettings_)
    {
        windowSettings_->lastPos = newPos;
        if(saveRestorePos)
            windowSettings_->restorePos = newRestorePos;
    }

    Window::SetPos(newPos);
}

void IngameWindow::Close()
{
    SaveOpenStatus(false);
    closeme = true;
}

void IngameWindow::SetMinimized(bool minimized)
{
    Extent fullSize = GetSize();
    if(isMinimized_)
        fullSize.y = iwHeight + contentOffset.y + contentOffsetEnd.y;
    this->isMinimized_ = minimized;
    Resize(fullSize);

    // if possible save the minimized state to settings
    if(windowSettings_)
        windowSettings_->isMinimized = isMinimized_;
}

void IngameWindow::SetPinned(bool pinned)
{
    isPinned_ = pinned;

    // if possible save the pinned state to settings
    if(windowSettings_)
        windowSettings_->isPinned = isPinned_;
}

void IngameWindow::MouseLeftDown(const MouseCoords& mc)
{
    // Check if the mouse is on the title bar
    if(IsPointInRect(mc.pos, GetButtonBounds(IwButton::Title)))
    {
        // start moving
        isMoving = true;
        snapOffset_ = SnapOffset::all(0);
        lastMousePos = mc.pos;
    } else
    {
        // Check the buttons
        for(const auto btn : helpers::enumRange<IwButton>())
        {
            if(IsPointInRect(mc.pos, GetButtonBounds(btn)))
                buttonStates_[btn] = ButtonState::Pressed;
        }
    }
}

void IngameWindow::MouseLeftUp(const MouseCoords& mc)
{
    isMoving = false;

    for(const auto btn : helpers::enumRange<IwButton>())
    {
        buttonStates_[btn] = ButtonState::Up;

        if((btn == IwButton::Close && closeBehavior_ == CloseBehavior::Custom) // no close button
           || (isModal_ // modal windows cannot be pinned or minimized
               && (btn == IwButton::Title || btn == IwButton::PinOrMinimize)))
            continue;

        if(IsPointInRect(mc.pos, GetButtonBounds(btn)))
        {
            switch(btn)
            {
                case IwButton::Close: Close(); break;
                case IwButton::Title:
                    if(SETTINGS.interface.enableWindowPinning && mc.dbl_click)
                    {
                        SetMinimized(!IsMinimized());
                        LOADER.GetSoundN("sound", 113)->Play(255, false);
                    }
                    break;
                case IwButton::PinOrMinimize:
                    if(SETTINGS.interface.enableWindowPinning)
                    {
                        SetPinned(!IsPinned());
                        LOADER.GetSoundN("sound", 111)->Play(255, false);
                    } else
                    {
                        SetMinimized(!IsMinimized());
                        LOADER.GetSoundN("sound", 113)->Play(255, false);
                    }
                    break;
            }
        }
    }
}

void IngameWindow::MouseMove(const MouseCoords& mc)
{
    if(isMoving)
    {
        // Calculate new window boundary rectangle without snapping
        DrawPoint delta = mc.pos - lastMousePos;
        Rect wndRect = GetBoundaryRect();
        RTTR_Assert(wndRect.getOrigin() == GetPos()); // The rest of the code assumes this to be true
        wndRect.move(delta - snapOffset_);

        // Try to snap this window to any other
        snapOffset_ = WINDOWMANAGER.snapWindow(this, wndRect);

        // The cursor position is always relative to the position of the "unsnapped" window; bound the "unsnapped"
        // window position to the screen…
        DrawPoint newPos = wndRect.getOrigin();
        DrawPoint newPosBounded =
          elMin(elMax(newPos, DrawPoint::all(0)), DrawPoint(VIDEODRIVER.GetRenderSize() - wndRect.getSize()));
        // …and use it to fix the mouse position if moved too far
        if(newPosBounded != newPos)
            VIDEODRIVER.SetMousePos(newPosBounded - wndRect.getOrigin() + mc.pos);

        // Set new position and re-calculate snap offset (window position may have been out of bounds)
        SetPos(newPos + snapOffset_);
        snapOffset_ = GetPos() - newPosBounded;

        lastMousePos = mc.pos;
    } else
    {
        // Check the buttons
        for(const auto btn : helpers::enumRange<IwButton>())
        {
            if(IsPointInRect(mc.pos, GetButtonBounds(btn)))
                buttonStates_[btn] = mc.ldown ? ButtonState::Pressed : ButtonState::Hover;
            else
                buttonStates_[btn] = ButtonState::Up;
        }
    }
}

void IngameWindow::Draw_()
{
    if(isModal_ && !IsActive())
        SetActive(true);
    if(!isMinimized_)
        DrawBackground();
    // Black border
    // TODO: It would be better if this was included in the windows size. But the controls are added with absolute
    // positions so adding the border to the size would move the border imgs inward into the content.
    //
    // Solution 1: Use contentOffset for adding controls
    //
    // Solution 2: Define that all controls added to an ingame window have positions relative to the contentOffset.
    //  This needs a change in GetDrawPos to add this offset and also change all control-add-calls but would be much
    //  cleaner (no more hard coded offsets and we could restyle the ingame windows easily)
    //
    const Rect drawRect = GetDrawRect();
    const Rect fullWndRect(drawRect.getOrigin() - borderSize, drawRect.getSize() + borderSize * 2u);
    // Top
    DrawRectangle(Rect(fullWndRect.getOrigin(), fullWndRect.getSize().x, borderSize.y), COLOR_BLACK);
    // Left
    DrawRectangle(Rect(fullWndRect.getOrigin(), borderSize.x, fullWndRect.getSize().y), COLOR_BLACK);
    // Right
    DrawRectangle(Rect(fullWndRect.right - borderSize.x, fullWndRect.top, borderSize.x, fullWndRect.getSize().y),
                  COLOR_BLACK);
    // Bottom
    DrawRectangle(Rect(fullWndRect.left, fullWndRect.bottom - borderSize.y, fullWndRect.getSize().x, borderSize.y),
                  COLOR_BLACK);

    // Upper parts
    glArchivItem_Bitmap* leftUpperImg = LOADER.GetImageN("resource", 36);
    leftUpperImg->DrawFull(GetPos());
    glArchivItem_Bitmap* rightUpperImg = LOADER.GetImageN("resource", 37);
    rightUpperImg->DrawFull(GetPos() + DrawPoint(GetSize().x - rightUpperImg->getWidth(), 0));

    // The buttons
    using ButtonStateResIds = helpers::EnumArray<unsigned, ButtonState>;
    constexpr ButtonStateResIds closeResIds = {47, 55, 51};
    constexpr ButtonStateResIds minimizeResIds = {48, 56, 52};
    constexpr ButtonStateResIds pinBaseResIds = {47, 47, 51};
    constexpr ButtonStateResIds pinOverlayResIds = {15, 16, 17};
    constexpr ButtonStateResIds unpinOverlayResIds = {18, 19, 20};
    if(closeBehavior_ != CloseBehavior::Custom)
        LOADER.GetImageN("resource", closeResIds[buttonStates_[IwButton::Close]])
          ->DrawFull(GetButtonBounds(IwButton::Close));
    if(!IsModal())
    {
        const auto buttonState = buttonStates_[IwButton::PinOrMinimize];
        const auto bounds = GetButtonBounds(IwButton::PinOrMinimize);
        if(SETTINGS.interface.enableWindowPinning)
        {
            LOADER.GetImageN("resource", pinBaseResIds[buttonState])->DrawFull(bounds);
            if(isPinned_)
                LOADER.GetImageN("io_new", unpinOverlayResIds[buttonState])->DrawFull(bounds);
            else
                LOADER.GetImageN("io_new", pinOverlayResIds[buttonState])->DrawFull(bounds);
        } else
            LOADER.GetImageN("resource", minimizeResIds[buttonState])->DrawFull(bounds);
    }

    // The title bar
    unsigned titleIndex;
    if(IsActive())
        titleIndex = isMoving ? 44 : 43;
    else
        titleIndex = 42;

    glArchivItem_Bitmap& titleImg = *LOADER.GetImageN("resource", titleIndex);
    DrawPoint titleImgPos = GetPos() + DrawPoint(leftUpperImg->getWidth(), 0);
    const unsigned titleWidth = GetSize().x - leftUpperImg->getWidth() - rightUpperImg->getWidth();
    // How often should the image be drawn to get the full width
    unsigned tileCount = titleWidth / titleImg.getWidth();
    for(unsigned i = 0; i < tileCount; ++i)
    {
        titleImg.DrawFull(titleImgPos);
        titleImgPos.x += titleImg.getWidth();
    }

    // The remaining part (if any)
    unsigned remainingTileSize = titleWidth % titleImg.getWidth();
    if(remainingTileSize)
        titleImg.DrawPart(Rect(titleImgPos, remainingTileSize, titleImg.getHeight()));

    // Text on the title bar
    NormalFont->Draw(GetPos() + DrawPoint(GetSize().x, titleImg.getHeight()) / 2, title_,
                     FontStyle::CENTER | FontStyle::VCENTER, COLOR_YELLOW);

    glArchivItem_Bitmap* bottomBorderSideImg = LOADER.GetImageN("resource", 45);
    glArchivItem_Bitmap* bottomBarImg = LOADER.GetImageN("resource", 40);

    // Side bars
    if(!isMinimized_)
    {
        unsigned sideHeight = GetSize().y - leftUpperImg->getHeight() - bottomBorderSideImg->getHeight();

        glArchivItem_Bitmap* leftSideImg = LOADER.GetImageN("resource", 38);
        glArchivItem_Bitmap* rightSideImg = LOADER.GetImageN("resource", 39);
        tileCount = sideHeight / leftSideImg->getHeight();
        DrawPoint leftImgPos = GetPos() + DrawPoint(0, leftUpperImg->getHeight());
        DrawPoint rightImgPos = leftImgPos + DrawPoint(GetSize().x - leftSideImg->getWidth(), 0);
        for(unsigned i = 0; i < tileCount; ++i)
        {
            leftSideImg->DrawFull(leftImgPos);
            rightSideImg->DrawFull(rightImgPos);
            rightImgPos.y = leftImgPos.y += leftSideImg->getHeight();
        }

        // And the partial part
        remainingTileSize = sideHeight % leftSideImg->getHeight();
        if(remainingTileSize)
        {
            leftSideImg->DrawPart(Rect(leftImgPos, leftSideImg->getWidth(), remainingTileSize));
            rightSideImg->DrawPart(Rect(rightImgPos, rightSideImg->getWidth(), remainingTileSize));
        }
    }

    // Lower bar
    const unsigned bottomBarWidth = GetSize().x - bottomBorderSideImg->getWidth() * 2;
    tileCount = bottomBarWidth / bottomBarImg->getWidth();
    DrawPoint bottomImgPos = GetPos() + DrawPoint(bottomBorderSideImg->getWidth(), GetRightBottomBoundary().y);
    for(unsigned i = 0; i < tileCount; ++i)
    {
        bottomBarImg->DrawFull(bottomImgPos);
        bottomImgPos.x += bottomBarImg->getWidth();
    }

    remainingTileSize = bottomBarWidth % bottomBarImg->getWidth();
    if(remainingTileSize)
        bottomBarImg->DrawPart(Rect(bottomImgPos, remainingTileSize, bottomBarImg->getHeight()));

    // Client area
    if(!isMinimized_)
    {
        Window::Draw_();
        DrawContent();
    }

    // The 2 rects on the bottom left and right
    bottomBorderSideImg->DrawFull(GetPos() + DrawPoint(0, GetSize().y - bottomBorderSideImg->getHeight()));
    bottomBorderSideImg->DrawFull(GetPos() + GetSize() - bottomBorderSideImg->GetSize());
}

void IngameWindow::DrawBackground()
{
    if(background)
        background->DrawPart(Rect(GetPos() + DrawPoint(contentOffset), GetIwSize()));
}

void IngameWindow::MoveToCenter()
{
    SetPos(DrawPoint(VIDEODRIVER.GetRenderSize() - GetSize()) / 2);
}

void IngameWindow::MoveNextToMouse()
{
    // Center vertically and move slightly right
    DrawPoint newPos = VIDEODRIVER.GetMousePos() - DrawPoint(-20, GetSize().y / 2);
    SetPos(newPos);
}

bool IngameWindow::IsMessageRelayAllowed() const
{
    return !isMinimized_ && !isMoving;
}

void IngameWindow::SaveOpenStatus(bool isOpen) const
{
    if(windowSettings_)
        windowSettings_->isOpen = isOpen;
}

Rect IngameWindow::GetButtonBounds(IwButton btn) const
{
    auto pos = GetPos();
    auto size = ButtonSize;
    switch(btn)
    {
        case IwButton::Close: break;
        case IwButton::Title:
            pos.x += TitleMargin;
            size.x = GetSize().x - TitleMargin * 2;
            break;
        case IwButton::PinOrMinimize: pos.x += GetSize().x - ButtonSize.x; break;
    }
    return Rect(pos, size);
}
