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
#include "IngameWindow.h"

#include "drivers/VideoDriverWrapper.h"
#include "Loader.h"
#include "CollisionDetection.h"
#include "driver/src/MouseCoords.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glArchivItem_Font.h"
#include "ogl/glArchivItem_Sound.h"
#include "gameData/const_gui_ids.h"
#include <algorithm>

std::vector<DrawPoint> IngameWindow::last_pos(CGI_NEXT + 1, DrawPoint::Invalid());
const DrawPoint IngameWindow::posLastOrCenter(std::numeric_limits<DrawPoint::ElementType>::max(), std::numeric_limits<DrawPoint::ElementType>::max());
const DrawPoint IngameWindow::posAtMouse(std::numeric_limits<DrawPoint::ElementType>::max()-1, std::numeric_limits<DrawPoint::ElementType>::max()-1);

IngameWindow::IngameWindow(unsigned int id, const DrawPoint& pos, unsigned short width, unsigned short height,
                           const std::string& title, glArchivItem_Bitmap* background, bool modal, bool closeOnRightClick, Window* parent)
    : Window(pos, id, parent, width, height),
      title_(title), background(background), lastMousePos(0, 0),
      last_down(false), last_down2(false), isModal_(modal), closeme(false), isMinimized_(false), isMoving(false), closeOnRightClick_(closeOnRightClick)
{
    std::fill(button_state.begin(), button_state.end(), BUTTON_UP);
    contentOffset.x = LOADER.GetImageN("resource", 38)->getWidth();     // left border
    contentOffset.y = LOADER.GetImageN("resource", 42)->getHeight();    // title bar
    contentOffsetEnd.x = LOADER.GetImageN("resource", 39)->getWidth();  // right border
    contentOffsetEnd.y = LOADER.GetImageN("resource", 40)->getHeight(); // bottom bar

    // For compatibility we treat the given height as the window height, not the content height
    iwHeight = std::max(0, height - contentOffset.y - contentOffsetEnd.y);

    // Load last position or center the window
    if(pos == posLastOrCenter)
    {

        if(id < last_pos.size() && last_pos[id].isValid())
            Move(last_pos[id]);
        else
            MoveToCenter();
    } else if(pos == posAtMouse)
        MoveNextToMouse();
}

IngameWindow::~IngameWindow()
{
    // Possibly save our old position
    if(id_ < last_pos.size())
        last_pos[id_] = pos_;
}

void IngameWindow::Resize(unsigned short width, unsigned short height)
{
    SetIwWidth(std::max(0, width - contentOffset.x - contentOffsetEnd.x));
    SetIwHeight(std::max(0, height - contentOffset.y - contentOffsetEnd.y));
}

void IngameWindow::SetIwHeight(unsigned short height)
{
    this->iwHeight = height;
    if(!isMinimized_)
        Window::Resize(width_, height + contentOffset.y + contentOffsetEnd.y);
}

unsigned short IngameWindow::GetIwHeight() const
{
    return iwHeight;
}

void IngameWindow::SetIwWidth(unsigned short width)
{
    Window::Resize(width + contentOffset.x + contentOffsetEnd.x, height_);
}

unsigned short IngameWindow::GetIwWidth() const
{
    return std::max(0, GetWidth() - contentOffset.x - contentOffsetEnd.x);
}

unsigned short IngameWindow::GetIwRightBoundary() const
{
    return GetWidth() - contentOffsetEnd.x;
}

unsigned short IngameWindow::GetIwBottomBoundary() const
{
    return GetHeight() - contentOffsetEnd.y;
}

void IngameWindow::SetMinimized(bool minimized)
{
    this->isMinimized_ = minimized;
    Window::SetHeight((minimized ? 0 : iwHeight) + contentOffset.y + contentOffsetEnd.y);
}

void IngameWindow::MouseLeftDown(const MouseCoords& mc)
{
    // Maus muss sich auf der Titelleiste befinden
    Rect title_rect(
        static_cast<unsigned short>(pos_.x + LOADER.GetImageN("resource", 36)->getWidth()),
        pos_.y,
        static_cast<unsigned short>(width_ - LOADER.GetImageN("resource", 36)->getWidth() - LOADER.GetImageN("resource", 37)->getWidth()),
        LOADER.GetImageN("resource", 43)->getHeight()
        );

    if(Coll(mc.x, mc.y, title_rect))
    {
        // Start mit Bewegung
        isMoving = true;
        lastMousePos = DrawPoint(mc.x, mc.y);
    }

    // beiden Buttons oben links und rechts prfen
    const Rect rec[2] =
    {
        GetLeftButtonRect(),
        GetRightButtonRect()
    };

    for(unsigned char i = 0; i < 2; ++i)
    {
        if(Coll(mc.x, mc.y, rec[i]))
            button_state[i] = BUTTON_PRESSED;
    }
}

