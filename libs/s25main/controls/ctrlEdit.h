// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
