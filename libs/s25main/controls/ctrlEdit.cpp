// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlEdit.h"
#include "CollisionDetection.h"
#include "ctrlTextDeepening.h"
#include "driver/MouseCoords.h"
#include "drivers/VideoDriverWrapper.h"
#include "helpers/containerUtils.h"
#include "ogl/FontStyle.h"
#include "ogl/glFont.h"
#include "s25util/StringConversion.h"
#include <s25util/utf8.h>
#include <boost/nowide/detail/utf.hpp>
#include <numeric>

ctrlEdit::ctrlEdit(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                   const glFont* font, unsigned short maxlength, bool password, bool disabled, bool notify)
    : Window(parent, id, pos, size), maxLength_(maxlength), isPassword_(password), isDisabled_(disabled),
      notify_(notify)
{
    txtCtrl = static_cast<ctrlTextDeepening*>(
      AddTextDeepening(0, DrawPoint(0, 0), size, tc, "", font, COLOR_YELLOW, FontStyle::LEFT | FontStyle::VCENTER));
    UpdateInternalText();
}

/**
 *  setzt den Text.
 *
 *  @param[in] text Der Text.
 */
void ctrlEdit::SetText(const std::string& text)
{
    text_ = s25util::utf8to32(text);
    if(numberOnly_)
        helpers::erase_if(text_, [](char32_t c) { return !(c >= '0' && c <= '9'); });
    if(maxLength_ > 0 && text_.size() > maxLength_)
        text_.resize(maxLength_);

    viewStart_ = 0;
    cursorPos_ = text_.length();
    UpdateInternalText();
    Notify();
}

void ctrlEdit::SetText(const unsigned text)
{
    SetText(s25util::toStringClassic(text));
}

std::string ctrlEdit::GetText() const
{
    return s25util::utf32to8(text_);
}

void ctrlEdit::SetFocus(bool focus)
{
    if(focus_ != focus)
    {
        focus_ = focus;
        txtCtrl->SetTextColor(focus_ ? 0xFFFFA000 : COLOR_YELLOW);
    }
}

static void removeFirstCharFromString(std::string& str)
{
    auto it = str.begin();
    boost::nowide::detail::utf::utf_traits<char>::decode_valid(it);
    str.erase(str.begin(), it);
}

void ctrlEdit::UpdateInternalText()
{
    if(text_.empty())
    {
        viewStart_ = 0;
        cursorPos_ = 0;
        cursorOffsetX_ = 0;
        txtCtrl->SetText("");
    } else
    {
        std::u32string dtext = (isPassword_) ? std::u32string(text_.size(), '*') : text_;
        viewStart_ = std::min<unsigned>(viewStart_, dtext.length() - 1);
        const auto* font = txtCtrl->GetFont();
        const unsigned max_width = GetSize().x - ctrlTextDeepening::borderSize.x * 2;
        unsigned max;
        std::string curText = s25util::utf32to8(dtext.substr(viewStart_));
        font->getWidth(curText, max_width, &max);
        // Add chars at front as long as full text is shown
        while(viewStart_ > 0 && curText.length() <= max)
        {
            --viewStart_;
            curText = s25util::utf32to8(dtext.substr(viewStart_));
            font->getWidth(curText, max_width, &max);
        }
        // Remove chars from front until remaining string can be fully shown
        while(curText.length() > max)
        {
            ++viewStart_;
            removeFirstCharFromString(curText);
            font->getWidth(curText, max_width, &max);
        }

        // Show (up to) 5 chars before the cursor
        if(viewStart_ + 5 > cursorPos_)
        {
            viewStart_ = std::max(0, static_cast<int>(cursorPos_) - 5);
            curText = s25util::utf32to8(dtext.substr(viewStart_));
        }
        if(cursorPos_ > viewStart_)
            cursorOffsetX_ = font->getWidth(s25util::utf32to8(dtext.substr(viewStart_, cursorPos_ - viewStart_)));
        else
            cursorOffsetX_ = 0;
        txtCtrl->SetText(curText);
    }
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

/**
 *  Maustaste-gedrückt Callback
 */
bool ctrlEdit::Msg_LeftDown(const MouseCoords& mc)
{
    SetFocus(IsPointInRect(mc.GetPos(), GetDrawRect()));
    return false; // "Unhandled" so other edits can handle this too and set their focus accordingly
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
        default: return false;
        // Wird bereits über Char geliefert !!
        case KeyType::Space: // Leertaste
            AddChar(0x20);
            break;

        case KeyType::Left: // Cursor nach Links
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

        case KeyType::Right: // Cursor nach Rechts
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

        case KeyType::Char: // Zeichen eingegeben
            if(!isDisabled_ && txtCtrl->GetFont()->CharExist(ke.c))
                AddChar(ke.c);
            break;

        case KeyType::Backspace: // Backspace gedrückt
            if(!isDisabled_)
                RemoveChar();
            break;

        case KeyType::Delete: // Entfernen gedrückt
            if(!isDisabled_ && cursorPos_ < text_.length())
            {
                CursorRight();
                RemoveChar();
            }
            break;

        case KeyType::Return: // Enter gedrückt
            if(!isDisabled_ && GetParent())
                GetParent()->Msg_EditEnter(GetID());
            break;

        case KeyType::Home: // Pos1 gedrückt
            if(cursorPos_ > 0)
            {
                cursorPos_ = 0;
                UpdateInternalText();
            }
            break;

        case KeyType::End: // Ende gedrückt
            if(cursorPos_ < text_.length())
            {
                cursorPos_ = text_.length();
                UpdateInternalText();
            }
            break;
    }

    return true;
}
