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

#include "rttrDefines.h" // IWYU pragma: keep
#include "ctrlEdit.h"
#include "CollisionDetection.h"
#include "ctrlTextDeepening.h"
#include "driver/MouseCoords.h"
#include "drivers/VideoDriverWrapper.h"
#include "ogl/FontStyle.h"
#include "ogl/glArchivItem_Font.h"
#include "s25util/StringConversion.h"
#include <utf8.h>

ctrlEdit::ctrlEdit(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, glArchivItem_Font* font,
                   unsigned short maxlength, bool password, bool disabled, bool notify)
    : Window(parent, id, pos, size), maxLength_(maxlength), isPassword_(password), isDisabled_(disabled), focus_(false), newFocus_(false),
      notify_(notify), numberOnly_(false)
{
    txtCtrl = static_cast<ctrlTextDeepening*>(
      AddTextDeepening(0, DrawPoint(0, 0), size, tc, "", font, COLOR_YELLOW, FontStyle::LEFT | FontStyle::VCENTER));
    SetText("");
}

/**
 *  setzt den Text.
 *
 *  @param[in] text Der Text.
 */
void ctrlEdit::SetText(const std::string& text)
{
    cursorPos_ = 0;
    viewStart_ = 0;

    text_.clear();
    std::u32string tmp = utf8::utf8to32(text);

    for(const auto c : tmp)
        AddChar(c);
}

void ctrlEdit::SetText(const unsigned text)
{
    SetText(s25util::toStringClassic(text));
}

std::string ctrlEdit::GetText() const
{
    return utf8::utf32to8(text_);
}

static void removeFirstCharFromString(std::string& str)
{
    auto it = str.begin();
    utf8::unchecked::next(it);
    str.erase(str.begin(), it);
}

void ctrlEdit::UpdateInternalText()
{
    std::u32string dtext = (isPassword_) ? std::u32string(text_.size(), '*') : text_;
    const auto* font = txtCtrl->GetFont();
    const unsigned max_width = GetSize().x - 8 - font->getDx();
    unsigned max;
    std::string curText = utf8::utf32to8(dtext.substr(viewStart_));
    font->getWidth(curText, max_width, &max);
    // Increase viewStart until string can be fully shown (removing chars from the front)
    while(max > 0 && curText.length() > max)
    {
        ++viewStart_;
        removeFirstCharFromString(curText);
        font->getWidth(curText, max_width, &max);
    }
    // Decrease viewStart as long as full text is shown (adding chars at the front)
    while(viewStart_ > 0 && curText.length() <= max)
    {
        --viewStart_;
        curText = utf8::utf32to8(dtext.substr(viewStart_));
        font->getWidth(curText, max_width, &max);
    }

    // Show (up to) 5 chars before the cursor
    if(viewStart_ + 5 > cursorPos_)
    {
        viewStart_ = std::max(0, static_cast<int>(cursorPos_) - 5);
        curText = utf8::utf32to8(dtext.substr(viewStart_));
    }
    if(cursorPos_ > viewStart_)
        cursorOffsetX_ = font->getWidth(utf8::utf32to8(dtext.substr(viewStart_, cursorPos_ - viewStart_)));
    else
        cursorOffsetX_ = 0;
    txtCtrl->SetText(curText);
}

inline void ctrlEdit::CursorLeft()
{
    if(cursorPos_ == 0)
        return;
    --cursorPos_;
    UpdateInternalText();
}

inline void ctrlEdit::CursorRight()
{
    if(cursorPos_ == text_.length())
        return;
    ++cursorPos_;
    UpdateInternalText();
}

/**
 *  zeichnet das Fenster.
 *
 *  @todo muss alles überarbeitet werden
 */
void ctrlEdit::Draw_()
{
    Window::Draw_();
    // Alle 500ms Cursor für 500ms anzeigen
    if(focus_ && !isDisabled_ && VIDEODRIVER.GetTickCount() % 1000 < 500)
    {
        const auto fontHeight = txtCtrl->GetFont()->getHeight();
        DrawPoint cursorDrawPos = GetDrawPos();
        cursorDrawPos.x += cursorOffsetX_ + ctrlTextDeepening::borderSize.x;
        cursorDrawPos.y += (GetSize().y - (fontHeight + 2)) / 2;

        DrawRectangle(Rect(cursorDrawPos, Extent(1, fontHeight + 2)), 0xFFFFA000);
    }
}

/**
 *  fügt ein Zeichen zum Text hinzu
 *
 *  @param[in] text Das Zeichen
 */
