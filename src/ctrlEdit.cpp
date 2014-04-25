// $Id: ctrlEdit.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "main.h"
#include "ctrlEdit.h"

#include "VideoDriverWrapper.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p ctrlEdit.
 *
 *  @author OLiver
 */
ctrlEdit::ctrlEdit(Window* parent,
                   unsigned int id,
                   unsigned short x,
                   unsigned short y,
                   unsigned short width,
                   unsigned short height,
                   TextureColor tc,
                   glArchivItem_Font* font,
                   unsigned short maxlength,
                   bool password,
                   bool disabled,
                   bool notify)
    : Window(x, y, id, parent, width, height),
      maxlength(maxlength), tc(tc), font(font), password(password), disabled(disabled),
      focus(false), newfocus(false), notify(notify), number_only(false)
{
    SetText("");
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  setzt den Text.
 *
 *  @param[in] text Der Text.
 *
 *  @author OLiver
 */
void ctrlEdit::SetText(const std::string& text)
{
    cursor_pos = 0;
    view_start = 0;

    this->text = L"";

    for(unsigned i = 0; i < unsigned(text.length()); ++i)
        AddChar(text.at(i));
}

void ctrlEdit::SetText(const unsigned int text)
{
    std::stringstream textt;
    textt << text;

    cursor_pos = 0;
    view_start = 0;

    this->text = L"";

    for(unsigned i = 0; i < unsigned(textt.str().length()); ++i)
        AddChar(textt.str().at(i));
}

const std::string ctrlEdit::GetText(void) const
{
    std::string t;
    for(unsigned int i = 0; i < text.length(); ++i)
        t += font->Unicode_to_Utf8(text[i]);
    return t;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  zeichnet das Fenster.
 *
 *  @todo muss alles überarbeitet werden
 *
 *  @author OLiver
 */
bool ctrlEdit::Draw_(void)
{
    // Box malen
    Draw3D(GetX(), GetY(), width, height, tc, 2);

    std::wstring dtext;

    // Text zeichnen
    if(password)
        dtext = std::wstring(text.length(), '*');
    else
        dtext = text;

    const unsigned max_width = width - 8 - font->getDx();
    unsigned short max;
    font->getWidth(dtext.substr(view_start), unsigned(text.length()) - view_start, max_width, &max);
    while(max > 0 && text.length() - view_start > max)
    {
        ++view_start;
        font->getWidth(&dtext[view_start], unsigned(text.length()) - view_start, max_width, &max);
    }

    if(view_start > 0)
    {
        font->getWidth(dtext, unsigned(text.length()), max_width, &max);
        while(view_start > 0 && unsigned(text.length()) - view_start <= max)
        {
            --view_start;

            if(max > 0)
                font->getWidth(&dtext[view_start], unsigned(text.length()) - view_start, max_width, &max);
        }
    }

    unsigned short start = view_start;
    if(cursor_pos > 5 && cursor_pos - 5 < view_start)
        start = cursor_pos - 5;
    if(cursor_pos <= 5)
        start = 0;
    font->Draw(GetX() + 4, GetY() + height / 2, dtext.substr(start), glArchivItem_Font::DF_VCENTER,
               (focus ? 0xFFFFA000 : COLOR_YELLOW), 0, width - 8);

    // Alle 500ms Cursor für 500ms anzeigen
    if(focus && !disabled && VideoDriverWrapper::inst().GetTickCount() % 1000 < 500)
    {
        unsigned short cwidth = 5;

        if(cursor_pos - start > 0)
            cwidth = font->getWidth(&dtext[start], cursor_pos - start) + 4;

        DrawRectangle(GetX() + cwidth , GetY() + ( height - (font->getHeight() + 2) ) / 2, 1, font->getHeight() + 2, 0xFFFFA000);
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  fügt ein Zeichen zum Text hinzu
 *
 *  @param[in] text Das Zeichen
 *
 *  @author FloSoft
 */
void ctrlEdit::AddChar(unsigned int c)
{
    // Number-only text fields accept numbers only ;)
    if(number_only && !(c >= '0' && c <= '9'))
        return;

    if(maxlength > 0 && text.size() >= maxlength)
        return;

    text.insert(cursor_pos, 1, c);
    CursorRight();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  entfernt ein Zeichen
 *
 *  @author FloSoft
 */
void ctrlEdit::RemoveChar()
{
    if(cursor_pos > 0 && text.length() > 0)
    {
        text.erase(cursor_pos - 1, 1);

        // View verschieben
        while(text.length() > 0 && text.length() <= view_start)
            --view_start;

        CursorLeft();
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  benachrichtigt das Parent ("OnChange")
 *
 *  @author FloSoft
 */
void ctrlEdit::Notify()
{
    if(!notify || !parent)
        return;

    parent->Msg_EditChange(GetID());
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void ctrlEdit::Msg_PaintAfter()
{
    focus = newfocus;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Maustaste-gedrückt Callback
 *
 *  @author OLiver
 */
bool ctrlEdit::Msg_LeftDown(const MouseCoords& mc)
{
    if((newfocus = Coll(mc.x, mc.y, GetX(), GetY(), width, height)))
        return false; /// vorläufig, um Fokus zu für andere Edit-Felder zu kriegen, damit es zu keinen Doppelfokus kommt
    else
        return false;
}

// vorläufig
bool ctrlEdit::Msg_LeftDown_After(const MouseCoords& mc)
{
    if(!Coll(mc.x, mc.y, GetX(), GetY(), width, height))
        newfocus = false;

    return false;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Taste-gedrückt Callback
 *
 *  @author FloSoft
 */
bool ctrlEdit::Msg_KeyDown(const KeyEvent& ke)
{
    // hat das Steuerelement den Fokus?
    if(!focus)
        return false;

    switch(ke.kt)
    {
        default:
            return false;
            // Wird bereits über Char geliefert !!
        case KT_SPACE: // Leertaste
        {
            AddChar(0x20);
        } break;

        case KT_LEFT: // Cursor nach Links
        {
            // Blockweise nach links, falls Strg gedrückt
            if(ke.ctrl)
            {
                // Erst über alle Trennzeichen hinweg
                while (cursor_pos > 0 && std::wstring(L" \t\n-+=").find(text[cursor_pos - 1]) != std::wstring::npos)
                {
                    CursorLeft();
                    if(cursor_pos == 0)
                        break;
                }

                // Und dann über alles, was kein Trenner ist
                while (cursor_pos > 0 && std::wstring(L" \t\n-+=").find(text[cursor_pos - 1]) == std::wstring::npos)
                {
                    CursorLeft();
                    if(cursor_pos == 0)
                        break;
                }
            }

            // Sonst nur einen Schritt
            if (cursor_pos > 0)
                CursorLeft();
        } break;

        case KT_RIGHT: // Cursor nach Rechts
        {
            // Blockweise nach rechts, falls Strg gedrückt
            if(ke.ctrl)
            {
                // Erst über alle Trennzeichen hinweg
                while (cursor_pos + 1 < text.length() && std::wstring(L" \t\n-+=").find(text[cursor_pos + 1]) != std::wstring::npos)
                {
                    CursorRight();
                    if(cursor_pos == text.length())
                        break;
                }
                // Und dann über alles, was kein Trenner ist
                while (cursor_pos + 1 < text.length() && std::wstring(L" \t\n-+=").find(text[cursor_pos + 1]) == std::wstring::npos)
                {
                    CursorRight();
                    if(cursor_pos == text.length())
                        break;
                }
            }

            // Sonst nur einen Schritt
            if (cursor_pos < text.length())
                CursorRight();
        } break;

        case KT_CHAR: // Zeichen eingegeben
        {
            if(!disabled && font->CharExist( ke.c ))
                AddChar(ke.c);
        } break;

        case KT_BACKSPACE: // Backspace gedrückt
        {
            if(!disabled)
                RemoveChar();
        } break;

        case KT_DELETE: // Entfernen gedrückt
        {
            if(!disabled && cursor_pos < text.length())
            {
                CursorRight();
                RemoveChar();
            }
        } break;

        case KT_RETURN: // Enter gedrückt
        {
            if(!disabled && parent)
                parent->Msg_EditEnter(GetID());
        } break;

        case KT_HOME: // Pos1 gedrückt
        {
            while(cursor_pos > 0)
                CursorLeft();
        } break;

        case KT_END: // Ende gedrückt
        {
            while(cursor_pos < text.length())
                CursorRight();
        } break;
    }

    return true;
}
