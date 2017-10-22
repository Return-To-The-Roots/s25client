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

#include "defines.h" // IWYU pragma: keep
#include "IngameWindow.h"

#include "CollisionDetection.h"
#include "Loader.h"
#include "driver/MouseCoords.h"
#include "drivers/VideoDriverWrapper.h"
#include "ogl/SoundEffectItem.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glArchivItem_Font.h"
#include "gameData/const_gui_ids.h"
#include <algorithm>

std::vector<DrawPoint> IngameWindow::last_pos(CGI_NEXT + 1, DrawPoint::Invalid());
const DrawPoint IngameWindow::posLastOrCenter(std::numeric_limits<DrawPoint::ElementType>::max(),
                                              std::numeric_limits<DrawPoint::ElementType>::max());
const DrawPoint IngameWindow::posAtMouse(std::numeric_limits<DrawPoint::ElementType>::max() - 1,
                                         std::numeric_limits<DrawPoint::ElementType>::max() - 1);

IngameWindow::IngameWindow(unsigned id, const DrawPoint& pos, const Extent& size, const std::string& title, glArchivItem_Bitmap* background,
                           bool modal, bool closeOnRightClick, Window* parent)
    : Window(parent, id, pos, size), title_(title), background(background), lastMousePos(0, 0), last_down(false), last_down2(false),
      isModal_(modal), closeme(false), isMinimized_(false), isMoving(false), closeOnRightClick_(closeOnRightClick)
{
    std::fill(button_state.begin(), button_state.end(), BUTTON_UP);
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
        if(id < last_pos.size() && last_pos[id].isValid())
            SetPos(last_pos[id]);
        else
            MoveToCenter();
    } else if(pos == posAtMouse)
        MoveNextToMouse();
}

IngameWindow::~IngameWindow()
{
    // Possibly save our old position
    if(GetID() < last_pos.size())
        last_pos[GetID()] = GetPos();
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
}

Extent IngameWindow::GetIwSize() const
{
    return Extent(GetSize().x - contentOffset.x - contentOffsetEnd.x, iwHeight);
}

DrawPoint IngameWindow::GetRightBottomBoundary()
{
    return DrawPoint(GetSize() - contentOffsetEnd);
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
    const Rect rec[2] = {GetLeftButtonRect(), GetRightButtonRect()};

    for(unsigned char i = 0; i < 2; ++i)
    {
        if(IsPointInRect(mc.GetPos(), rec[i]))
            button_state[i] = BUTTON_PRESSED;
    }
}

void IngameWindow::MouseLeftUp(const MouseCoords& mc)
{
    // Bewegung stoppen
    isMoving = false;

    // beiden Buttons oben links und rechts prfen
    const Rect rec[2] = {GetLeftButtonRect(), GetRightButtonRect()};

    for(unsigned i = 0; i < 2; ++i)
    {
        button_state[i] = BUTTON_UP;
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
        DrawPoint newPosBounded = elMin(elMax(newPos, DrawPoint::all(0)), DrawPoint(VIDEODRIVER.GetScreenSize() - GetSize()));
        // Fix mouse position if moved too far
        if(newPosBounded != newPos)
            VIDEODRIVER.SetMousePos(newPosBounded - GetPos() + lastMousePos);

        SetPos(newPosBounded);
        lastMousePos = mc.GetPos();
    }

    // beiden Buttons oben links und rechts prfen
    const Rect rec[2] = {GetLeftButtonRect(), GetRightButtonRect()};

    for(unsigned char i = 0; i < 2; ++i)
    {
        if(IsPointInRect(mc.GetPos(), rec[i]))
        {
            if(mc.ldown)
                button_state[i] = BUTTON_PRESSED;
            else
                button_state[i] = BUTTON_HOVER;
        } else
            button_state[i] = BUTTON_UP;
    }
}

