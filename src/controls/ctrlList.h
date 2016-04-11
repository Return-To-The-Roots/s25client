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
#ifndef CTRLLIST_H_INCLUDED
#define CTRLLIST_H_INCLUDED

#pragma once

#include "Window.h"
#include <string>
#include <vector>
class MouseCoords;
class glArchivItem_Font;

class ctrlList : public Window
{
    public:
        ctrlList(Window* parent, unsigned int id, unsigned short x, unsigned short y, unsigned short width, unsigned short height, TextureColor tc, glArchivItem_Font* font);
        ~ctrlList() override;

        /// Größe verändern
        void Resize_(unsigned short width, unsigned short height) override;

        /// Neuen String zur Listbox hinzufügen.
        void AddString(const std::string& text);
        /// Verändert einen String
        void SetString(const std::string& text, const unsigned id);
        /// Listbox leeren.
        void DeleteAllItems();
        /// liefert den Wert einer Zeile.
        const std::string& GetItemText(unsigned short line) const;
        /// liefert den Wert der aktuell gewählten Zeile.
        const std::string& GetSelItemText() const { return GetItemText(selection_); };
        /// Vertauscht zwei Zeilen.
        void Swap(unsigned short first, unsigned short second);
        /// Löscht ein Element
        void Remove(const unsigned short index);

        unsigned short GetLineCount() const { return static_cast<unsigned short>(lines.size()); }
        int GetSelection() const { return selection_; };
        void SetSelection(unsigned selection)
        {
            if(static_cast<int>(selection) != selection_ && selection < lines.size())
            {
                selection_ = selection;
                if(parent_)
                    parent_->Msg_ListSelectItem(id_, selection);
            }
        }

        bool Msg_MouseMove(const MouseCoords& mc) override;
        bool Msg_LeftDown(const MouseCoords& mc) override;
        bool Msg_RightDown(const MouseCoords& mc) override;
        bool Msg_LeftUp(const MouseCoords& mc) override;
        bool Msg_WheelUp(const MouseCoords& mc) override;
        bool Msg_WheelDown(const MouseCoords& mc) override;
    protected:
        /// Zeichenmethode.
        bool Draw_() override;

    private:
        TextureColor tc;
        glArchivItem_Font* font;

        std::vector<std::string> lines;

        int selection_;
        int mouseover;
        unsigned int pagesize;

};

#endif // CTRLLIST_H_INCLUDED
