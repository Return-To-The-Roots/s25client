// $Id: IngameWindow.h 9357 2014-04-25 15:35:25Z FloSoft $
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
#ifndef INGAMEWINDOW_H_INCLUDED
#define INGAMEWINDOW_H_INCLUDED

#pragma once

#include "Window.h"
#include "const_gui_ids.h"

class IngameWindow : public Window
{

        /// For each id we save the last posistion of the window
        static std::vector< Point<unsigned short> > last_pos;
    public:
        /// Konstruktor von @p IngameWindow.
        IngameWindow(unsigned int id, unsigned short x, unsigned short y, unsigned short width, unsigned short height, const std::string& title, glArchivItem_Bitmap* background, bool modal = false, bool close_on_right_click = true);
        /// Destruktor von @p IngameWindow.
        ~IngameWindow(void);

        /// setzt die Fenster-ID.
        void SetID(unsigned int id) { this->id = id; }
        /// liefert die Fenster-ID.
        unsigned int GetID(void) { return id; }

        /// setzt den Hintergrund.
        void SetBackground(glArchivItem_Bitmap* background) { this->background = background; }
        /// liefert den Hintergrund.
        glArchivItem_Bitmap* GetBackground(void) { return background; }

        /// setzt den Fenstertitel.
        void SetTitle(const std::string& title) { this->title = title; }
        /// liefert den Fenstertitel.
        const std::string& GetTitle(void) { return title; }

        /// setzt die ausgeklappte Höhe des Fensters.
        void SetIwHeight(unsigned short height) { this->iwHeight = height; if(!minimized) this->height = height; }
        /// liefert die ausgeklappte Höhe des Fensters.
        unsigned short GetIwHeight(void) const { return iwHeight; }

        /// merkt das Fenster zum Schließen vor.
        void Close(bool closeme = true) { this->closeme = closeme; }
        /// soll das Fenster geschlossen werden.
        bool ShouldBeClosed() { return closeme; }

        /// minimiert das Fenster.
        void SetMinimized(bool minimized = true);
        /// ist das Fenster minimiert?
        bool GetMinimized() { return minimized; }

        /// Fenster wird bei Rechtsklick geschlossen?
        void SetCloseOnRightClick(bool close_on_right_click) {this->close_on_right_click = close_on_right_click;}
        bool GetCloseOnRightClick() {return(close_on_right_click);}

        /// "modalisiert" das Fenster.
        void SetModal(bool modal = true) { this->modal = modal; }
        /// ist das Fenster ein modales Fenster?
        bool GetModal(void) { return modal; }

        void MouseLeftDown(const MouseCoords& mc);
        void MouseLeftUp(const MouseCoords& mc);
        void MouseMove(const MouseCoords& mc);

    protected:
        virtual bool Draw_(void);

        /// Verschiebt Fenster in die Bildschirmmitte
        void MoveToCenter();
        /// Verschiebt Fenster neben die Maus
        void MoveNextToMouse();

        /// Weiterleitung von Nachrichten erlaubt oder nicht?
        bool IsMessageRelayAllowed() const;

    protected:
        unsigned short iwHeight;
        std::string title;
        glArchivItem_Bitmap* background;
        unsigned short last_x;
        unsigned short last_y;
        bool last_down;
        bool last_down2;
        ButtonState button_state[2];

    private:
        bool modal;
        bool closeme;
        bool minimized;
        bool move;
        bool close_on_right_click;
};

#endif // !INGAMEWINDOW_H_INCLUDED