void IngameWindow::Draw_()
{
    if(isModal_)
    {
        SetActive(true);
        Close(false);
    }

    // Linkes oberes Teil
    glArchivItem_Bitmap* leftUpperImg = LOADER.GetImageN("resource", 36);
    leftUpperImg->DrawFull(GetPos());
    // Rechtes oberes Teil
    glArchivItem_Bitmap* rightUpperImg = LOADER.GetImageN("resource", 37);
    rightUpperImg->DrawFull(GetPos() + DrawPoint(GetSize().x - rightUpperImg->getWidth(), 0));

    // Die beiden Buttons oben
    static const unsigned short ids[2][3] = {{47, 55, 50}, {48, 56, 52}};

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
                     glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_VCENTER, COLOR_YELLOW);

    if(!isMinimized_)
    {
        // Seitenleisten

        // Höhe
        glArchivItem_Bitmap* bottomBorderSideImg = LOADER.GetImageN("resource", 45);
        unsigned side_height = GetSize().y - leftUpperImg->getHeight() - bottomBorderSideImg->getHeight();

        // Wieviel mal nebeneinanderzeichnen?
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

        // Untere Leiste

        unsigned side_width = GetSize().x - bottomBorderSideImg->getWidth() * 2;

        // Wieviel mal nebeneinanderzeichnen?
        glArchivItem_Bitmap* bottomBarImg = LOADER.GetImageN("resource", 40);
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

        // Clientbereich

        // überhaupt ne Clienttexture gewnscht?
        if(background)
            background->DrawPart(Rect(GetPos() + DrawPoint(contentOffset), GetIwSize()));

        // Msg_PaintBefore aufrufen vor den Controls
        Msg_PaintBefore();

        DrawControls();

        // Links und rechts unten die 2 kleinen Knäufe
        bottomBorderSideImg->DrawFull(GetPos() + DrawPoint(0, GetSize().y - bottomBorderSideImg->getHeight()));
        bottomBorderSideImg->DrawFull(
          GetPos() + DrawPoint(GetSize().x - bottomBorderSideImg->getWidth(), GetSize().y - bottomBorderSideImg->getHeight()));
    } else
    {
        glArchivItem_Bitmap* bottomBorderSideImg = LOADER.GetImageN("resource", 45);
        glArchivItem_Bitmap* bottomBarImg = LOADER.GetImageN("resource", 40);
        unsigned side_width = GetSize().x - bottomBorderSideImg->getWidth() * 2;
        title_count = side_width / bottomBarImg->getWidth();

        DrawPoint bottomImgPos = GetPos() + DrawPoint(bottomBorderSideImg->getWidth(), 20);
        for(unsigned short i = 0; i < title_count; ++i)
        {
            bottomBarImg->DrawFull(bottomImgPos);
            bottomImgPos.x += bottomBarImg->getWidth();
        }

        rest = side_width % bottomBarImg->getWidth();

        if(rest)
            bottomBarImg->DrawPart(Rect(bottomImgPos, rest, bottomBarImg->getHeight()));

        bottomBorderSideImg->DrawFull(GetPos() + DrawPoint(0, bottomBorderSideImg->getHeight()));
        bottomBorderSideImg->DrawFull(GetPos()
                                      + DrawPoint(GetSize().x - bottomBorderSideImg->getWidth(), bottomBorderSideImg->getHeight()));
    }
}

/// Verschiebt Fenster in die Bildschirmmitte
void IngameWindow::MoveToCenter()
{
    SetPos(DrawPoint(VIDEODRIVER.GetScreenSize() - GetSize()) / 2);
}

/// Verschiebt Fenster neben die Maus
void IngameWindow::MoveNextToMouse()
{
    // Center vertically and move slightly right
    DrawPoint newPos = VIDEODRIVER.GetMousePos() - DrawPoint(-20, GetSize().y / 2);
    // To far right?
    if(newPos.x + GetSize().x > VIDEODRIVER.GetScreenSize().x)
        newPos.x = VIDEODRIVER.GetScreenSize().x - GetSize().x;

    // To high or low?
    if(newPos.y < 0)
        newPos.y = 0;
    else if(newPos.y + GetSize().y > VIDEODRIVER.GetScreenSize().y)
        newPos.y = VIDEODRIVER.GetScreenSize().y - GetSize().y;
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
    return Rect(static_cast<unsigned short>(GetPos().x + GetSize().x - 16), GetPos().y, 16, 16);
}
