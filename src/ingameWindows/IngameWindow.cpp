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
#include "IngameWindow.h"

#include "drivers/VideoDriverWrapper.h"
#include "Loader.h"
#include "CollisionDetection.h"
#include "driver/src/MouseCoords.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glArchivItem_Font.h"
#include "ogl/glArchivItem_Sound.h"
#include "gameData/const_gui_ids.h"
#include <cstring>

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

const unsigned MAX_POS_SAVE_ENTRIES = CGI_MERCHANDISE_STATISTICS + 1;
std::vector< Point<unsigned short> > IngameWindow::last_pos(MAX_POS_SAVE_ENTRIES, Point<unsigned short>::Invalid());

IngameWindow::IngameWindow(unsigned int id, unsigned short x, unsigned short y, unsigned short width, unsigned short height,
                           const std::string& title, glArchivItem_Bitmap* background, bool modal, bool close_on_right_click, Window* parent)
    : Window(x, y, id, parent, width, height),
      iwHeight(height), title_(title), background(background), last_x(0), last_y(0),
      last_down(false), last_down2(false), modal(modal), closeme(false), isMinimized_(false), move(false), close_on_right_click(close_on_right_click)
{
    memset(button_state, BUTTON_UP, sizeof(ButtonState) * 2);

    // Load last position or center the window
    if(x == 0xFFFF)
    {
        MoveToCenter();

        if(id < MAX_POS_SAVE_ENTRIES)
        {
            Point<unsigned short> pos = last_pos[id];
            if(pos.x != 0xffff)
            {
                Move(pos.x, pos.y);
            }
        }


    }
    else if(x == 0xFFFE)
    {
        MoveNextToMouse();
    }
}

IngameWindow::~IngameWindow()
{
    // Possibly save our old position
    if(id_ < MAX_POS_SAVE_ENTRIES)
        last_pos[id_] = Point<unsigned short>(x_, y_);
}

void IngameWindow::SetMinimized(bool minimized)
{
    this->isMinimized_ = minimized;
    SetHeight(minimized ? 31 : iwHeight);
}

