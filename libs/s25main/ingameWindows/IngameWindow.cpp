// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "IngameWindow.h"
#include "CollisionDetection.h"
#include "Loader.h"
#include "Settings.h"
#include "driver/MouseCoords.h"
#include "drivers/VideoDriverWrapper.h"
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

const DrawPoint IngameWindow::posLastOrCenter(std::numeric_limits<DrawPoint::ElementType>::max(),
                                              std::numeric_limits<DrawPoint::ElementType>::max());
const DrawPoint IngameWindow::posCenter(std::numeric_limits<DrawPoint::ElementType>::max() - 1,
                                        std::numeric_limits<DrawPoint::ElementType>::max());
const DrawPoint IngameWindow::posAtMouse(std::numeric_limits<DrawPoint::ElementType>::max() - 1,
                                         std::numeric_limits<DrawPoint::ElementType>::max() - 1);

const Extent IngameWindow::borderSize(1, 1);
IngameWindow::IngameWindow(unsigned id, const DrawPoint& pos, const Extent& size, std::string title,
                           glArchivItem_Bitmap* background, bool modal, bool closeOnRightClick, Window* parent)
    : Window(parent, id, pos, size), title_(std::move(title)), background(background), lastMousePos(0, 0),
      last_down(false), last_down2(false), isModal_(modal), closeme(false), isMinimized_(false), isMoving(false),
      closeOnRightClick_(closeOnRightClick)
{
    std::fill(button_state.begin(), button_state.end(), ButtonState::Up);
    contentOffset.x = LOADER.GetImageN("resource", 38)->getWidth();     // left border
    contentOffset.y = LOADER.GetImageN("resource", 42)->getHeight();    // title bar
    contentOffsetEnd.x = LOADER.GetImageN("resource", 39)->getWidth();  // right border
    contentOffsetEnd.y = LOADER.GetImageN("resource", 40)->getHeight(); // bottom bar

    // For compatibility we treat the given height as the window height, not the content height
    // First we have to make sure the size is not to small
    Window::Resize(elMax(contentOffset + contentOffsetEnd, GetSize()));
    iwHeight = GetSize().y - contentOffset.y - contentOffsetEnd.y;

    // Load last position or center the window
    if(pos == posLastOrCenter)
    {
        const auto settings = SETTINGS.windows.persistentSettings.find(GetGUIID());
        if(settings != SETTINGS.windows.persistentSettings.cend() && settings->second.lastPos.isValid())
            SetPos(settings->second.lastPos);
        else
            MoveToCenter();
    } else if(pos == posCenter)
        MoveToCenter();
    else if(pos == posAtMouse)
        MoveNextToMouse();
}

IngameWindow::~IngameWindow()
{
    try
    {
        // Possibly save our old position
        auto& settings = SETTINGS.windows.persistentSettings.at(GetGUIID());
        settings.lastPos = GetPos();
        settings.isOpen = !closeme;
    } catch(const std::out_of_range& oor)
    { // Window is not persistent, no problem
    } catch(std::runtime_error& err)
    { // SETTINGS was probably destroyed already, don't save but print a warning
        s25util::warning(std::string("Could not save ingame windows settings. Reason: ") + err.what());
    }
}

void IngameWindow::Resize(const Extent& newSize)
{
    DrawPoint iSize(newSize);
    iSize = elMax(DrawPoint(0, 0), iSize - DrawPoint(contentOffset + contentOffsetEnd));
    SetIwSize(Extent(iSize));
}

void IngameWindow::SetIwSize(const Extent& newSize)
{
    iwHeight = newSize.y;
    Extent wndSize = newSize;
    if(isMinimized_)
        wndSize.y = 0;
    wndSize += contentOffset + contentOffsetEnd;
    Window::Resize(wndSize);

    // Reset the position to check if parts of the window are out of the visible area
    SetPos(GetPos());
}

Extent IngameWindow::GetIwSize() const
{
    return Extent(GetSize().x - contentOffset.x - contentOffsetEnd.x, iwHeight);
}

DrawPoint IngameWindow::GetRightBottomBoundary()
{
    return DrawPoint(GetSize() - contentOffsetEnd);
}

void IngameWindow::SetPos(DrawPoint newPos)
{
    const Extent screenSize = VIDEODRIVER.GetRenderSize();
    // Too far left or right?
    if(newPos.x < 0)
        newPos.x = 0;
    else if(newPos.x + GetSize().x > screenSize.x)
        newPos.x = screenSize.x - GetSize().x;

    // Too high or low?
    if(newPos.y < 0)
        newPos.y = 0;
    else if(newPos.y + GetSize().y > screenSize.y)
        newPos.y = screenSize.y - GetSize().y;

    Window::SetPos(newPos);
}

void IngameWindow::Close()
{
    closeme = true;
}