void ctrlEdit::AddChar(char32_t c)
{
    // Number-only text fields accept numbers only ;)
    if(numberOnly_ && !(c >= '0' && c <= '9'))
        return;

    if(maxLength_ > 0 && text_.size() >= maxLength_)
        return;

    text_.insert(cursorPos_, 1, c);
    CursorRight();
    Notify();
}

/**
 *  entfernt ein Zeichen
 */
void ctrlEdit::RemoveChar()
{
    if(cursorPos_ > 0 && text_.length() > 0)
    {
        text_.erase(cursorPos_ - 1, 1);

        // View verschieben
        while(text_.length() > 0 && text_.length() <= viewStart_)
            --viewStart_;

        CursorLeft();
        Notify();
    }
}

/**
 *  benachrichtigt das Parent ("OnChange")
 */
void ctrlEdit::Notify()
{
    if(!notify_ || !GetParent())
        return;

    GetParent()->Msg_EditChange(GetID());
}

void ctrlEdit::Resize(const Extent& newSize)
{
    Window::Resize(newSize);
    UpdateInternalText();
}

void ctrlEdit::Msg_PaintAfter()
{
    if(focus_ != newFocus_)
    {
        focus_ = newFocus_;
        txtCtrl->SetTextColor(focus_ ? 0xFFFFA000 : COLOR_YELLOW);
    }
    Window::Msg_PaintAfter();
}

/**
 *  Maustaste-gedrückt Callback
 */
bool ctrlEdit::Msg_LeftDown(const MouseCoords& mc)
{
    if((newFocus_ = IsPointInRect(mc.GetPos(), GetDrawRect())))
        return false; /// vorläufig, um Fokus zu für andere Edit-Felder zu kriegen, damit es zu keinen Doppelfokus kommt
    else
        return false;
}

/**
 *  Taste-gedrückt Callback
 */
bool ctrlEdit::Msg_KeyDown(const KeyEvent& ke)
{
    auto isDelimiter = [](char32_t c) {
        for(const auto cur : U" \t\n-+=")
        {
            if(cur == c)
                return true;
        }
        return false;
    };

    // hat das Steuerelement den Fokus?
    if(!focus_)
        return false;

    switch(ke.kt)
    {
        default:
            return false;
        // Wird bereits über Char geliefert !!
        case KT_SPACE: // Leertaste
            AddChar(0x20);
            break;

        case KT_LEFT: // Cursor nach Links
            // Blockweise nach links, falls Strg gedrückt
            if(ke.ctrl)
            {
                // Erst über alle Trennzeichen hinweg
                while(cursorPos_ > 0 && isDelimiter(text_[cursorPos_ - 1]))
                {
                    --cursorPos_;
                }

                // Und dann über alles, was kein Trenner ist
                while(cursorPos_ > 0 && !isDelimiter(text_[cursorPos_ - 1]))
                {
                    --cursorPos_;
                }
                UpdateInternalText();
            } else
                CursorLeft(); // just one step
            break;

        case KT_RIGHT: // Cursor nach Rechts
            // Blockweise nach rechts, falls Strg gedrückt
            if(ke.ctrl)
            {
                // Erst über alle Trennzeichen hinweg
                while(cursorPos_ + 1 < text_.length() && isDelimiter(text_[cursorPos_ + 1]))
                {
                    ++cursorPos_;
                }
                // Und dann über alles, was kein Trenner ist
                while(cursorPos_ + 1 < text_.length() && !isDelimiter(text_[cursorPos_ + 1]))
                {
                    ++cursorPos_;
                }
                UpdateInternalText();
            } else
                CursorRight(); // just one step
            break;

        case KT_CHAR: // Zeichen eingegeben
            if(!isDisabled_ && txtCtrl->GetFont()->CharExist(ke.c))
                AddChar(ke.c);
            break;

        case KT_BACKSPACE: // Backspace gedrückt
            if(!isDisabled_)
                RemoveChar();
            break;

        case KT_DELETE: // Entfernen gedrückt
            if(!isDisabled_ && cursorPos_ < text_.length())
            {
                CursorRight();
                RemoveChar();
            }
            break;

        case KT_RETURN: // Enter gedrückt
            if(!isDisabled_ && GetParent())
                GetParent()->Msg_EditEnter(GetID());
            break;

        case KT_HOME: // Pos1 gedrückt
            if(cursorPos_ > 0)
            {
                cursorPos_ = 0;
                UpdateInternalText();
            }
            break;

        case KT_END: // Ende gedrückt
            if(cursorPos_ < text_.length())
            {
                cursorPos_ = text_.length();
                UpdateInternalText();
            }
            break;
    }

    return true;
}
