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

#pragma once

#include "Window.h"

class MouseCoords;
class glFont;
class ctrlTextDeepening;
struct KeyEvent;

class ctrlEdit : public Window
{
public:
    ctrlEdit(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, const glFont* font,
             unsigned short maxlength = 0, bool password = false, bool disabled = false, bool notify = false);
    /// setzt den Text.
    void SetText(const std::string& text);
    void SetText(unsigned text);

    std::string GetText() const;
    void SetFocus(bool focus = true);
    bool HasFocus() const { return focus_; }
    void SetDisabled(bool disabled = true) { this->isDisabled_ = disabled; }
    void SetNotify(bool notify = true) { this->notify_ = notify; }
    void SetNumberOnly(const bool activated) { this->numberOnly_ = activated; }

    void Resize(const Extent& newSize) override;

    bool Msg_LeftDown(const MouseCoords& mc) override;
    bool Msg_KeyDown(const KeyEvent& ke) override;

protected:
    void Draw_() override;

private:
    void AddChar(char32_t c);
    void RemoveChar();
    void Notify();
    void UpdateInternalText();

    void CursorLeft();
    void CursorRight();

private:
    unsigned short maxLength_;
    ctrlTextDeepening* txtCtrl;
    bool isPassword_;
    bool isDisabled_;
    bool focus_ = false;
    bool notify_;

    std::u32string text_;
    /// Position of cursor in text (in UTF32 chars)
    unsigned cursorPos_ = 0;
    /// Offset of the cursor from the start of the text start position
    unsigned cursorOffsetX_ = 0;
    unsigned viewStart_ = 0;

    bool numberOnly_ = false;
};