void IngameWindow::SetMinimized(bool minimized)
{
    Extent fullSize = GetSize();
    if(isMinimized_)
        fullSize.y = iwHeight + contentOffset.y + contentOffsetEnd.y;
    this->isMinimized_ = minimized;
    Resize(fullSize);
}

void IngameWindow::MouseLeftDown(const MouseCoords& mc)
{
    // Maus muss sich auf der Titelleiste befinden
    Rect title_rect(LOADER.GetImageN("resource", 36)->getWidth(), 0,
                    static_cast<unsigned short>(GetSize().x - LOADER.GetImageN("resource", 36)->getWidth()
                                                - LOADER.GetImageN("resource", 37)->getWidth()),
                    LOADER.GetImageN("resource", 43)->getHeight());
    title_rect.move(GetDrawPos());

    if(IsPointInRect(mc.GetPos(), title_rect))
    {
        // Start mit Bewegung
        isMoving = true;
        lastMousePos = mc.GetPos();
    }

    // beiden Buttons oben links und rechts prfen
    const std::array<Rect, 2> rec = {GetLeftButtonRect(), GetRightButtonRect()};

    for(unsigned char i = 0; i < 2; ++i)
    {
        if(IsPointInRect(mc.GetPos(), rec[i]))
            button_state[i] = ButtonState::Pressed;
    }
}

void IngameWindow::MouseLeftUp(const MouseCoords& mc)
{
    // Bewegung stoppen
    isMoving = false;

    // beiden Buttons oben links und rechts prfen
    const std::array<Rect, 2> rec = {GetLeftButtonRect(), GetRightButtonRect()};

    for(unsigned i = 0; i < 2; ++i)
    {
        button_state[i] = ButtonState::Up;
        if(IsPointInRect(mc.GetPos(), rec[i]))
        {
            if(i == 0 && (!IsModal() || closeOnRightClick_))
                Close();
            else if(i == 1 && !IsModal())
            {
                SetMinimized(!IsMinimized());
                LOADER.GetSoundN("sound", 113)->Play(255, false);
            }
        }
    }
}

void IngameWindow::MouseMove(const MouseCoords& mc)
{
    // Fenster bewegen, wenn die Bewegung aktiviert wurde
    if(isMoving)
    {
        DrawPoint newPos = GetPos() + mc.GetPos() - lastMousePos;
        // Make sure we don't move outside window on either side
        DrawPoint newPosBounded =
          elMin(elMax(newPos, DrawPoint::all(0)), DrawPoint(VIDEODRIVER.GetRenderSize() - GetSize()));
        // Fix mouse position if moved too far
        if(newPosBounded != newPos)
            VIDEODRIVER.SetMousePos(newPosBounded - GetPos() + lastMousePos);

        SetPos(newPosBounded);
        lastMousePos = mc.GetPos();
    }

    // beiden Buttons oben links und rechts prfen
    const std::array<Rect, 2> rec = {GetLeftButtonRect(), GetRightButtonRect()};

    for(unsigned char i = 0; i < 2; ++i)
    {
        if(IsPointInRect(mc.GetPos(), rec[i]))
        {
            if(mc.ldown)
                button_state[i] = ButtonState::Pressed;
            else
                button_state[i] = ButtonState::Hover;
        } else
            button_state[i] = ButtonState::Up;
    }
}

