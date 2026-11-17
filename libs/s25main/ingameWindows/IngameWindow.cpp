// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "IngameWindow.h"
#include "CollisionDetection.h"
#include "Loader.h"
#include "Point.h"
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
      closeBehavior_(closeBehavior), isModal_(modal), closeme(false), isPinned_(false), isMinimized_(false),
      isMoving(false)
{
    std::fill(buttonStates_.begin(), buttonStates_.end(), ButtonState::Up);
    contentOffset.x = LOADER.getImageN("resource", 38)->getWidth();     // left border
    contentOffset.y = LOADER.getImageN("resource", 42)->getHeight();    // title bar
    contentOffsetEnd.x = LOADER.getImageN("resource", 39)->getWidth();  // right border
    contentOffsetEnd.y = LOADER.getImageN("resource", 40)->getHeight(); // bottom bar

    const auto it = SETTINGS.windows.persistentSettings.find(getGUIID());
    windowSettings_ = (it == SETTINGS.windows.persistentSettings.cend() ? nullptr : &it->second);

    // For compatibility we treat the given height as the window height, not the content height
    // First we have to make sure the size is not to small
    Window::resize(elMax(contentOffset + contentOffsetEnd, getSize()));
    iwHeight = getSize().y - contentOffset.y - contentOffsetEnd.y;

    // Save to settings that window is open
    saveOpenStatus(true);

    if(windowSettings_)
    {
        // Restore minimized state
        if(windowSettings_->isMinimized)
        {
            isMinimized_ = true;
            Extent minimizedSize(getSize().x, contentOffset.y + contentOffsetEnd.y);
            Window::resize(minimizedSize);
        }
        isPinned_ = windowSettings_->isPinned;     // Restore pinned state
        restorePos_ = windowSettings_->restorePos; // Load restorePos
    }

    // Load last position or center the window
    if(pos == posLastOrCenter)
    {
        if(windowSettings_ && windowSettings_->lastPos.isValid())
            setPos(windowSettings_->lastPos, !restorePos_.isValid());
        else
            moveToCenter();
    } else if(pos == posCenter)
        moveToCenter();
    else if(pos == posAtMouse)
        moveNextToMouse();
    else
        setPos(pos); // always call SetPos() to update restorePos
}

void IngameWindow::resize(const Extent& newSize)
{
    DrawPoint iSize(newSize);
    iSize = elMax(DrawPoint(0, 0), iSize - DrawPoint(contentOffset + contentOffsetEnd));
    setIwSize(Extent(iSize));
}

void IngameWindow::setIwSize(const Extent& newSize)
{
    // Is the window connecting with the bottom screen edge?
    const auto atBottom = (getPos().y + getSize().y) >= VIDEODRIVER.getRenderSize().y;

    iwHeight = newSize.y;
    Extent wndSize = newSize;
    if(isMinimized_)
        wndSize.y = 0;
    wndSize += contentOffset + contentOffsetEnd;
    Window::resize(wndSize);

    // Adjust restorePos if the window was connecting with the bottom screen edge before being minimized
    const auto pos = (atBottom && isMinimized_) ? DrawPoint(restorePos_.x, DrawPoint::MaxElementValue) : restorePos_;

    // Reset the position
    // 1) to check if parts of the window are out of the visible area
    // 2) to re-connect the window with the bottom screen edge, if needed
    setPos(pos, false);
}

Extent IngameWindow::getIwSize() const
{
    return Extent(getSize().x - contentOffset.x - contentOffsetEnd.x, iwHeight);
}

Extent IngameWindow::getFullSize() const
{
    return Extent(getSize().x, contentOffset.y + contentOffsetEnd.y + iwHeight);
}

DrawPoint IngameWindow::getRightBottomBoundary()
{
    return DrawPoint(getSize() - contentOffsetEnd);
}

