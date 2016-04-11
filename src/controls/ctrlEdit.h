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
#ifndef CTRLEDIT_H_INCLUDED
#define CTRLEDIT_H_INCLUDED

#pragma once

#include "Window.h"
#include "libutil/src/ucString.h"
class MouseCoords;
class glArchivItem_Font;
struct KeyEvent;

class ctrlEdit : public Window
{
    public:
        ctrlEdit(Window* parent, unsigned int id, unsigned short x, unsigned short y, unsigned short width, unsigned short height, TextureColor tc, glArchivItem_Font* font, unsigned short maxlength = 0, bool password = false, bool disabled = false, bool notify = false);
        /// setzt den Text.
        void SetText(const std::string& text);
        void SetText(const unsigned int text);

        std::string GetText() const;
        void SetFocus(bool focus = true) { newFocus_ = focus; }
        void SetDisabled(bool disabled = true) { this->isDisabled_ = disabled; }
        void SetNotify(bool notify = true) { this->notify_ = notify; }
        void SetMaxLength(unsigned short maxlength = 0) { this->maxLength_ = maxlength; }
        void SetNumberOnly(const bool activated) {this->numberOnly_ = activated; }

        void Msg_PaintAfter() override;
        bool Msg_LeftDown(const MouseCoords& mc) override;
        bool Msg_LeftDown_After(const MouseCoords& mc) override;
        bool Msg_KeyDown(const KeyEvent& ke) override;

    protected:
        bool Draw_() override;

    private:
        void AddChar(unsigned int c);
        void RemoveChar();
        void Notify();

        void CursorLeft() { if(cursorPos_ == 0) return; --cursorPos_; Notify(); };
        void CursorRight() { if(cursorPos_ == text_.length()) return; ++cursorPos_; Notify(); };

    private:
        unsigned short maxLength_;
        TextureColor texColor_;
        glArchivItem_Font* font_;
        bool isPassword_;
        bool isDisabled_;
        bool focus_;
        bool newFocus_;
        bool notify_;

        ucString text_;
        unsigned cursorPos_;
        unsigned viewStart_;

        bool numberOnly_;
};

#endif // !CTRLEDIT_H_INCLUDED
