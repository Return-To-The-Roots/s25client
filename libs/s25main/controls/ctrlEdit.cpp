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
#include "driver/MouseCoords.h"
#include "drivers/VideoDriverWrapper.h"
#include "ogl/FontStyle.h"
#include "ogl/glArchivItem_Font.h"
#include "s25util/StringConversion.h"
#include <utf8.h>

ctrlEdit::ctrlEdit(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, glArchivItem_Font* font,
                   unsigned short maxlength, bool password, bool disabled, bool notify)
    : Window(parent, id, pos, size), maxLength_(maxlength), texColor_(tc), font_(font), isPassword_(password), isDisabled_(disabled),
      focus_(false), newFocus_(false), notify_(notify), numberOnly_(false)
{
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

/**
 *  zeichnet das Fenster.
 *
 *  @todo muss alles überarbeitet werden
 */
void ctrlEdit::Draw_()
{
    // Box malen
    Draw3D(Rect(GetDrawPos(), GetSize()), texColor_, false);

    std::u32string dtext;

    // Text zeichnen
    if(isPassword_)
        dtext = std::u32string(text_.length(), '*');
    else
        dtext = text_;

    const unsigned max_width = GetSize().x - 8 - font_->getDx();
    unsigned max;
    font_->getWidth(dtext.substr(viewStart_), unsigned(text_.length()) - viewStart_, max_width, &max);
    while(max > 0 && text_.length() - viewStart_ > max)
    {
        ++viewStart_;
        font_->getWidth(&dtext[viewStart_], unsigned(text_.length()) - viewStart_, max_width, &max);
    }

    if(viewStart_ > 0)
    {
        font_->getWidth(dtext, unsigned(text_.length()), max_width, &max);
        while(viewStart_ > 0 && unsigned(text_.length()) - viewStart_ <= max)
        {
            --viewStart_;

            if(max > 0)
                font_->getWidth(&dtext[viewStart_], unsigned(text_.length()) - viewStart_, max_width, &max);
        }
    }

    unsigned short start = viewStart_;
    if(cursorPos_ > 5 && cursorPos_ - 5 < viewStart_)
        start = cursorPos_ - 5;
    if(cursorPos_ <= 5)
        start = 0;
    font_->Draw(GetDrawPos() + DrawPoint(4, GetSize().y / 2), dtext.substr(start), FontStyle::VCENTER, (focus_ ? 0xFFFFA000 : COLOR_YELLOW),
                0, GetSize().x - 8);

    // Alle 500ms Cursor für 500ms anzeigen
    if(focus_ && !isDisabled_ && VIDEODRIVER.GetTickCount() % 1000 < 500)
    {
        DrawPoint cursorDrawPos = GetDrawPos();
        if(cursorPos_ > start)
            cursorDrawPos.x += font_->getWidth(&dtext[start], cursorPos_ - start) + 4;
        else
            cursorDrawPos.x += 5;
        cursorDrawPos.y += (GetSize().y - (font_->getHeight() + 2)) / 2;

        DrawRectangle(Rect(cursorDrawPos, Extent(1, font_->getHeight() + 2)), 0xFFFFA000);
    }
}

/**
 *  fügt ein Zeichen zum Text hinzu
 *
 *  @param[in] text Das Zeichen
 */
void ctrlEdit::AddChar(unsigned c)
{
    // Number-only text fields accept numbers only ;)
    if(numberOnly_ && !(c >= '0' && c <= '9'))
        return;

    if(maxLength_ > 0 && text_.size() >= maxLength_)
        return;

    text_.insert(cursorPos_, 1, c);
    CursorRight();
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

void ctrlEdit::Msg_PaintAfter()
{
    focus_ = newFocus_;
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
    static const std::u32string delimiters = U" \t\n-+=";

    // hat das Steuerelement den Fokus?
    if(!focus_)
        return false;

    switch(ke.kt)
    {
        default:
            return false;
        // Wird bereits über Char geliefert !!
        case KT_SPACE: // Leertaste
        {
            AddChar(0x20);
        }
        break;

        case KT_LEFT: // Cursor nach Links
        {
            // Blockweise nach links, falls Strg gedrückt
            if(ke.ctrl)
            {
                // Erst über alle Trennzeichen hinweg
                while(cursorPos_ > 0 && delimiters.find(text_[cursorPos_ - 1]) != std::u32string::npos)
                {
                    CursorLeft();
                }

                // Und dann über alles, was kein Trenner ist
                while(cursorPos_ > 0 && delimiters.find(text_[cursorPos_ - 1]) == std::u32string::npos)
                {
                    CursorLeft();
                }
            } else
                CursorLeft(); // just one step
        }
        break;

        case KT_RIGHT: // Cursor nach Rechts
        {
            // Blockweise nach rechts, falls Strg gedrückt
            if(ke.ctrl)
            {
                // Erst über alle Trennzeichen hinweg
                while(cursorPos_ + 1 < text_.length() && delimiters.find(text_[cursorPos_ + 1]) != std::u32string::npos)
                {
                    CursorRight();
                }
                // Und dann über alles, was kein Trenner ist
                while(cursorPos_ + 1 < text_.length() && delimiters.find(text_[cursorPos_ + 1]) == std::u32string::npos)
                {
                    CursorRight();
                }
            } else
                CursorRight(); // just one step
        }
        break;

        case KT_CHAR: // Zeichen eingegeben
        {
            if(!isDisabled_ && font_->CharExist(ke.c))
                AddChar(ke.c);
        }
        break;

        case KT_BACKSPACE: // Backspace gedrückt
        {
            if(!isDisabled_)
                RemoveChar();
        }
        break;

        case KT_DELETE: // Entfernen gedrückt
        {
            if(!isDisabled_ && cursorPos_ < text_.length())
            {
                CursorRight();
                RemoveChar();
            }
        }
        break;

        case KT_RETURN: // Enter gedrückt
        {
            if(!isDisabled_ && GetParent())
                GetParent()->Msg_EditEnter(GetID());
        }
        break;

        case KT_HOME: // Pos1 gedrückt
        {
            while(cursorPos_ > 0)
                CursorLeft();
        }
        break;

        case KT_END: // Ende gedrückt
        {
            while(cursorPos_ < text_.length())
                CursorRight();
        }
        break;
    }

    return true;
}