void IngameWindow::Draw_()
{
    if(isModal_ && !IsActive())
        SetActive(true);

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
    Rect drawRect = GetDrawRect();
    Rect fullWndRect(drawRect.getOrigin() - borderSize, drawRect.getSize() + borderSize * 2u);
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

    // Linkes oberes Teil
    glArchivItem_Bitmap* leftUpperImg = LOADER.GetImageN("resource", 36);
    leftUpperImg->DrawFull(GetPos());
    // Rechtes oberes Teil
    glArchivItem_Bitmap* rightUpperImg = LOADER.GetImageN("resource", 37);
    rightUpperImg->DrawFull(GetPos() + DrawPoint(GetSize().x - rightUpperImg->getWidth(), 0));

    // Die beiden Buttons oben
    static constexpr std::array<helpers::EnumArray<uint16_t, ButtonState>, 2> ids = {{{47, 55, 50}, {48, 56, 52}}};

    // Titelleiste
    if(closeOnRightClick_ || !IsModal())
        LOADER.GetImageN("resource", ids[0][button_state[0]])->DrawFull(GetPos());
    if(!IsModal())
        LOADER.GetImageN("resource", ids[1][button_state[1]])->DrawFull(GetPos() + DrawPoint(GetSize().x - 16, 0));

    // Breite berechnen
    unsigned title_width = GetSize().x - leftUpperImg->getWidth() - rightUpperImg->getWidth();

    unsigned short title_index;
    if(IsActive())
        title_index = isMoving ? 44 : 43;
    else
        title_index = 42;

    glArchivItem_Bitmap& titleImg = *LOADER.GetImageN("resource", title_index);
    DrawPoint titleImgPos = GetPos() + DrawPoint(leftUpperImg->getWidth(), 0);
    // Wieviel mal nebeneinanderzeichnen?
    unsigned short title_count = title_width / titleImg.getWidth();
    for(unsigned short i = 0; i < title_count; ++i)
    {
        titleImg.DrawFull(titleImgPos);
        titleImgPos.x += titleImg.getWidth();
    }

    // Rest zeichnen
    unsigned short rest = title_width % titleImg.getWidth();

    if(rest)
        titleImg.DrawPart(Rect(titleImgPos, rest, titleImg.getHeight()));

    // Text auf die Leiste
    NormalFont->Draw(GetPos() + DrawPoint(GetSize().x, titleImg.getHeight()) / 2, title_,
                     FontStyle::CENTER | FontStyle::VCENTER, COLOR_YELLOW);

    glArchivItem_Bitmap* bottomBorderSideImg = LOADER.GetImageN("resource", 45);
    glArchivItem_Bitmap* bottomBarImg = LOADER.GetImageN("resource", 40);

    // Side bars
    if(!isMinimized_)
    {
        unsigned side_height = GetSize().y - leftUpperImg->getHeight() - bottomBorderSideImg->getHeight();

        glArchivItem_Bitmap* leftSideImg = LOADER.GetImageN("resource", 38);
        glArchivItem_Bitmap* rightSideImg = LOADER.GetImageN("resource", 39);
        title_count = side_height / leftSideImg->getHeight();
        DrawPoint leftImgPos = GetPos() + DrawPoint(0, leftUpperImg->getHeight());
        DrawPoint rightImgPos = leftImgPos + DrawPoint(GetSize().x - leftSideImg->getWidth(), 0);
        for(unsigned short i = 0; i < title_count; ++i)
        {
            leftSideImg->DrawFull(leftImgPos);
            rightSideImg->DrawFull(rightImgPos);
            rightImgPos.y = leftImgPos.y += leftSideImg->getHeight();
        }

        // Rest zeichnen
        rest = side_height % leftSideImg->getHeight();

        if(rest)
        {
            leftSideImg->DrawPart(Rect(leftImgPos, leftSideImg->getWidth(), rest));
            rightSideImg->DrawPart(Rect(rightImgPos, rightSideImg->getWidth(), rest));
        }
    }

    // Untere Leiste
    unsigned side_width = GetSize().x - bottomBorderSideImg->getWidth() * 2;
    title_count = side_width / bottomBarImg->getWidth();
    DrawPoint bottomImgPos = GetPos() + DrawPoint(bottomBorderSideImg->getWidth(), GetRightBottomBoundary().y);
    for(unsigned short i = 0; i < title_count; ++i)
    {
        bottomBarImg->DrawFull(bottomImgPos);
        bottomImgPos.x += bottomBarImg->getWidth();
    }

    rest = side_width % bottomBarImg->getWidth();

    if(rest)
        bottomBarImg->DrawPart(Rect(bottomImgPos, rest, bottomBarImg->getHeight()));

    // Client area
    if(!isMinimized_)
    {
        if(background)
            background->DrawPart(Rect(GetPos() + DrawPoint(contentOffset), GetIwSize()));

        Window::Draw_();
    }

    // Links und rechts unten die 2 kleinen Knäufe
    bottomBorderSideImg->DrawFull(GetPos() + DrawPoint(0, GetSize().y - bottomBorderSideImg->getHeight()));
    bottomBorderSideImg->DrawFull(
      GetPos()
      + DrawPoint(GetSize().x - bottomBorderSideImg->getWidth(), GetSize().y - bottomBorderSideImg->getHeight()));
}

/// Verschiebt Fenster in die Bildschirmmitte
void IngameWindow::MoveToCenter()
{
    SetPos(DrawPoint(VIDEODRIVER.GetRenderSize() - GetSize()) / 2);
}

/// Verschiebt Fenster neben die Maus
void IngameWindow::MoveNextToMouse()
{
    // Center vertically and move slightly right
    DrawPoint newPos = VIDEODRIVER.GetMousePos() - DrawPoint(-20, GetSize().y / 2);
    SetPos(newPos);
}

/// Weiterleitung von Nachrichten erlaubt oder nicht?
bool IngameWindow::IsMessageRelayAllowed() const
{
    // Wenn es minimiert wurde, sollen keine Nachrichten weitergeleitet werden
    return !isMinimized_;
}

Rect IngameWindow::GetLeftButtonRect() const
{
    return Rect(GetPos(), 16, 16);
}

Rect IngameWindow::GetRightButtonRect() const
{
    return Rect(GetPos().x + GetSize().x - 16, GetPos().y, 16, 16);
}