void IngameWindow::MouseLeftUp(const MouseCoords& mc)
{
    // Bewegung stoppen
    isMoving = false;

    // beiden Buttons oben links und rechts prfen
    const Rect rec[2] =
    {
        GetLeftButtonRect(),
        GetRightButtonRect()
    };

    for(unsigned i = 0; i < 2; ++i)
    {
        button_state[i] = BUTTON_UP;
        if(Coll(mc.x, mc.y, rec[i]))
        {
            if(i == 0 && (!IsModal() || closeOnRightClick_))
                Close();
            else if(i==1 && !IsModal())
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
        DrawPoint newMousePos(mc.x, mc.y);

        DrawPoint newPos = pos_ + newMousePos - lastMousePos;
        if(newPos.x < 0)
        {
            newMousePos.x -= newPos.x;
            newPos.x = 0;
        }
        if(newPos.y < 0)
        {
            newMousePos.y -= newPos.y;
            newPos.y = 0;
        }
        if(newPos.x > VIDEODRIVER.GetScreenWidth() - width_)
        {
            newMousePos.x -= newPos.x - (VIDEODRIVER.GetScreenWidth() - width_);
            newPos.x = VIDEODRIVER.GetScreenWidth() - width_;
        }
        if(newPos.y > VIDEODRIVER.GetScreenHeight() - height_)
        {
            newMousePos.y -= newPos.y - (VIDEODRIVER.GetScreenHeight() - height_);
            newPos.y = VIDEODRIVER.GetScreenHeight() - height_;
        }

        // Fix mouse position if moved too far
        if(newMousePos.x - mc.x || newMousePos.y - mc.y)
            VIDEODRIVER.SetMousePos(newMousePos);

        pos_ = newPos;
        lastMousePos = DrawPoint(mc.x, mc.y);
    }

    // beiden Buttons oben links und rechts prfen
    const Rect rec[2] =
    {
        GetLeftButtonRect(),
        GetRightButtonRect()
    };

    for(unsigned char i = 0; i < 2; ++i)
    {
        if(Coll(mc.x, mc.y, rec[i]))
        {
            if(mc.ldown)
                button_state[i] = BUTTON_PRESSED;
            else
                button_state[i] = BUTTON_HOVER;
        }
        else
            button_state[i] =  BUTTON_UP;
    }
}

bool IngameWindow::Draw_()
{
    if(isModal_)
    {
        SetActive(true);
        Close(false);
    }

    // Linkes oberes Teil
    glArchivItem_Bitmap* leftUpperImg = LOADER.GetImageN("resource", 36);
    leftUpperImg->Draw(pos_);
    // Rechtes oberes Teil
    glArchivItem_Bitmap* rightUpperImg = LOADER.GetImageN("resource", 37);
    rightUpperImg->Draw(pos_ + DrawPoint(width_ - rightUpperImg->getWidth(), 0));

    // Die beiden Buttons oben
    static const unsigned short ids[2][3] =
    {
        {47, 55, 50},
        {48, 56, 52}
    };

    // Titelleiste
    if(closeOnRightClick_ || !IsModal())
        LOADER.GetImageN("resource", ids[0][button_state[0]])->Draw(pos_);
    if(!IsModal())
        LOADER.GetImageN("resource", ids[1][button_state[1]])->Draw(pos_ + DrawPoint(width_ - 16, 0));


    // Breite berechnen
    unsigned title_width = width_ - leftUpperImg->getWidth() - rightUpperImg->getWidth();

    unsigned short title_index;
    if(active_)
        title_index = isMoving ? 44 : 43;
    else
        title_index = 42;

    glArchivItem_Bitmap& titleImg = *LOADER.GetImageN("resource", title_index);
    DrawPoint titleImgPos = pos_ + DrawPoint(leftUpperImg->getWidth(), 0);
    // Wieviel mal nebeneinanderzeichnen?
    unsigned short title_count = title_width / titleImg.getWidth();
    for(unsigned short i = 0; i < title_count; ++i)
    {
        titleImg.Draw(titleImgPos);
        titleImgPos.x += titleImg.getWidth();
    }

    // Rest zeichnen
    unsigned short rest = title_width % titleImg.getWidth();

    if(rest)
        titleImg.Draw(titleImgPos, rest, 0, 0, 0, rest, 0);

    // Text auf die Leiste
    NormalFont->Draw(pos_ + DrawPoint(width_, titleImg.getHeight()) / 2, title_, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_VCENTER, COLOR_YELLOW);

    if(!isMinimized_)
    {
        // Seitenleisten

        // Höhe
        glArchivItem_Bitmap* bottomBorderSideImg = LOADER.GetImageN("resource", 45);
        unsigned side_height = height_ - leftUpperImg->getHeight() - bottomBorderSideImg->getHeight();

        // Wieviel mal nebeneinanderzeichnen?
        glArchivItem_Bitmap* leftSideImg = LOADER.GetImageN("resource", 38);
        glArchivItem_Bitmap* rightSideImg = LOADER.GetImageN("resource", 39);
        title_count = side_height / leftSideImg->getHeight();
        DrawPoint leftImgPos = pos_ + DrawPoint(0, leftUpperImg->getHeight());
        DrawPoint rightImgPos = leftImgPos + DrawPoint(width_ - leftSideImg->getWidth(), 0);
        for(unsigned short i = 0; i < title_count; ++i)
        {
            leftSideImg->Draw(leftImgPos);
            rightSideImg->Draw(rightImgPos);
            rightImgPos.y = leftImgPos.y += leftSideImg->getHeight();    
        }

        // Rest zeichnen
        rest = side_height % leftSideImg->getHeight();

        if(rest)
        {
            leftSideImg->Draw(leftImgPos, 0, rest, 0, 0, 0, rest);
            rightSideImg->Draw(rightImgPos, 0, rest, 0, 0, 0, rest);
        }

        // Untere Leiste

        unsigned side_width = width_ - bottomBorderSideImg->getWidth() * 2;

        // Wieviel mal nebeneinanderzeichnen?
        glArchivItem_Bitmap* bottomBarImg = LOADER.GetImageN("resource", 40);
        title_count = side_width / bottomBarImg->getWidth();
        DrawPoint bottomImgPos = pos_ + DrawPoint(bottomBorderSideImg->getWidth(), GetIwBottomBoundary());
        for(unsigned short i = 0; i < title_count; ++i)
        {
            bottomBarImg->Draw(bottomImgPos);
            bottomImgPos.x += bottomBarImg->getWidth();
        }

        rest = side_width % bottomBarImg->getWidth();

        if(rest)
            bottomBarImg->Draw(bottomImgPos, rest, 0, 0, 0, rest, 0);

        // Clientbereich

        // überhaupt ne Clienttexture gewnscht?
        if(background)
            background->Draw(pos_ + contentOffset, GetIwWidth(), iwHeight, 0, 0, GetIwWidth(), iwHeight);

        // Msg_PaintBefore aufrufen vor den Controls
        Msg_PaintBefore();

        DrawControls();

        // Links und rechts unten die 2 kleinen Knäufe
        bottomBorderSideImg->Draw(pos_ + DrawPoint(0, height_ - bottomBorderSideImg->getHeight()));
        bottomBorderSideImg->Draw(pos_ + DrawPoint(width_ - bottomBorderSideImg->getWidth(), height_ - bottomBorderSideImg->getHeight()));
    }
    else
    {
        glArchivItem_Bitmap* bottomBorderSideImg = LOADER.GetImageN("resource", 45);
        glArchivItem_Bitmap* bottomBarImg = LOADER.GetImageN("resource", 40);
        unsigned side_width = width_ - bottomBorderSideImg->getWidth() * 2;
        title_count = side_width / bottomBarImg->getWidth();

        DrawPoint bottomImgPos = pos_ + DrawPoint(bottomBorderSideImg->getWidth(), 20);
        for(unsigned short i = 0; i < title_count; ++i)
        {
            bottomBarImg->Draw(bottomImgPos);
            bottomImgPos.x += bottomBarImg->getWidth();
        }

        rest = side_width % bottomBarImg->getWidth();

        if(rest)
            bottomBarImg->Draw(bottomImgPos, rest, 0, 0, 0, rest, 0);

        bottomBorderSideImg->Draw(pos_ + DrawPoint(0, bottomBorderSideImg->getHeight()));
        bottomBorderSideImg->Draw(pos_ + DrawPoint(width_ - bottomBorderSideImg->getWidth(), bottomBorderSideImg->getHeight()));
    }

    return true;
}

/// Verschiebt Fenster in die Bildschirmmitte
void IngameWindow::MoveToCenter()
{
    Move( (VIDEODRIVER.GetScreenWidth() - width_) / 2, (VIDEODRIVER.GetScreenHeight() - height_) / 2 );
}

/// Verschiebt Fenster neben die Maus
void IngameWindow::MoveNextToMouse()
{
    // Fenster soll neben der Maus dargestellt werden
    if(VIDEODRIVER.GetMouseX() + 20 + width_ < VIDEODRIVER.GetScreenWidth())
        pos_.x = VIDEODRIVER.GetMouseX() + 20;
    else
        pos_.x = VIDEODRIVER.GetScreenWidth() - width_;


    if(VIDEODRIVER.GetMouseY() - height_ / 2 < 0)
        pos_.y = 0;
    else if(VIDEODRIVER.GetMouseY() + 20 + height_ / 2 > VIDEODRIVER.GetScreenHeight())
        pos_.y = VIDEODRIVER.GetScreenHeight() - height_;
    else
        pos_.y = VIDEODRIVER.GetMouseY() - height_ / 2;
}

/// Weiterleitung von Nachrichten erlaubt oder nicht?
bool IngameWindow::IsMessageRelayAllowed() const
{
    // Wenn es minimiert wurde, sollen keine Nachrichten weitergeleitet werden
    return !isMinimized_;
}
