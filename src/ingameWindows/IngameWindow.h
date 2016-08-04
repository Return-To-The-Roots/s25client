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
#ifndef INGAMEWINDOW_H_INCLUDED
#define INGAMEWINDOW_H_INCLUDED

#pragma once

#include "Window.h"
#include <vector>

class glArchivItem_Bitmap;
class MouseCoords;
template <typename T> struct Point;

class IngameWindow : public Window
{

        /// For each id we save the last posistion of the window
        static std::vector< Point<unsigned short> > last_pos;
    public:
        IngameWindow(unsigned int id, unsigned short x, unsigned short y, unsigned short width, unsigned short height,
            const std::string& title, glArchivItem_Bitmap* background, bool modal = false, bool closeOnRightClick = true, Window* parent = NULL);
        ~IngameWindow() override;

        /// setzt die Fenster-ID.
        void SetID(unsigned int id) { this->id_ = id; }
        /// liefert die Fenster-ID.
        unsigned int GetID() const { return id_; }

        /// setzt den Hintergrund.
        void SetBackground(glArchivItem_Bitmap* background) { this->background = background; }
        /// liefert den Hintergrund.
        glArchivItem_Bitmap* GetBackground() const { return background; }

        /// setzt den Fenstertitel.
        void SetTitle(const std::string& title) { this->title_ = title; }
        /// liefert den Fenstertitel.
        const std::string& GetTitle() const { return title_; }

        /// setzt die ausgeklappte Höhe des Fensters.
        void SetIwHeight(unsigned short height) { this->iwHeight = height; if(!isMinimized_) this->height_ = height; }
        /// liefert die ausgeklappte Höhe des Fensters.
        unsigned short GetIwHeight() const { return iwHeight; }

        /// merkt das Fenster zum Schließen vor.
        void Close(bool closeme = true) { this->closeme = closeme; }
        /// soll das Fenster geschlossen werden.
        bool ShouldBeClosed() const { return closeme; }

        /// minimiert das Fenster.
        void SetMinimized(bool minimized = true);
        /// ist das Fenster minimiert?
        bool IsMinimized() const { return isMinimized_; }

        /// Can we close the window on right-click?
        /// If this is false and the window is modal the close button at the title bar will be hidden
        void SetCloseOnRightClick(bool closeOnRightClick) {closeOnRightClick_ = closeOnRightClick;}
        bool GetCloseOnRightClick() const {return closeOnRightClick_;}

        /// Toggle the modal property. Modal windows disallow focus change.
        /// To also disallow closing of the window via the title bar or right-click set close_on_right_click to false
        void SetModal(bool modal = true) { this->modal = modal; }
        /// ist das Fenster ein modales Fenster?
        bool IsModal() const { return modal; }

        void MouseLeftDown(const MouseCoords& mc);
        void MouseLeftUp(const MouseCoords& mc);
        void MouseMove(const MouseCoords& mc);

    protected:
        bool Draw_() override;

        /// Verschiebt Fenster in die Bildschirmmitte
        void MoveToCenter();
        /// Verschiebt Fenster neben die Maus
        void MoveNextToMouse();

        /// Weiterleitung von Nachrichten erlaubt oder nicht?
        bool IsMessageRelayAllowed() const override;

    protected:
        unsigned short iwHeight;
        std::string title_;
        glArchivItem_Bitmap* background;
        unsigned short last_x;
        unsigned short last_y;
        bool last_down;
        bool last_down2;
        ButtonState button_state[2];

        Rect GetLeftButtonRect()  const { return Rect(x_, y_, 16, 16); }
        Rect GetRightButtonRect() const { return Rect(static_cast<unsigned short>(x_ + width_ - 16), y_, 16, 16); }

    private:
        bool modal;
        bool closeme;
        bool isMinimized_;
        bool move;
        bool closeOnRightClick_;
};

#endif // !INGAMEWINDOW_H_INCLUDED