void IngameWindow::setPos(DrawPoint newPos, bool saveRestorePos)
{
    const Extent screenSize = VIDEODRIVER.getRenderSize();
    DrawPoint newRestorePos = newPos;
    // Too far left or right?
    if(newPos.x < 0)
        newRestorePos.x = newPos.x = 0;
    else if(newPos.x + getSize().x >= screenSize.x)
    {
        newPos.x = screenSize.x - getSize().x;
        newRestorePos.x = DrawPoint::MaxElementValue; // make window stick to the right
    }

    // Too high or low?
    if(newPos.y < 0)
        newRestorePos.y = newPos.y = 0;
    else if(newPos.y + getSize().y >= screenSize.y)
    {
        newPos.y = screenSize.y - getSize().y;
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

    Window::setPos(newPos);
}

void IngameWindow::close()
{
    saveOpenStatus(false);
    closeme = true;
}

void IngameWindow::setMinimized(bool minimized)
{
    Extent fullSize = getSize();
    if(isMinimized_)
        fullSize.y = iwHeight + contentOffset.y + contentOffsetEnd.y;
    this->isMinimized_ = minimized;
    resize(fullSize);

    // if possible save the minimized state to settings
    if(windowSettings_)
        windowSettings_->isMinimized = isMinimized_;
}

void IngameWindow::setPinned(bool pinned)
{
    isPinned_ = pinned;

    // if possible save the pinned state to settings
    if(windowSettings_)
        windowSettings_->isPinned = isPinned_;
}

bool IngameWindow::msgLeftDown(const MouseCoords& mc)
{
    for(const auto btn : helpers::enumRange<IwButton>())
    {
        if(IsPointInRect(mc.pos, getButtonBounds(btn)))
        {
            buttonStates_[btn] = ButtonState::Pressed;
            if(btn == IwButton::Title)
            {
                startDragging(mc.pos);
                return true;
            }
        }
    }
    return false;
}

bool IngameWindow::msgLeftUp(const MouseCoords& mc)
{
    // If we handled the msg event. Don't return yet in case touch dblclick triggered dragging
    const bool handled = stopDragging();

    for(const auto btn : helpers::enumRange<IwButton>())
    {
        const auto clicked = buttonStates_[btn] == ButtonState::Pressed;
        buttonStates_[btn] = ButtonState::Up;
        if(!clicked)
            continue;

        if((btn == IwButton::Close && closeBehavior_ == CloseBehavior::Custom) // no close button
           || (isModal_ // modal windows cannot be pinned or minimized
               && (btn == IwButton::Title || btn == IwButton::PinOrMinimize)))
            continue;

        if(IsPointInRect(mc.pos, getButtonBounds(btn)))
        {
            switch(btn)
            {
                case IwButton::Close: close(); return true;
                case IwButton::Title:
                    if(SETTINGS.interface.enableWindowPinning && mc.dbl_click)
                    {
                        setMinimized(!isMinimized());
                        LOADER.getSoundN("sound", 113)->play(255, false);
                        return true;
                    }
                    break;
                case IwButton::PinOrMinimize:
                    if(SETTINGS.interface.enableWindowPinning)
                    {
                        setPinned(!isPinned());
                        LOADER.getSoundN("sound", 111)->play(255, false);
                    } else
                    {
                        setMinimized(!isMinimized());
                        LOADER.getSoundN("sound", 113)->play(255, false);
                    }
                    return true;
            }
        }
    }

    // On touch devices dblclick closes window
    if(VIDEODRIVER.isTouch() && mc.dbl_click && closeBehavior_ != CloseBehavior::Custom)
    {
        close();
        return true;
    }
    return handled;
}

bool IngameWindow::msgMiddleDown(const MouseCoords& mc)
{
    startDragging(mc.pos);
    return true;
}

bool IngameWindow::msgMiddleUp(const MouseCoords&)
{
    return stopDragging();
}

bool IngameWindow::msgMouseMove(const MouseCoords& mc)
{
    if(isMoving)
    {
        // Calculate new window boundary rectangle without snapping
        DrawPoint delta = mc.pos - lastMousePos;
        Rect wndRect = getBoundaryRect();
        RTTR_Assert(wndRect.getOrigin() == getPos()); // The rest of the code assumes this to be true
        wndRect.move(delta - snapOffset_);

        // Try to snap this window to any other
        snapOffset_ = WINDOWMANAGER.snapWindow(this, wndRect);

        // The cursor position is always relative to the position of the "unsnapped" window; bound the "unsnapped"
        // window position to the screen…
        DrawPoint newPos = wndRect.getOrigin();
        DrawPoint newPosBounded =
          elMin(elMax(newPos, DrawPoint::all(0)), DrawPoint(VIDEODRIVER.getRenderSize() - wndRect.getSize()));
        // …and use it to fix the mouse position if moved too far
        if(newPosBounded != newPos)
            VIDEODRIVER.setMousePos(newPosBounded - wndRect.getOrigin() + mc.pos);

        // Set new position and re-calculate snap offset (window position may have been out of bounds)
        setPos(newPos + snapOffset_);
        snapOffset_ = getPos() - newPosBounded;

        lastMousePos = mc.pos;
        return true;
    } else
    {
        // Check the buttons
        for(const auto btn : helpers::enumRange<IwButton>())
        {
            if(IsPointInRect(mc.pos, getButtonBounds(btn)))
            {
                if(!mc.ldown)
                    buttonStates_[btn] = ButtonState::Hover;
            } else
                buttonStates_[btn] = ButtonState::Up;
        }
        return false;
    }
}

void IngameWindow::drawImpl()
{
    if(isModal_ && !IsActive())
        setActive(true);
    if(!isMinimized_)
        drawBackground();
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
    const Rect drawRect = getDrawRect();
    const Rect fullWndRect(drawRect.getOrigin() - borderSize, drawRect.getSize() + borderSize * 2u);
    // Top
    drawRectangle(Rect(fullWndRect.getOrigin(), fullWndRect.getSize().x, borderSize.y), COLOR_BLACK);
    // Left
    drawRectangle(Rect(fullWndRect.getOrigin(), borderSize.x, fullWndRect.getSize().y), COLOR_BLACK);
    // Right
    drawRectangle(Rect(fullWndRect.right - borderSize.x, fullWndRect.top, borderSize.x, fullWndRect.getSize().y),
                  COLOR_BLACK);
    // Bottom
    drawRectangle(Rect(fullWndRect.left, fullWndRect.bottom - borderSize.y, fullWndRect.getSize().x, borderSize.y),
                  COLOR_BLACK);

    // Upper parts
    glArchivItem_Bitmap* leftUpperImg = LOADER.getImageN("resource", 36);
    leftUpperImg->drawFull(getPos());
    glArchivItem_Bitmap* rightUpperImg = LOADER.getImageN("resource", 37);
    rightUpperImg->drawFull(getPos() + DrawPoint(getSize().x - rightUpperImg->getWidth(), 0));

    // The buttons
    using ButtonStateResIds = helpers::EnumArray<unsigned, ButtonState>;
    constexpr ButtonStateResIds closeResIds = {47, 55, 51};
    constexpr ButtonStateResIds minimizeResIds = {48, 56, 52};
    constexpr ButtonStateResIds pinBaseResIds = {47, 47, 51};
    constexpr ButtonStateResIds pinOverlayResIds = {15, 16, 17};
    constexpr ButtonStateResIds unpinOverlayResIds = {18, 19, 20};
    if(closeBehavior_ != CloseBehavior::Custom)
        LOADER.getImageN("resource", closeResIds[buttonStates_[IwButton::Close]])
          ->drawFull(getButtonBounds(IwButton::Close));
    if(!isModal())
    {
        const auto buttonState = buttonStates_[IwButton::PinOrMinimize];
        const auto bounds = getButtonBounds(IwButton::PinOrMinimize);
        if(SETTINGS.interface.enableWindowPinning)
        {
            LOADER.getImageN("resource", pinBaseResIds[buttonState])->drawFull(bounds);
            if(isPinned_)
                LOADER.getImageN("io_new", unpinOverlayResIds[buttonState])->drawFull(bounds);
            else
                LOADER.getImageN("io_new", pinOverlayResIds[buttonState])->drawFull(bounds);
        } else
            LOADER.getImageN("resource", minimizeResIds[buttonState])->drawFull(bounds);
    }

    // The title bar
    unsigned titleIndex;
    if(IsActive())
        titleIndex = isMoving ? 44 : 43;
    else
        titleIndex = 42;

    glArchivItem_Bitmap& titleImg = *LOADER.getImageN("resource", titleIndex);
    DrawPoint titleImgPos = getPos() + DrawPoint(leftUpperImg->getWidth(), 0);
    const unsigned titleWidth = getSize().x - leftUpperImg->getWidth() - rightUpperImg->getWidth();
    // How often should the image be drawn to get the full width
    unsigned tileCount = titleWidth / titleImg.getWidth();
    for(unsigned i = 0; i < tileCount; ++i)
    {
        titleImg.drawFull(titleImgPos);
        titleImgPos.x += titleImg.getWidth();
    }

    // The remaining part (if any)
    unsigned remainingTileSize = titleWidth % titleImg.getWidth();
    if(remainingTileSize)
        titleImg.drawPart(Rect(titleImgPos, remainingTileSize, titleImg.getHeight()));

    // Text on the title bar
    NormalFont->draw(getPos() + DrawPoint(getSize().x, titleImg.getHeight()) / 2, title_,
                     FontStyle::CENTER | FontStyle::VCENTER, COLOR_YELLOW);

    glArchivItem_Bitmap* bottomBorderSideImg = LOADER.getImageN("resource", 45);
    glArchivItem_Bitmap* bottomBarImg = LOADER.getImageN("resource", 40);

    // Side bars
    if(!isMinimized_)
    {
        unsigned sideHeight = getSize().y - leftUpperImg->getHeight() - bottomBorderSideImg->getHeight();

        glArchivItem_Bitmap* leftSideImg = LOADER.getImageN("resource", 38);
        glArchivItem_Bitmap* rightSideImg = LOADER.getImageN("resource", 39);
        tileCount = sideHeight / leftSideImg->getHeight();
        DrawPoint leftImgPos = getPos() + DrawPoint(0, leftUpperImg->getHeight());
        DrawPoint rightImgPos = leftImgPos + DrawPoint(getSize().x - leftSideImg->getWidth(), 0);
        for(unsigned i = 0; i < tileCount; ++i)
        {
            leftSideImg->drawFull(leftImgPos);
            rightSideImg->drawFull(rightImgPos);
            rightImgPos.y = leftImgPos.y += leftSideImg->getHeight();
        }

        // And the partial part
        remainingTileSize = sideHeight % leftSideImg->getHeight();
        if(remainingTileSize)
        {
            leftSideImg->drawPart(Rect(leftImgPos, leftSideImg->getWidth(), remainingTileSize));
            rightSideImg->drawPart(Rect(rightImgPos, rightSideImg->getWidth(), remainingTileSize));
        }
    }

    // Lower bar
    const unsigned bottomBarWidth = getSize().x - bottomBorderSideImg->getWidth() * 2;
    tileCount = bottomBarWidth / bottomBarImg->getWidth();
    DrawPoint bottomImgPos = getPos() + DrawPoint(bottomBorderSideImg->getWidth(), getRightBottomBoundary().y);
    for(unsigned i = 0; i < tileCount; ++i)
    {
        bottomBarImg->drawFull(bottomImgPos);
        bottomImgPos.x += bottomBarImg->getWidth();
    }

    remainingTileSize = bottomBarWidth % bottomBarImg->getWidth();
    if(remainingTileSize)
        bottomBarImg->drawPart(Rect(bottomImgPos, remainingTileSize, bottomBarImg->getHeight()));

    // Client area
    if(!isMinimized_)
    {
        Window::drawImpl();
        drawContent();
    }

    // The 2 rects on the bottom left and right
    bottomBorderSideImg->drawFull(getPos() + DrawPoint(0, getSize().y - bottomBorderSideImg->getHeight()));
    bottomBorderSideImg->drawFull(getPos() + getSize() - bottomBorderSideImg->getSize());
}

void IngameWindow::drawBackground()
{
    if(background)
        background->drawPart(Rect(getPos() + DrawPoint(contentOffset), getIwSize()));
}

void IngameWindow::moveToCenter()
{
    setPos(DrawPoint(VIDEODRIVER.getRenderSize() - getSize()) / 2);
}

void IngameWindow::moveNextToMouse()
{
    // Center vertically and move slightly right
    DrawPoint newPos = VIDEODRIVER.getMousePos() - DrawPoint(-20, getSize().y / 2);
    setPos(newPos);
}

bool IngameWindow::isMessageRelayAllowed() const
{
    return !isMinimized_ && !isMoving;
}

void IngameWindow::saveOpenStatus(bool isOpen) const
{
    if(windowSettings_)
        windowSettings_->isOpen = isOpen;
}

Rect IngameWindow::getButtonBounds(IwButton btn) const
{
    auto pos = getPos();
    auto size = ButtonSize;
    switch(btn)
    {
        case IwButton::Close: break;
        case IwButton::Title:
            pos.x += TitleMargin;
            size.x = getSize().x - TitleMargin * 2;
            break;
        case IwButton::PinOrMinimize: pos.x += getSize().x - ButtonSize.x; break;
    }
    return Rect(pos, size);
}

void IngameWindow::startDragging(const Position& pos)
{
    isMoving = true;
    snapOffset_ = SnapOffset::all(0);
    lastMousePos = pos;
}

bool IngameWindow::stopDragging()
{
    if(isMoving)
    {
        isMoving = false;
        return true;
    }
    return false;
}