void IngameWindow::MouseLeftDown(const MouseCoords& mc)
{
    // Maus muss sich auf der Titelleiste befinden
    Rect title_rect(
        static_cast<unsigned short>(x_ + LOADER.GetImageN("resource", 36)->getWidth()),
        y_,
        static_cast<unsigned short>(width_ - LOADER.GetImageN("resource", 36)->getWidth() - LOADER.GetImageN("resource", 37)->getWidth()),
        LOADER.GetImageN("resource", 43)->getHeight()
        );

    if(Coll(mc.x, mc.y, title_rect))
    {
        // Start mit Bewegung
        move = true;
        last_x = mc.x;
        last_y = mc.y;
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
    move = false;

    // beiden Buttons oben links und rechts prfen
    const Rect rec[2] =
    {
        GetLeftButtonRect(),
        GetRightButtonRect()
    };

    for(unsigned char i = 0; i < 2; ++i)
    {
        button_state[i] = BUTTON_UP;
        if(Coll(mc.x, mc.y, rec[i]))
        {
            if(i)
            {
                SetMinimized(!GetMinimized());
                LOADER.GetSoundN("sound", 113)->Play(255, false);
            }
            else if(!modal)
                Close();
        }
    }
}

void IngameWindow::MouseMove(const MouseCoords& mc)
{
    // Fenster bewegen, wenn die Bewegung aktiviert wurde
    if(move)
    {
        int nx, ny;
        int NewMouseX = mc.x;
        int NewMouseY = mc.y;

        nx = x_ + (mc.x - last_x);
        ny = y_ + (mc.y - last_y);
        if(nx < 0)
        {
            NewMouseX -= nx;
            nx = 0;
        }
        if(ny < 0)
        {
            NewMouseY -= ny;
            ny = 0;
        }
        if(nx > VIDEODRIVER.GetScreenWidth() - width_)
        {
            NewMouseX -= nx - (VIDEODRIVER.GetScreenWidth() - width_);
            nx = VIDEODRIVER.GetScreenWidth() - width_;
        }
        if(ny > VIDEODRIVER.GetScreenHeight() - height_)
        {
            NewMouseY -= ny - (VIDEODRIVER.GetScreenHeight() - height_);
            ny = VIDEODRIVER.GetScreenHeight() - height_;
        }

        // Fix mouse position if moved too far
        if(NewMouseX - mc.x || NewMouseY - mc.y)
            VIDEODRIVER.SetMousePos(NewMouseX, NewMouseY);

        x_ = (unsigned short)nx;
        y_ = (unsigned short)ny;

        last_x = mc.x;
        last_y = mc.y;
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
    if(modal)
    {
        SetActive(true);
        Close(false);
    }

    // Linkes oberes Teil
    glArchivItem_Bitmap* leftUpperImg = LOADER.GetImageN("resource", 36);
    leftUpperImg->Draw(x_, y_, 0, 0, 0, 0, 0, 0);
    // Rechtes oberes Teil
    glArchivItem_Bitmap* rightUpperImg = LOADER.GetImageN("resource", 37);
    rightUpperImg->Draw(x_ + width_ - rightUpperImg->getWidth(), y_, 0, 0, 0, 0, 0, 0);

    // Die beiden Buttons oben
    static const unsigned short ids[2][3] =
    {
        {47, 55, 50},
        {48, 56, 52}
    };

    // Titelleiste
    if(!modal)
        LOADER.GetImageN("resource", ids[0][button_state[0]])->Draw(x_, y_, 0, 0, 0, 0, 0, 0);

    LOADER.GetImageN("resource", ids[1][button_state[1]])->Draw(x_ + width_ - 16, y_, 0, 0, 0, 0, 0, 0);


    // Breite berechnen
    unsigned title_width = width_ - leftUpperImg->getWidth() - rightUpperImg->getWidth();

    // Wieviel mal nebeneinanderzeichnen?
    unsigned short title_count = title_width / LOADER.GetImageN("resource", 43)->getWidth();

    unsigned short title_index = 42;
    if(active_)
    {
        if(move)
            title_index = 44;
        else
            title_index = 43;
    }

    glArchivItem_Bitmap& titleImg = *LOADER.GetImageN("resource", title_index);
    for(unsigned short i = 0; i < title_count; ++i)
        titleImg.Draw(x_ + leftUpperImg->getWidth() + i * titleImg.getWidth(), y_, 0, 0, 0, 0, 0, 0);

    // Rest zeichnen
    unsigned short rest = title_width % titleImg.getWidth();

    if(rest)
        titleImg.Draw(x_ + leftUpperImg->getWidth() + title_count * titleImg.getWidth(), y_, rest, 0, 0, 0, rest, 0);

    // Text auf die Leiste
    NormalFont->Draw( x_ + width_ / 2, y_ + titleImg.getHeight() / 2, title_, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_VCENTER, COLOR_YELLOW);

    if(!isMinimized_)
    {
        // Seitenleisten

        // Höhe
        glArchivItem_Bitmap* leftLowerImg = LOADER.GetImageN("resource", 45);
        unsigned side_height = height_ - leftUpperImg->getHeight() - leftLowerImg->getHeight();

        // Wieviel mal nebeneinanderzeichnen?
        glArchivItem_Bitmap* leftSideImg = LOADER.GetImageN("resource", 38);
        title_count = side_height / leftSideImg->getHeight();

        for(unsigned short i = 0; i < title_count; ++i)
        {
            leftSideImg->Draw(x_, y_ + leftUpperImg->getHeight() + i * leftSideImg->getHeight(), 0, 0, 0, 0, 0, 0);
            LOADER.GetImageN("resource", 39)->Draw(x_ + width_ - leftSideImg->getWidth(), y_ + leftUpperImg->getHeight() + i * leftSideImg->getHeight(), 0, 0, 0, 0, 0, 0);
            //LOADER.GetImageN("resource", 43)->Draw(x+width-LOADER.GetImageN("resource", 38)->getWidth(),y);
        }

        // Rest zeichnen
        rest = side_height % leftSideImg->getHeight();

        if(rest)
        {
            leftSideImg->Draw(x_, y_ + height_ - rest - leftLowerImg->getHeight(), 0, rest, 0, 0, 0, rest);
            LOADER.GetImageN("resource", 39)->Draw(x_ + width_ - leftSideImg->getWidth(), y_ + leftUpperImg->getHeight() + title_count * leftSideImg->getHeight(), 0, rest, 0, 0, 0, rest);
        }

        // Untere Leiste

        unsigned side_width = width_ - leftLowerImg->getWidth() * 2;

        // Wieviel mal nebeneinanderzeichnen?
        glArchivItem_Bitmap* bottomBarImg = LOADER.GetImageN("resource", 40);
        title_count = side_width / bottomBarImg->getWidth();

        for(unsigned short i = 0; i < title_count; ++i)
            bottomBarImg->Draw(x_ + leftLowerImg->getWidth() + i * bottomBarImg->getWidth(), y_ + iwHeight - bottomBarImg->getHeight(), 0, 0, 0, 0, 0, 0);

        rest = side_width % bottomBarImg->getWidth();

        if(rest)
            bottomBarImg->Draw(x_ + leftLowerImg->getWidth() + title_count * bottomBarImg->getWidth(), y_ + iwHeight - bottomBarImg->getHeight(), rest, 0, 0, 0, rest, 0);

        // Clientbereich

        // überhaupt ne Clienttexture gewnscht?
        if(background)
        {
            // Bereich ausrechnen
            unsigned client_width = width_ - 20;
            unsigned client_height = iwHeight - 31;

            background->Draw(this->x_ + leftSideImg->getWidth(), this->y_ + leftUpperImg->getHeight(), client_width, client_height, 0, 0, client_width, client_height);
        }

        // Links und rechts unten die 2 kleinen Knäufe
        leftLowerImg->Draw(x_, y_ + iwHeight - leftLowerImg->getHeight(), 0, 0, 0, 0, 0, 0);
        leftLowerImg->Draw(x_ + width_ - leftLowerImg->getWidth(), y_ + iwHeight - leftLowerImg->getHeight(), 0, 0, 0, 0, 0, 0);

        // Msg_PaintBefore aufrufen vor den Controls
        Msg_PaintBefore();

        DrawControls();
    }
    else
    {
        glArchivItem_Bitmap& imgBorderSide = *LOADER.GetImageN("resource", 45);
        glArchivItem_Bitmap& imgTitleBar = *LOADER.GetImageN("resource", 40);
        unsigned side_width = width_ - imgBorderSide.getWidth() * 2;
        title_count = side_width / imgTitleBar.getWidth();

        for(unsigned short i = 0; i < title_count; ++i)
            imgTitleBar.Draw(x_ + imgBorderSide.getWidth() + i * imgTitleBar.getWidth(), y_ + 20, 0, 0, 0, 0, 0, 0);

        rest = side_width % imgTitleBar.getWidth();

        if(rest)
            imgTitleBar.Draw(x_ + imgBorderSide.getWidth() + title_count * imgTitleBar.getWidth(), y_ + 20, rest, 0, 0, 0, rest, 0);

        imgBorderSide.Draw(x_, y_ + 16, 0, 0, 0, 0, 0, 0);
        imgBorderSide.Draw(x_ + width_ - 16, y_ + 16, 0, 0, 0, 0, 0, 0);
    }

    return true;
}

/// Verschiebt Fenster in die Bildschirmmitte
void IngameWindow::MoveToCenter()
{
    // Ja, also zentrieren
    Move( (VIDEODRIVER.GetScreenWidth() - width_) / 2, (VIDEODRIVER.GetScreenHeight() - height_) / 2 );

}

/// Verschiebt Fenster neben die Maus
void IngameWindow::MoveNextToMouse()
{
    // Fenster soll neben der Maus dargestellt werden
    if(VIDEODRIVER.GetMouseX() + 20 + width_ < VIDEODRIVER.GetScreenWidth())
        this->x_ = VIDEODRIVER.GetMouseX() + 20;
    else
        this->x_ = VIDEODRIVER.GetScreenWidth() - width_;


    if(VIDEODRIVER.GetMouseY() - height_ / 2 < 0)
        this->y_ = 0;
    else if(VIDEODRIVER.GetMouseY() + 20 + height_ / 2 > VIDEODRIVER.GetScreenHeight())
        this->y_ = VIDEODRIVER.GetScreenHeight() - height_;
    else
        this->y_ = VIDEODRIVER.GetMouseY() - height_ / 2;
}

/// Weiterleitung von Nachrichten erlaubt oder nicht?
bool IngameWindow::IsMessageRelayAllowed() const
{
    // Wenn es minimiert wurde, sollen keine Nachrichten weitergeleitet werden
    return !isMinimized_;
}
